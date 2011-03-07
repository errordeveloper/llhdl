#ifndef __FLOW_H
#define __FLOW_H

#include <llhdl/structure.h>
#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/io.h>
#include <netlist/symbol.h>
#include <mapkit/mapkit.h>

struct flow_settings {
	char *input_lhd;
	
	char *part;

	int io_buffers;
	int dsp;
	int carry_arith;
	int srl;
	int dedicated_muxes;
	int prune;
	
	int lut_mapper;
	int lut_max_inputs;
	void *lutmapper_extra_param;

	char *output_anl;
	char *output_edf;
	char *output_dot;
	char *output_sym;
};

struct flow_sc {
	struct flow_settings *settings;
	
	struct llhdl_module *module;
	struct netlist_iop_manager *netlist_iop;
	struct netlist_manager *netlist;
	struct netlist_sym_store *symbols;
	struct netlist_net *vcc_net;
	struct netlist_net *gnd_net;
	struct mapkit_sc *mapkit;
};

void run_flow(struct flow_settings *settings);

#endif /* __FLOW_H */

