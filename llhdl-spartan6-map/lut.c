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
	int primitive_type;
	char val[17];
	struct netlist_instance *inst;

	assert(inputs > 0);
	assert(inputs < 7);
	primitive_type = 0;
	switch(inputs) {
		case 1:
			primitive_type = NETLIST_XIL_LUT1;
			gmp_sprintf(val, "%Zx", contents);
			break;
		case 2:
			primitive_type = NETLIST_XIL_LUT2;
			gmp_sprintf(val, "%Zx", contents);
			break;
		case 3:
			primitive_type = NETLIST_XIL_LUT3;
			gmp_sprintf(val, "%02Zx", contents);
			break;
		case 4:
			primitive_type = NETLIST_XIL_LUT4;
			gmp_sprintf(val, "%04Zx", contents);
			break;
		case 5:
			primitive_type = NETLIST_XIL_LUT5;
			gmp_sprintf(val, "%08Zx", contents);
			break;
		case 6:
			primitive_type = NETLIST_XIL_LUT6;
			gmp_sprintf(val, "%16Zx", contents);
			break;
	}
	inst = netlist_m_instantiate(sc->netlist, &netlist_xilprims[primitive_type]);
	netlist_set_attribute(inst, "INIT", val);
	return inst;
}

static void *tc_create_mux(int muxlevel, void *user)
{
	//struct flow_sc *sc = user;
	/* TODO: support MUXF7 and MUXF8 */
	return NULL;
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

