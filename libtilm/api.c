#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <util.h>

#include <tilm/tilm.h>
#include <tilm/mappers.h>

struct tilm_desc tilm_mappers[] = {
	{
		.handle = "shannon",
		.description = "Naive Shannon decomposition based"
	},
	{
		.handle = "bdspga",
		.description = "BDS-PGA"
	}
};

int tilm_get_mapper_by_handle(const char *handle)
{
	int i;
	
	for(i=0;i<TILM_COUNT;i++) {
		if(strcmp(tilm_mappers[i].handle, handle) == 0)
			return i;
	}
	return -1;
}

struct tilm_result *tilm_map(struct tilm_param *p, struct llhdl_node *top)
{
	assert(p->max_inputs >= 3);
	assert(p->create != NULL);
	assert(p->connect != NULL);
	switch(p->mapper_id) {
		case TILM_SHANNON:
			return tilm_shannon_map(p, top);
		case TILM_BDSPGA:
			return tilm_bdspga_map(p, top);
		default:
			assert(0);
			return NULL;
	}
}

struct tilm_result *tilm_create_result(int vectorsize)
{
	struct tilm_result *r;

	r = alloc_type(struct tilm_result);
	r->vectorsize = vectorsize;
	r->out_insts = alloc_size(vectorsize*sizeof(void *));
	memset(r->out_insts, 0, vectorsize*sizeof(void *));
	r->merges = alloc_size(vectorsize*sizeof(struct tilm_merge));
	memset(r->merges, 0, vectorsize*sizeof(struct tilm_merge));
	r->ihead = NULL;
	return r;
}

void tilm_result_add_input(struct tilm_result *r, struct llhdl_node *signal, int bit, void *inst, int n)
{
	struct tilm_input_assoc *a;
	
	a = alloc_type(struct tilm_input_assoc);
	a->signal = signal;
	a->bit = bit;
	a->inst = inst;
	a->n = n;
	a->next = r->ihead;
	r->ihead = a;
}

void tilm_result_add_merge(struct tilm_result *r, int i, struct llhdl_node *signal, int bit)
{
	assert(r->merges[i].signal == NULL);
	r->merges[i].signal = signal;
	r->merges[i].bit = bit;
}

void tilm_free_result(struct tilm_result *r)
{
	struct tilm_input_assoc *a1, *a2;
	
	a1 = r->ihead;
	while(a1 != NULL) {
		a2 = a1->next;
		free(a1);
		a1 = a2;
	}
	
	free(r->out_insts);
	free(r->merges);
	free(r);
}
