#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gmp.h>

#include <util.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/xilprims.h>

#include <mapkit/mapkit.h>

#include "flow.h"
#include "commonstruct.h"
#include "carryarith.h"

static struct netlist_instance *make_input_lut(struct flow_sc *sc, int sub)
{
	struct netlist_instance *lut;
	mpz_t contents;
	
	mpz_init_set_ui(contents, sub ? 0x9 : 0x6);
	lut = cs_create_lut(sc, 2, contents);
	mpz_clear(contents);
	return lut;
}

static void mkc_process(struct llhdl_node **n2, void *user)
{
	struct llhdl_node *n = *n2;
	struct flow_sc *sc = user;
	int sub;
	int n_bits_a, n_bits_b, n_bits;
	int i;
	struct mapkit_result *result;
	struct netlist_net *an, *bn, *xn, *mn, *rn;
	struct netlist_instance *lut2, *muxcy, *xorcy;
	
	if((n->type == LLHDL_NODE_EXTLOGIC)
	  && ((n->p.logic.op == LLHDL_EXTLOGIC_ADD) || (n->p.logic.op == LLHDL_EXTLOGIC_SUB))) {
		sub = n->p.logic.op == LLHDL_EXTLOGIC_SUB;
		
		n_bits_a = llhdl_get_vectorsize(n->p.logic.operands[0]);
		n_bits_b = llhdl_get_vectorsize(n->p.logic.operands[1]);
		n_bits = max(n_bits_a, n_bits_b);
		
		result = mapkit_create_result(2, n_bits_a+n_bits_b, n_bits+1);
		result->input_nodes[0] = &n->p.logic.operands[0];
		result->input_nodes[1] = &n->p.logic.operands[1];
		
		an = bn = NULL;
		mn = cs_constant_net(sc,sub);
		for(i=0;i<n_bits;i++) {
			if(i < n_bits_a) {
				an = netlist_m_create_net(sc->netlist);
				result->input_nets[i] = an;
			} else {
				if(!llhdl_get_sign(n->p.logic.operands[0]))
					an = cs_constant_net(sc, 0);
				/* otherwise, an keeps the MSB, which is what we want. */
			}
			if(i < n_bits_b) {
				bn = netlist_m_create_net(sc->netlist);
				result->input_nets[n_bits_a+i] = bn;
			} else {
				if(!llhdl_get_sign(n->p.logic.operands[1]))
					bn = cs_constant_net(sc, 0);
				/* otherwise, bn keeps the MSB, which is what we want. */
			}
			
			lut2 = make_input_lut(sc, sub);
			muxcy = netlist_m_instantiate(sc->netlist, &netlist_xilprims[NETLIST_XIL_MUXCY]);
			xorcy = netlist_m_instantiate(sc->netlist, &netlist_xilprims[NETLIST_XIL_XORCY]);
			
			netlist_add_branch(an, lut2, 0, NETLIST_XIL_LUT2_I0);
			netlist_add_branch(an, muxcy, 0, NETLIST_XIL_MUXCY_DI);
			
			netlist_add_branch(bn, lut2, 0, NETLIST_XIL_LUT2_I1);
			
			xn = netlist_m_create_net(sc->netlist);
			netlist_add_branch(xn, lut2, 1, NETLIST_XIL_LUT2_O);
			netlist_add_branch(xn, muxcy, 0, NETLIST_XIL_MUXCY_S);
			netlist_add_branch(xn, xorcy, 0, NETLIST_XIL_XORCY_LI);
			
			netlist_add_branch(mn, xorcy, 0, NETLIST_XIL_XORCY_CI);
			netlist_add_branch(mn, muxcy, 0, NETLIST_XIL_MUXCY_CI);
			mn = netlist_m_create_net(sc->netlist);
			netlist_add_branch(mn, muxcy, 1, NETLIST_XIL_MUXCY_O);
			
			rn = netlist_m_create_net(sc->netlist);
			netlist_add_branch(rn, xorcy, 1, NETLIST_XIL_XORCY_O);
			result->output_nets[i] = rn;
		}
		/* last bit of the result is carry out (addition) or ~carry out (subtraction) */
		if(sub) {
			xorcy = netlist_m_instantiate(sc->netlist, &netlist_xilprims[NETLIST_XIL_XORCY]);
			netlist_add_branch(cs_constant_net(sc, 1), xorcy, 0, NETLIST_XIL_XORCY_LI);
			netlist_add_branch(mn, xorcy, 0, NETLIST_XIL_XORCY_CI);
			rn = netlist_m_create_net(sc->netlist);
			netlist_add_branch(rn, xorcy, 1, NETLIST_XIL_XORCY_O);
		} else
			rn = mn;
		result->output_nets[n_bits] = rn;
		
		mapkit_consume(sc->mapkit, n, result);
	}
}

void carryarith_register(struct flow_sc *sc)
{
	mapkit_register_process(sc->mapkit, mkc_process, NULL, sc);
}

