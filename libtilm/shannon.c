#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <gmp.h>

#include <util.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>
#include <tilm/mappers.h>

#include "rg.h"

struct enumerated_signal {
	struct llhdl_node *ln;
	int bit;
	int value;
	struct enumerated_signal *next;
};

struct enumeration {
	struct enumerated_signal *head;
};

static struct enumeration *enumeration_new()
{
	struct enumeration *e;
	
	e = alloc_type(struct enumeration);
	e->head = NULL;
	return e;
}

static struct enumerated_signal *enumeration_find(struct enumeration *e, struct llhdl_node *ln, int bit)
{
	struct enumerated_signal *s;
	
	s = e->head;
	while(s != NULL) {
		if((s->ln == ln) && (s->bit == bit))
			return s;
		s = s->next;
	}
	return NULL;
}

static void enumeration_add(struct enumeration *e, struct llhdl_node *ln, int bit)
{
	struct enumerated_signal *s;
	
	if(enumeration_find(e, ln, bit) != NULL)
		return;
	s = alloc_type(struct enumerated_signal);
	s->ln = ln;
	s->bit = bit;
	s->value = 0;
	s->next = e->head;
	e->head = s;
}

struct llhdl_node *enumeration_get_value(struct enumeration *e, struct llhdl_node *ln)
{
	struct enumerated_signal *s;
	int i;
	mpz_t v;
	struct llhdl_node *n;
	
	mpz_init2(v, ln->p.signal.vectorsize);
	for(i=0;i<ln->p.signal.vectorsize;i++) {
		s = enumeration_find(e, ln, i);
		if((s != NULL) && s->value)
			mpz_setbit(v, i);
	}
	n = llhdl_create_constant(v, ln->p.signal.sign, ln->p.signal.vectorsize);
	mpz_clear(v);
	return n;
}

static void enumeration_free(struct enumeration *e)
{
	struct enumerated_signal *s1, *s2;
	
	s1 = e->head;
	while(s1 != NULL) {
		s2 = s1->next;
		free(s1);
		s1 = s2;
	}
	free(e);
}

static void enumerate(struct enumeration *e, struct llhdl_node *n, int bit)
{
	int i, j;
	int arity;
	int len;

	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			break;
		case LLHDL_NODE_SIGNAL:
			if(bit < 0) {
				for(i=0;i<n->p.signal.vectorsize;i++)
					enumeration_add(e, n, i);
			} else if(bit < n->p.signal.vectorsize)
				enumeration_add(e, n, bit);
			break;
		case LLHDL_NODE_LOGIC:
			arity = llhdl_get_logic_arity(n->p.logic.op);
			for(i=0;i<arity;i++)
				enumerate(e, n->p.logic.operands[i], bit);
			break;
		case LLHDL_NODE_MUX:
			enumerate(e, n->p.mux.select, -1);
			for(i=0;i<n->p.mux.nsources;i++)
				enumerate(e, n->p.mux.sources[i], bit);
			break;
		case LLHDL_NODE_VECT:
			if(bit < 0) {
				for(i=0;i<n->p.vect.nslices;i++)
					for(j=n->p.vect.slices[i].start;j<=n->p.vect.slices[i].end;j++)
						enumerate(e, n->p.vect.slices[i].source, j);
			} else {
				for(i=0;i<n->p.vect.nslices;i++) {
					len = n->p.vect.slices[i].end - n->p.vect.slices[i].start + 1;
					if(bit < len) {
						enumerate(e, n->p.vect.slices[i].source, n->p.vect.slices[i].start + bit);
						break;
					}
					bit -= len;
				}
			}
			break;
		default:
			assert(0);
			break;
	}
}

static int count_remaining_variables(struct enumerated_signal *s)
{
	int i;
	
	i = 0;
	while(s != NULL) {
		i++;
		s = s->next;
	}
	return i;
}

static void compose_from_slices(mpz_t v, struct llhdl_slice *slices, int n)
{
	int i, j;
	int nbits;
	int bitindex;
	
	bitindex = 0;
	for(i=n-1;i>=0;i--) {
		nbits = slices[i].end - slices[i].start + 1;
		for(j=0;j<nbits;i++) {
			if(mpz_tstbit(slices[i].source->p.constant.value, j))
				mpz_setbit(v, bitindex);
			bitindex++;
		}
	}
}

static struct llhdl_node *eval(struct enumeration *e, struct llhdl_node *n)
{
	mpz_t v;
	int i, arity;
	struct llhdl_node *value;
	struct llhdl_node **values;
	struct llhdl_node *r; /* returned constant */

