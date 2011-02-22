#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmp.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/io.h>
#include <netlist/xilprims.h>
#include <netlist/symbol.h>
#include <netlist/antares.h>
#include <netlist/edif.h>
#include <netlist/dot.h>

#include <llhdl/structure.h>
#include <llhdl/exchange.h>
#include <llhdl/tools.h>

#include <mapkit/mapkit.h>

#include <bd/bd.h>

#include "commonstruct.h"
#include "dsp.h"
#include "carryarith.h"
#include "srl16.h"
#include "lut.h"
#include "fd.h"
#include "flow.h"

static char *iosuffix(const char *base)
{
	int r;
	char *ret;
	r = asprintf(&ret, "%s$IO", base);
	if(r == -1) abort();
	return ret;
}

/* TODO: better naming of vectors, this one could cause conflicts */
static char *vecsuffix(const char *base, int i)
{
	int r;
	char *ret;
	r = asprintf(&ret, "%s_%d", base, i);
	if(r == -1) abort();
	return ret;
}

static void create_io(struct flow_sc *sc, struct llhdl_node *n, struct netlist_net *net, const char *name)
{
	int isout;
	int buf_type;
	int buf_port_fabric, buf_port_pin;
	char *ioname;
	struct netlist_sym *sym;
	struct netlist_instance *iobuf;
	struct netlist_net *ionet;
	struct netlist_primitive *ioprim;
	struct netlist_instance *ioport;

	if(sc->settings->io_buffers) {
		isout = n->p.signal.type == LLHDL_SIGNAL_PORT_OUT;
		if(llhdl_is_clock(n)) {
			assert(!isout);
			buf_type = NETLIST_XIL_BUFGP;
			buf_port_fabric = NETLIST_XIL_BUFGP_O;
			buf_port_pin = NETLIST_XIL_BUFGP_I;
		} else if(isout) {
			buf_type = NETLIST_XIL_OBUF;
			buf_port_fabric = NETLIST_XIL_OBUF_I;
			buf_port_pin = NETLIST_XIL_OBUF_O;
		} else {
			buf_type = NETLIST_XIL_IBUF;
			buf_port_fabric = NETLIST_XIL_IBUF_O;
			buf_port_pin = NETLIST_XIL_IBUF_I;
		}

		/* I/O and clock buffer insertion */
		iobuf = netlist_m_instantiate(sc->netlist, &netlist_xilprims[buf_type]);
		ionet = netlist_m_create_net(sc->netlist);
		ioname = iosuffix(name);
		sym = netlist_sym_add(sc->symbols, ionet->uid, 'N', ioname);
		sym->user = ionet;
		free(ioname);
		netlist_add_branch(ionet, iobuf, !isout, buf_port_fabric);
	}

	/* Create I/O port and connect to buffer */
	ioprim = netlist_create_io_primitive(sc->netlist_iop, 
		isout ? NETLIST_PRIMITIVE_PORT_OUT : NETLIST_PRIMITIVE_PORT_IN,
		name);
	ioport = netlist_m_instantiate(sc->netlist, ioprim);
	netlist_add_branch(net, ioport, !isout, 0);
	if(sc->settings->io_buffers)
		netlist_add_branch(net, iobuf, isout, buf_port_pin);
}

static void create_signal(struct flow_sc *sc, struct llhdl_node *n)
{
	struct netlist_net *net;
	struct netlist_sym *sym;
	int i;
	char *name;

	for(i=0;i<n->p.signal.vectorsize;i++) {
		if(n->p.signal.vectorsize == 1)
			name = n->p.signal.name;
		else
			name = vecsuffix(n->p.signal.name, i);
		net = netlist_m_create_net(sc->netlist);
		sym = netlist_sym_add(sc->symbols, net->uid, 'N', name);
		sym->user = net;
		if(n->p.signal.type != LLHDL_SIGNAL_INTERNAL)
			create_io(sc, n, net, name);
		if(n->p.signal.vectorsize != 1)
			free(name);
	}
}

static void create_signals(struct flow_sc *sc)
{
	struct llhdl_node *n;
	
	n = sc->module->head;
	while(n != NULL) {
		assert(n->type == LLHDL_NODE_SIGNAL);
		create_signal(sc, n);
		n = n->p.signal.next;
	}
}

static void *mkc_constant(int v, void *user)
{
	struct flow_sc *sc = user;
	return cs_constant_net(sc, v);
}

static void *mkc_signal(struct llhdl_node *n, int bit, void *user)
{
	struct flow_sc *sc = user;
	struct netlist_sym *sym;
	int is_vec;
	int is_io;
	char *vecname, *signame;

	assert(n->type == LLHDL_NODE_SIGNAL);
	is_vec = n->p.signal.vectorsize != 1;
	is_io = sc->settings->io_buffers && (n->p.signal.type != LLHDL_SIGNAL_INTERNAL);
	if(is_vec)
		vecname = vecsuffix(n->p.signal.name, bit);
	else
		vecname = n->p.signal.name;
	if(is_io)
		signame = iosuffix(vecname);
	else
		signame = vecname;
	sym = netlist_sym_lookup(sc->symbols, signame, 'N');
	if(is_io)
		free(signame);
	if(is_vec)
		free(vecname);
	assert(sym != NULL);

	return sym->user;
}

static void mkc_join(void *a, void *b, void *user)
{
	netlist_join(a, b);
}

void run_flow(struct flow_settings *settings)
{
	struct flow_sc sc;

	/* Initialize */
	sc.settings = settings;
	sc.module = llhdl_parse_file(settings->input_lhd);
	sc.netlist_iop = netlist_create_iop_manager();
	sc.netlist = netlist_m_new();
	sc.symbols = netlist_sym_newstore();
	sc.vcc_net = NULL;
	sc.gnd_net = NULL;
	sc.mapkit = mapkit_new(sc.module, mkc_constant, mkc_signal, mkc_join, &sc);
	
	/* Build the meta-mapper process stack */
	if(settings->dsp)
		dsp_register(&sc);
	if(settings->carry_chains)
		carryarith_register(&sc);
	if(settings->srl16)
		srl16_register(&sc);
	bd_register(sc.mapkit);
	lut_register(&sc);
	fd_register(&sc);
	
	/* Create netlist signals. I/O and clock buffers are also inserted here. */
	llhdl_identify_clocks(sc.module);
	create_signals(&sc);
	/* Run the meta-mapper */
	mapkit_metamap(sc.mapkit);
	mapkit_free(sc.mapkit);
	
	/* Prune netlist */
	if(settings->prune)
		netlist_m_prune(sc.netlist);
	
	/* Write output files */
	if(settings->output_anl != NULL)
		netlist_m_antares_file(sc.netlist, settings->output_anl, sc.module->name, settings->part);
	if(settings->output_edf != NULL) {
		struct edif_param edif_param;
		edif_param.flavor = EDIF_FLAVOR_XILINX;
		edif_param.design_name = sc.module->name;
		edif_param.cell_library = "UNISIMS";
		edif_param.part = settings->part;
		edif_param.manufacturer = "Xilinx";
		netlist_m_edif_file(sc.netlist, settings->output_edf, &edif_param);
	}
	if(settings->output_dot != NULL)
		netlist_m_dot_file(sc.netlist, settings->output_dot);
	if(settings->output_sym)
		netlist_sym_to_file(sc.symbols, settings->output_sym);
	
	/* Clean up */
	netlist_sym_freestore(sc.symbols);
	netlist_m_free(sc.netlist);
	netlist_free_iop_manager(sc.netlist_iop);
	llhdl_free_module(sc.module);
}
