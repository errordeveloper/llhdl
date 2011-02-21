#include <assert.h>
#include <gmp.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/xilprims.h>

#include "flow.h"
#include "commonstruct.h"

struct netlist_net *cs_constant_net(struct flow_sc *sc, int v)
{
	struct netlist_instance *inst;

	if(v) {
		if(sc->vcc_net == NULL) {
			inst = netlist_m_instantiate(sc->netlist, &netlist_xilprims[NETLIST_XIL_VCC]);
			sc->vcc_net = netlist_m_create_net(sc->netlist);
			netlist_add_branch(sc->vcc_net, inst, 1, NETLIST_XIL_VCC_P);
		}
		return sc->vcc_net;
	} else {
		if(sc->gnd_net == NULL) {
			inst = netlist_m_instantiate(sc->netlist, &netlist_xilprims[NETLIST_XIL_GND]);
			sc->gnd_net = netlist_m_create_net(sc->netlist);
			netlist_add_branch(sc->gnd_net, inst, 1, NETLIST_XIL_GND_G);
		}
		return sc->gnd_net;
	}
}

struct netlist_instance *cs_create_lut(struct flow_sc *sc, int inputs, mpz_t contents)
{
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
			gmp_sprintf(val, "%016Zx", contents);
			break;
	}
	inst = netlist_m_instantiate(sc->netlist, &netlist_xilprims[primitive_type]);
	netlist_set_attribute(inst, "INIT", val);
	return inst;
}
