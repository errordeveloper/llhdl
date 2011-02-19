#ifndef __FLOW_H
#define __FLOW_H

#include <llhdl/structure.h>
#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/io.h>
#include <netlist/symbol.h>
#include <mapkit/mapkit.h>

struct flow_sc {
	struct llhdl_module *module;
	struct netlist_iop_manager *netlist_iop;
	struct netlist_manager *netlist;
	struct netlist_sym_store *symbols;
	struct netlist_net *vcc_net;
	struct netlist_net *gnd_net;
	struct mapkit_sc *mapkit;
};

void run_flow(const char *input_lhd, const char *output_edf, const char *output_sym, const char *part, int lutmapper, void *lutmapper_extra_param);

#endif /* __FLOW_H */

