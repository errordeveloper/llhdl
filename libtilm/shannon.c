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

void tilm_process_shannon(struct tilm_sc *sc, struct llhdl_node **n)
{
	struct mapkit_result *r;
	struct tilm_variables *var;
	
	r = tilm_try_partition(sc, n);
	if(r == NULL) return;
	
	var = tilm_variables_enumerate(*n);
	tilm_variables_dump(var);
	tilm_variables_free(var);
	
	mapkit_consume(sc->mapkit, *n, r);
}
