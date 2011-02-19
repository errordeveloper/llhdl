#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <util.h>

#include <mapkit/mapkit.h>
#include <tilm/tilm.h>

#include "internal.h"

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

static void tilm_process_c(struct llhdl_node **n, void *user)
{
	struct tilm_sc *sc = user;
	switch(sc->mapper_id) {
		case TILM_SHANNON:
			tilm_process_shannon(sc, n);
			break;
		case TILM_BDSPGA:
			tilm_process_bdspga(sc, n);
			break;
		default:
			assert(0);
			break;
	}
}

static void tilm_free_c(void *user)
{
	free(user);
}

void tilm_register(struct mapkit_sc *mapkit,
	int mapper_id,
	int max_inputs,
	void *extra_mapper_param,
	tilm_create_net_c create_net,
	tilm_create_lut_c create_lut,
	tilm_create_mux_c create_mux,
	void *user)
{
	struct tilm_sc *sc;
	
	assert(max_inputs >= 3);
	sc = alloc_type(struct tilm_sc);
	sc->mapkit = mapkit;
	sc->mapper_id = mapper_id;
	sc->max_inputs = max_inputs;
	sc->extra_mapper_param = extra_mapper_param;
	sc->create_net = create_net;
	sc->create_lut = create_lut;
	sc->create_mux = create_mux;
	sc->user = user;
	
	mapkit_register_process(mapkit, tilm_process_c, tilm_free_c, sc);
}

