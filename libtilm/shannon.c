#include <assert.h>
#include <stdlib.h>
#include <gmp.h>

#include <util.h>

#include <llhdl/structure.h>
#include <tilm/mappers.h>

#include "rg.h"

struct enumerated_signal {
	struct llhdl_node *ln;
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

static struct enumerated_signal *enumeration_find(struct enumeration *e, struct llhdl_node *ln)
{
	struct enumerated_signal *s;
	
	s = e->head;
	while(s != NULL) {
		if(s->ln == ln)
			return s;
		s = s->next;
	}
	return NULL;
}

static void enumeration_add(struct enumeration *e, struct llhdl_node *ln)
{
	struct enumerated_signal *s;
	
	if(enumeration_find(e, ln) != NULL)
		return;
	s = alloc_type(struct enumerated_signal);
	s->ln = ln;
	s->value = 0;
	s->next = e->head;
	e->head = s;
}

static int enumeration_get_value(struct enumeration *e, struct llhdl_node *ln)
{
	struct enumerated_signal *s;
	
	s = enumeration_find(e, ln);
	assert(s != NULL);
	return s->value;
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

static void enumerate(struct enumeration *e, struct llhdl_node *n)
{
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			break;
		case LLHDL_NODE_SIGNAL:
			enumeration_add(e, n);
			break;
		case LLHDL_NODE_MUX:
			assert(n->p.mux.sel->type == LLHDL_NODE_SIGNAL);
			enumeration_add(e, n->p.mux.sel);
			enumerate(e, n->p.mux.negative);
			enumerate(e, n->p.mux.positive);
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

static int eval(struct enumeration *e, struct llhdl_node *n)
{
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			return mpz_get_si(n->p.constant.value);
		case LLHDL_NODE_SIGNAL:
			return enumeration_get_value(e, n);
		case LLHDL_NODE_MUX:
			assert(n->p.mux.sel->type == LLHDL_NODE_SIGNAL);
			if(enumeration_get_value(e, n->p.mux.sel))
				return eval(e, n->p.mux.positive);
			else
				return eval(e, n->p.mux.negative);
			break;
		default:
			assert(0);
			break;
	}
}

static void eval_vec(struct enumeration *e, struct enumerated_signal *s, struct llhdl_node *n, mpz_t result)
{
	if(s->next == NULL) {
		s->value = 1;
		mpz_mul_2exp(result, result, 1);
		if(eval(e, n))
			mpz_setbit(result, 0);
		s->value = 0;
		mpz_mul_2exp(result, result, 1);
		if(eval(e, n))
			mpz_setbit(result, 0);
	} else {
		s->value = 1;
		eval_vec(e, s->next, n, result);
		s->value = 0;
		eval_vec(e, s->next, n, result);
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
};

static void *map_level(struct map_level_param *mlp, struct enumerated_signal *s)
{
	int varcount;
	
	varcount = count_remaining_variables(s);
	if(varcount <= mlp->p->max_inputs) {
		mpz_t contents;
		void *lut;
		int i;
		
		mpz_init2(contents, 1 << varcount);
		eval_vec(mlp->e, s, mlp->top, contents);
		lut = TILM_CALL_CREATE(mlp->p, varcount, contents);
		mpz_clear(contents);
		
		i = varcount;
		while(s != NULL) {
			i--;
			tilm_rg_add(mlp->rg, s->ln, lut, i);
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
		tilm_rg_add(mlp->rg, s->ln, mux, 0);
		TILM_CALL_CONNECT(mlp->p, negative, mux, 1);
		TILM_CALL_CONNECT(mlp->p, positive, mux, 2);
		
		return mux;
	}
}

static struct llhdl_node *is_identity(struct llhdl_node *n)
{
	if(n->type != LLHDL_NODE_MUX) return NULL;
	if((n->p.mux.negative->type != LLHDL_NODE_CONSTANT) ||  (mpz_get_si(n->p.mux.negative->p.constant.value))) return NULL;
	if((n->p.mux.positive->type != LLHDL_NODE_CONSTANT) || !(mpz_get_si(n->p.mux.positive->p.constant.value))) return NULL;
	return n->p.mux.sel;
}

struct tilm_result *tilm_shannon_map(struct tilm_param *p, struct llhdl_node *top)
{
	struct map_level_param mlp;
	struct tilm_result *result;
	void *out_lut;
	struct llhdl_node *identity;

	mlp.p = p;
	mlp.e = enumeration_new();
	mlp.top = top;
	mlp.rg = tilm_rg_new();
	
	identity = is_identity(top);
	if(identity != NULL) {
		tilm_rg_add(mlp.rg, identity, NULL, 0);
		out_lut = NULL;
	} else {
		enumerate(mlp.e, top);
		out_lut = map_level(&mlp, mlp.e->head);
	}
	
	result = tilm_rg_generate(mlp.rg, out_lut);
	
	tilm_rg_free(mlp.rg);
	enumeration_free(mlp.e);

	return result;
}
