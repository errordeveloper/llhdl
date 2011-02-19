#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/xilprims.h>

#include <mapkit/mapkit.h>

#include "flow.h"

static void mkc_process(struct llhdl_node **n2, void *user)
{
	struct llhdl_node *n = *n2;
	struct flow_sc *sc = user;
	int n_bits;
	int i;
	struct mapkit_result *result;
	struct netlist_instance *inst;
	
	if(n->type == LLHDL_NODE_FD) {
		if(llhdl_get_vectorsize(n->p.fd.clock) != 1) {
			fprintf(stderr, "Flip-flop clock must be a single signal\n");
			exit(EXIT_FAILURE);
		}
		n_bits = llhdl_get_vectorsize(n->p.fd.data);
		result = mapkit_create_result(2, 1+n_bits, n_bits);
		result->input_nodes[0] = &n->p.fd.clock;
		result->input_nodes[1] = &n->p.fd.data;
		result->input_nets[0] = netlist_m_create_net(sc->netlist); /* < clock */
		for(i=0;i<n_bits;i++) {
			inst = netlist_m_instantiate(sc->netlist, &netlist_xilprims[NETLIST_XIL_FD]);
			netlist_add_branch(result->input_nets[0], inst, 0, NETLIST_XIL_FD_C);
			result->input_nets[i+1] = netlist_m_create_net_with_branch(sc->netlist, inst, 0, NETLIST_XIL_FD_D);
			result->output_nets[i] = netlist_m_create_net_with_branch(sc->netlist, inst, 1, NETLIST_XIL_FD_Q);
		}
		mapkit_consume(sc->mapkit, n, result);
	}
}


void fd_register(struct flow_sc *sc)
{
	mapkit_register_process(sc->mapkit, mkc_process, NULL, sc);
}

