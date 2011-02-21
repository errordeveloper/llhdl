#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmp.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/edif.h>
#include <netlist/io.h>
#include <netlist/xilprims.h>
#include <netlist/symbol.h>

#include <tilm/tilm.h>

#include <mapkit/mapkit.h>

#include "flow.h"
#include "commonstruct.h"
#include "lut.h"

static void *tc_create_net(void *user)
{
	struct flow_sc *sc = user;
	return netlist_m_create_net(sc->netlist);
}

static void tc_branch(void *net, void *a, int output, int an, void *user)
{
	return netlist_add_branch(net, a, output, an);
}

static void *tc_create_lut(int inputs, mpz_t contents, void *user)
{
	struct flow_sc *sc = user;
	return cs_create_lut(sc, inputs, contents);
}

static void *tc_create_mux(int muxlevel, void *user)
{
	struct flow_sc *sc = user;
	int type;

	switch(muxlevel) {
		case 0: type = NETLIST_XIL_MUXF7;
		case 1: type = NETLIST_XIL_MUXF8;
		default: return NULL;
	}
	return netlist_m_instantiate(sc->netlist, &netlist_xilprims[type]);
}

void lut_register(struct flow_sc *sc)
{
	tilm_register(sc->mapkit,
		sc->settings->lut_mapper,
		sc->settings->lut_max_inputs,
		sc->settings->lutmapper_extra_param,
		tc_create_net,
		tc_branch,
		tc_create_lut,
		sc->settings->dedicated_muxes ? tc_create_mux : NULL,
		sc);
}
