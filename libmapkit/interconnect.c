#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

#include <util.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <mapkit/mapkit.h>

static void mapkit_interconnect_down(struct mapkit_sc *sc, int target_vectorsize, int target_sign, void **target_nets, struct llhdl_node *source)
{
	int source_vectorsize, source_sign;
	void **source_nets;
	struct mapkit_result *mapr;
	int vectorsize, sign;
	struct llhdl_node *n;
	int i, offset;
	int sign_extend;
	
	if(source == NULL) return;
	
	/* Retrieve the source nets in their original size */
	source_vectorsize = llhdl_get_vectorsize(source);
	source_sign = llhdl_get_sign(source);
	source_nets = alloc_size(source_vectorsize*sizeof(void *));
	mapr = source->user;
	if(mapr != NULL) {
		memcpy(source_nets, mapr->output_nets, source_vectorsize*sizeof(void *));
		offset = 0;
		for(i=0;i<mapr->ninput_nodes;i++) {
			n = *(mapr->input_nodes[i]);
			vectorsize = llhdl_get_vectorsize(n);
			sign = llhdl_get_sign(n);
			mapkit_interconnect_down(sc, vectorsize, sign, &mapr->input_nets[offset], n);
			offset += vectorsize;
		}
	} else {
		if(source->type == LLHDL_NODE_SIGNAL) {
			for(i=0;i<source_vectorsize;i++)
				source_nets[i] = MAPKIT_CALL_SIGNAL(sc, source, i);
		} else {
			fprintf(stderr, "LLHDL node of type '%s' failed to map\n", llhdl_strtype(source->type));
			exit(EXIT_FAILURE);
		}
	}
	
	/* Make the connection, cutting or expanding to fit the target vectorsize */
	sign_extend = source_sign && target_sign;
	for(i=0;i<target_vectorsize;i++) {
		if(i < source_vectorsize)
			MAPKIT_CALL_JOIN(sc, target_nets[i], source_nets[i]);
		else {
			if(sign_extend)
				MAPKIT_CALL_JOIN(sc, target_nets[i], source_nets[source_vectorsize-1]);
			else
				MAPKIT_CALL_JOIN(sc, target_nets[i], MAPKIT_CALL_CONSTANT(sc, 0));
		}
	}
	
	free(source_nets);
}

void mapkit_interconnect_arc(struct mapkit_sc *sc, struct llhdl_node *n)
{
	void **target_nets;
	int i;
	
	assert(n->type == LLHDL_NODE_SIGNAL);
	target_nets = alloc_size(n->p.signal.vectorsize*sizeof(void *));
	for(i=0;i<n->p.signal.vectorsize;i++)
		target_nets[i] = MAPKIT_CALL_SIGNAL(sc, n, i);
	mapkit_interconnect_down(sc, n->p.signal.vectorsize, n->p.signal.sign, target_nets, n->p.signal.source);
	free(target_nets);
}