	r = NULL;
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			r = llhdl_create_constant(n->p.constant.value, n->p.constant.sign, n->p.constant.vectorsize);
			break;
		case LLHDL_NODE_SIGNAL:
			r = enumeration_get_value(e, n);
			break;
		case LLHDL_NODE_VECT:
			values = alloc_size(n->p.vect.nslices*sizeof(struct llhdl_node *));
			for(i=0;i<n->p.vect.nslices;i++)
				values[i] = eval(e, n->p.vect.slices[i].source);
			mpz_init(v);
			compose_from_slices(v, n->p.vect.slices, n->p.vect.nslices);
			for(i=0;i<n->p.vect.nslices;i++)
				llhdl_free_node(values[i]);
			free(values);
			r = llhdl_create_constant(v, llhdl_get_sign(n), llhdl_get_vectorsize(n));
			mpz_clear(v);
			break;
		case LLHDL_NODE_LOGIC:
			arity = llhdl_get_logic_arity(n->p.logic.op);
			values = alloc_size(arity*sizeof(struct llhdl_node *));
			for(i=0;i<arity;i++)
				values[i] = eval(e, n->p.logic.operands[i]);
			/* TODO: sign extension when appropriate */
			mpz_init(v);
			switch(n->p.logic.op) {
				case LLHDL_LOGIC_NOT:
					mpz_com(v, values[0]->p.constant.value);
					break;
				case LLHDL_LOGIC_AND:
					mpz_and(v, values[0]->p.constant.value, values[1]->p.constant.value);
					break;
				case LLHDL_LOGIC_OR:
					mpz_ior(v, values[0]->p.constant.value, values[1]->p.constant.value);
					break;
				case LLHDL_LOGIC_XOR:
					mpz_xor(v, values[0]->p.constant.value, values[1]->p.constant.value);
					break;
				default:
					assert(0);
					break;
			}
			r = llhdl_create_constant(v, llhdl_get_sign(n), llhdl_get_vectorsize(n));
			mpz_clear(v);
			for(i=0;i<arity;i++)
				llhdl_free_node(values[i]);
			free(values);
			break;
		case LLHDL_NODE_MUX:
			value = eval(e, n->p.mux.select);
			i = mpz_get_si(value->p.constant.value);
			llhdl_free_node(value);
			if(i < n->p.mux.nsources) {
				r = eval(e, n->p.mux.sources[i]);
				r->p.constant.sign = llhdl_get_sign(n);
				r->p.constant.vectorsize = llhdl_get_vectorsize(n);
			}
			break;
		default:
			assert(0);
			break;
	}
	
	/* Catch undefined values */
	if(r == NULL) {
		mpz_init(v);
		r = llhdl_create_constant(v, llhdl_get_sign(n), llhdl_get_vectorsize(n));
		mpz_clear(v);
	}
	
	return r;
}

static void eval_and_set(struct enumeration *e, struct llhdl_node *n, mpz_t result, int bit)
{
	struct llhdl_node *evn;

	mpz_mul_2exp(result, result, 1);
	evn = eval(e, n);
	if(mpz_tstbit(evn->p.constant.value, bit))
		mpz_setbit(result, 0);
	llhdl_free_node(evn);
}

static void eval_multi(struct enumeration *e, struct enumerated_signal *s, struct llhdl_node *n, mpz_t result, int bit)
{
	if(s->next == NULL) {
		s->value = 1;
		eval_and_set(e, n, result, bit);
		s->value = 0;
		eval_and_set(e, n, result, bit);
	} else {
		s->value = 1;
		eval_multi(e, s->next, n, result, bit);
		s->value = 0;
		eval_multi(e, s->next, n, result, bit);
	}
}

static void *new_mux(struct tilm_param *p)
{
	void *lut;
	mpz_t contents;
	
	mpz_init_set_ui(contents, 0xE4);
	lut = TILM_CALL_CREATE(p, 3, contents);
	mpz_clear(contents);
	return lut;
}

struct map_level_param {
	struct tilm_param *p;
	struct enumeration *e;
	struct llhdl_node *top;
	struct tilm_rg *rg;
	int bit;
};

static void *map_level(struct map_level_param *mlp, struct enumerated_signal *s)
{
	int varcount;
	int i;
	
	varcount = count_remaining_variables(s);
	if(varcount <= mlp->p->max_inputs) {
		mpz_t contents;
		void *lut;
		
		mpz_init2(contents, 1 << varcount);
		eval_multi(mlp->e, s, mlp->top, contents, mlp->bit);
		lut = TILM_CALL_CREATE(mlp->p, varcount, contents);
		mpz_clear(contents);
		
		i = varcount;
		while(s != NULL) {
			i--;
			tilm_rg_add(mlp->rg, s->ln, s->bit, lut, i);
			s = s->next;
		}
		assert(i == 0);
		
		return lut;
	} else {
		void *negative;
		void *positive;
		void *mux;
		
		s->value = 0;
		negative = map_level(mlp, s->next);
		s->value = 1;
		positive = map_level(mlp, s->next);
		
		mux = new_mux(mlp->p);
		tilm_rg_add(mlp->rg, s->ln, s->bit, mux, 0);
		TILM_CALL_CONNECT(mlp->p, negative, mux, 1);
		TILM_CALL_CONNECT(mlp->p, positive, mux, 2);
		
		return mux;
	}
}

struct tilm_result *tilm_shannon_map(struct tilm_param *p, struct llhdl_node *top)
{
	struct map_level_param mlp;
	struct tilm_result *result;
	int vectorsize;
	void **out_luts;
	int i;

	mlp.p = p;
	mlp.top = top;
	mlp.rg = tilm_rg_new();

	vectorsize = llhdl_get_vectorsize(top);
	out_luts = alloc_size(vectorsize*sizeof(void *));
	
	for(i=0;i<vectorsize;i++) {
		mlp.e = enumeration_new();
		mlp.bit = i;
		enumerate(mlp.e, top, i);
		out_luts[i] = map_level(&mlp, mlp.e->head);
		enumeration_free(mlp.e);
	}
	
	result = tilm_rg_generate(mlp.rg, out_luts, vectorsize);
	
	tilm_rg_free(mlp.rg);

	return result;
}
