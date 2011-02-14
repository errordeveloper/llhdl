#include <string.h>
#include <stdlib.h>

#include <llhdl/structure.h>
#include <tilm/tilm.h>

#include <util.h>

#include "rg.h"

struct tilm_rg *tilm_rg_new()
{
	struct tilm_rg *r;
	
	r = alloc_type(struct tilm_rg);
	r->head = NULL;
	return r;
}

void tilm_rg_free(struct tilm_rg *rg)
{
	struct tilm_rg_input_assoc *a1, *a2;
	
	a1 = rg->head;
	while(a1 != NULL) {
		a2 = a1->next;
		free(a1);
		a1 = a2;
	}
	free(rg);
}

void tilm_rg_add(struct tilm_rg *rg, struct llhdl_node *signal, int bit, void *lut, int n)
{
	struct tilm_rg_input_assoc *a;
	
	a = alloc_type(struct tilm_rg_input_assoc);
	a->payload.signal = signal;
	a->payload.bit = bit;
	a->payload.lut = lut;
	a->payload.n = n;
	a->next = rg->head;
	rg->head = a;
}

int tilm_rg_count(struct tilm_rg *rg)
{
	int i;
	struct tilm_rg_input_assoc *a;
	
	i = 0;
	a = rg->head;
	while(a != NULL) {
		i++;
		a = a->next;
	}
	return i;
}

struct tilm_result *tilm_rg_generate(struct tilm_rg *rg, void **out_luts, int vectorsize)
{
	int count;
	struct tilm_result *r;
	int i;
	struct tilm_rg_input_assoc *a;
	
	count = tilm_rg_count(rg);
	r = alloc_size(sizeof(struct tilm_result)+count*sizeof(struct tilm_input_assoc));
	r->out_luts = out_luts;
	r->vectorsize = vectorsize;
	r->n_input_assoc = count;
	a = rg->head;
	i = 0;
	while(a != NULL) {
		memcpy(&r->input_assoc[i], &a->payload, sizeof(struct tilm_input_assoc));
		i++;
		a = a->next;
	}
	
	return r;
}

