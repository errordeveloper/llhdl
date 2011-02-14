#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

void tilm_free_result(struct tilm_result *r)
{
	free(r->out_luts);
	free(r);
}
