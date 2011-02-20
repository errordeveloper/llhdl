#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <util.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <mapkit/mapkit.h>

#include <tilm/tilm.h>

#include "internal.h"
#include "partition.h"

struct part_node {
	struct llhdl_node **n;
	struct part_node *next;
};

struct part_node_list {
	int nnodes;
	int nbits;
	struct part_node *head;
};

static void part_node_list_add(struct part_node_list *nl, struct llhdl_node **n)
{
	struct part_node *new;
	struct part_node *last;
	
	new = alloc_type(struct part_node);
	new->n = n;
	new->next = NULL;
	
	if(nl->head == NULL)
		nl->head = new;
	else {
		last = nl->head;
		while(last->next != NULL)
			last = last->next;
		last->next = new;
	}
	
	nl->nnodes++;
	nl->nbits += llhdl_get_vectorsize(*n);
}

static void part_node_list_free(struct part_node_list *nl)
{
	struct part_node *n1, *n2;
	
	n1 = nl->head;
	while(n1 != NULL) {
		n2 = n1->next;
		free(n1);
		n1 = n2;
	}
}

static void find_partition_boundary(struct part_node_list *nl, struct llhdl_node **n)
{
	int i, arity;

	if((*n)->user != NULL) {
		/* Already mapped */
		part_node_list_add(nl, n);
		return;
	}

	switch((*n)->type) {
		case LLHDL_NODE_CONSTANT:
			/* nothing to do */
			break;
		case LLHDL_NODE_LOGIC:
			arity = llhdl_get_logic_arity((*n)->p.logic.op);
			for(i=0;i<arity;i++)
				find_partition_boundary(nl, &(*n)->p.logic.operands[i]);
			break;
		case LLHDL_NODE_MUX:
			find_partition_boundary(nl, &(*n)->p.mux.select);
			for(i=0;i<(*n)->p.mux.nsources;i++)
				find_partition_boundary(nl, &(*n)->p.mux.sources[i]);
			break;
		case LLHDL_NODE_VECT:
			for(i=0;i<(*n)->p.vect.nslices;i++)
				find_partition_boundary(nl, &(*n)->p.vect.slices[i].source);
			break;
		default:
			part_node_list_add(nl, n);
			break;
	}
}

struct mapkit_result *tilm_try_partition(struct tilm_sc *sc, struct llhdl_node **n)
{
	struct part_node_list nl;
	int noutputs;
	struct mapkit_result *r;
	int i;
	struct part_node *pn;
	
	/* We should not try to map already mapped nodes. */
	assert((*n)->user == NULL);
	/* A LUT mapper should handle exactly these node types. */
	if(((*n)->type != LLHDL_NODE_CONSTANT) &&
	   ((*n)->type != LLHDL_NODE_LOGIC) &&
	   ((*n)->type != LLHDL_NODE_MUX) &&
	   ((*n)->type != LLHDL_NODE_VECT))
	   	return NULL;

	/* OK - we map the current node. Find the boundaries of the partition. */
	nl.nnodes = 0;
	nl.nbits = 0;
	nl.head = NULL;
	find_partition_boundary(&nl, n);
	noutputs = llhdl_get_vectorsize(*n);
	
	/* Generate the Mapkit result structure */
	r = mapkit_create_result(nl.nnodes, nl.nbits, noutputs);
	i = 0;
	pn = nl.head;
	while(pn != NULL) {
		r->input_nodes[i] = pn->n;
		pn = pn->next;
		i++;
	}
	for(i=0;i<nl.nbits;i++)
		r->input_nets[i] = TILM_CALL_CREATE_NET(sc);
	for(i=0;i<noutputs;i++)
		r->output_nets[i] = TILM_CALL_CREATE_NET(sc);
	
	/* Clean up */
	part_node_list_free(&nl);
	
	return r;
}
