#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <gmp.h>

#include <util.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <mapkit/mapkit.h>

#include "partition.h"
#include "variables.h"
#include "internal.h"

struct llhdl_node *get_node_value(struct tilm_variables *var, int obit, struct llhdl_node *n)
{
	int vectorsize;
	int sign;
	int i;
	mpz_t v;
	struct llhdl_node *on;
	
	vectorsize = llhdl_get_vectorsize(n);
	sign = llhdl_get_sign(n);
	mpz_init2(v, vectorsize);
	for(i=0;i<vectorsize;i++) {
		if(tilm_variable_value(var, obit, n, i))
			mpz_setbit(v, i);
	}
	on = llhdl_create_constant(v, sign, vectorsize);
	mpz_clear(v);
	return on;
}

static void compose_from_slices(mpz_t v, struct llhdl_slice *slices, struct llhdl_node **values, int n)
{
	int i, j;
	int bitindex;
	
	bitindex = 0;
	for(i=0;i<n;i++) {
		for(j=slices[i].start;j<=slices[i].end;j++) {
			if(mpz_tstbit(values[i]->p.constant.value, j))
				mpz_setbit(v, bitindex);
			bitindex++;
		}
	}
}

static struct llhdl_node *eval(struct tilm_variables *var, int obit, struct llhdl_node *n)
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
		case LLHDL_NODE_VECT:
			values = alloc_size(n->p.vect.nslices*sizeof(struct llhdl_node *));
			for(i=0;i<n->p.vect.nslices;i++)
				values[i] = eval(var, obit, n->p.vect.slices[i].source);
			mpz_init(v);
			compose_from_slices(v, n->p.vect.slices, values, n->p.vect.nslices);
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
				values[i] = eval(var, obit, n->p.logic.operands[i]);
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
			value = eval(var, obit, n->p.mux.select);
			i = mpz_get_si(value->p.constant.value);
			llhdl_free_node(value);
			if(i < n->p.mux.nsources) {
				r = eval(var, obit, n->p.mux.sources[i]);
				r->p.constant.sign = llhdl_get_sign(n);
				r->p.constant.vectorsize = llhdl_get_vectorsize(n);
			} /* otherwise, result is undefined and caught below */
			break;
		default:
			r = get_node_value(var, obit, n);
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

struct map_level_param {
	struct tilm_sc *sc;
	struct llhdl_node *top;
	struct mapkit_result *r;
	struct tilm_variables *var;
	int obit;
};

static void eval_and_set(struct map_level_param *mlp, mpz_t result)
{
	struct llhdl_node *evn;

	mpz_mul_2exp(result, result, 1);
	evn = eval(mlp->var, mlp->obit, mlp->top);
	if(mpz_tstbit(evn->p.constant.value, mlp->obit))
		mpz_setbit(result, 0);
	llhdl_free_node(evn);
}

static void eval_multi(struct map_level_param *mlp, struct tilm_variable *v, mpz_t result)
{
	if(v->next == NULL) {
		v->value = 1;
		eval_and_set(mlp, result);
		v->value = 0;
		eval_and_set(mlp, result);
	} else {
		v->value = 1;
		eval_multi(mlp, v->next, result);
		v->value = 0;
		eval_multi(mlp, v->next, result);
	}
}

static void *fit_into_lut(struct map_level_param *mlp, struct tilm_variable *v)
{
	int varcount;
	mpz_t contents;
	void *lut;
	int i;
	struct tilm_variable *v2;
	void *in_net;
	void *lut_net;

	varcount = tilm_variables_remaining(v);
	
	mpz_init2(contents, 1 << varcount);
	eval_multi(mlp, v, contents);
	
	if(varcount == 0)
		/* Constant */
		lut_net = TILM_CALL_CONSTANT(mlp->sc, mpz_get_ui(contents));
	else if((varcount == 1) && (mpz_get_ui(contents) == 2))
		/* Identity */
		lut_net = mapkit_find_input_net(mlp->r, v->n, v->bit);
	else {
		/* Arbitrary function */
		lut = TILM_CALL_CREATE_LUT(mlp->sc, varcount, contents);
	
		i = varcount;
		v2 = v;
		while(v2 != NULL) {
			i--;
			in_net = mapkit_find_input_net(mlp->r, v2->n, v2->bit);
			TILM_CALL_BRANCH(mlp->sc, in_net, lut, 0, i);
			v2 = v2->next;
		}
		assert(i == 0);
		
		lut_net = TILM_CALL_CREATE_NET(mlp->sc);
		TILM_CALL_BRANCH(mlp->sc, lut_net, lut, 1, 0);
	}
	
	mpz_clear(contents);
	
	return lut_net;
}

static void *create_mux_lut(struct tilm_sc *sc)
{
	void *lut;
	mpz_t contents;
	
	mpz_init_set_ui(contents, 0xE4);
	lut = TILM_CALL_CREATE_LUT(sc, 3, contents);
	mpz_clear(contents);
	return lut;
}

static void *create_mux(struct tilm_sc *sc, int muxlevel)
{
	void *r;

	if(sc->create_mux_c == NULL)
		return create_mux_lut(sc);
	r = TILM_CALL_CREATE_MUX(sc, muxlevel);
	if(r == NULL)
		return create_mux_lut(sc);
	return r;
}

static void *map_level(struct map_level_param *mlp, struct tilm_variable *v, int level);

static void *decompose(struct map_level_param *mlp, struct tilm_variable *v, int level)
{
	void *negative_net;
	void *positive_net;
	void *select_net;
	void *mux;
	void *mux_net;
	
	v->value = 0;
	negative_net = map_level(mlp, v->next, level+1);
	v->value = 1;
	positive_net = map_level(mlp, v->next, level+1);
	
	select_net = mapkit_find_input_net(mlp->r, v->n, v->bit);
	
	mux = create_mux(mlp->sc, level);
	TILM_CALL_BRANCH(mlp->sc, select_net, mux, 0, 0);
	TILM_CALL_BRANCH(mlp->sc, negative_net, mux, 0, 1);
	TILM_CALL_BRANCH(mlp->sc, positive_net, mux, 0, 2);
	
	mux_net = TILM_CALL_CREATE_NET(mlp->sc);
	TILM_CALL_BRANCH(mlp->sc, mux_net, mux, 1, 0);
	
	return mux_net;
}

static void *map_level(struct map_level_param *mlp, struct tilm_variable *v, int level)
{
	int varcount;
	
	varcount = tilm_variables_remaining(v);
	if(varcount <= mlp->sc->max_inputs)
		return fit_into_lut(mlp, v);
	else
		return decompose(mlp, v, level);
}

void tilm_process_shannon(struct tilm_sc *sc, struct llhdl_node **n)
{
	struct map_level_param mlp;
	int vectorsize;
	
	mlp.sc = sc;
	mlp.top = *n;
	
	mlp.r = tilm_try_partition(sc, n);
	if(mlp.r == NULL) return;
	mlp.var = tilm_variables_enumerate(*n);
	
	vectorsize = llhdl_get_vectorsize(*n);
	for(mlp.obit=0;mlp.obit<vectorsize;mlp.obit++)
		mlp.r->output_nets[mlp.obit] = map_level(&mlp, mlp.var->heads[mlp.obit], 0);

	tilm_variables_free(mlp.var);
	mapkit_consume(sc->mapkit, *n, mlp.r);
}
