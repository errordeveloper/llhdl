#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/edif.h>
#include <netlist/io.h>
#include <netlist/xilprims.h>
#include <netlist/symbol.h>

#include <llhdl/structure.h>
#include <llhdl/exchange.h>

static int count_lut_inputs(struct llhdl_node *n)
{
	switch(n->type) {
		case LLHDL_NODE_BOOLEAN:
			return 0;
		case LLHDL_NODE_SIGNAL:
			assert(n->p.signal.vectorsize == 1);
			return 1;
		case LLHDL_NODE_MUX:
			return 1 + count_lut_inputs(n->p.mux.negative) + count_lut_inputs(n->p.mux.positive);
		default:
			fprintf(stderr, "simple-mapper is a simple application that does not support nodes of type %d.\n", n->type);
			exit(EXIT_FAILURE);
			return 0;
	}
}

typedef unsigned long long int lut_t;

static struct netlist_instance *create_lut(struct netlist_manager *m, unsigned int inputs, lut_t contents)
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
			sprintf(val, "%llx", contents);
			break;
		case 2:
			primitive_type = NETLIST_XIL_LUT2;
			sprintf(val, "%llx", contents);
			break;
		case 3:
			primitive_type = NETLIST_XIL_LUT3;
			sprintf(val, "%02llx", contents);
			break;
		case 4:
			primitive_type = NETLIST_XIL_LUT4;
			sprintf(val, "%04llx", contents);
			break;
		case 5:
			primitive_type = NETLIST_XIL_LUT5;
			sprintf(val, "%08llx", contents);
			break;
		case 6:
			primitive_type = NETLIST_XIL_LUT6;
			sprintf(val, "%16llx", contents);
			break;
	}
	inst = netlist_m_instantiate(m, &netlist_xilprims[primitive_type]);
	netlist_set_attribute(inst, "INIT", val);
	return inst;
}

static void lut_on(lut_t *lut, int value)
{
	*lut |= (1ULL << (lut_t)value);
}

/*
 * mux2 inputs:
 * I0: select
 * I1: negative
 * I2: positive
 *
 * I0 I1 I2
 *  0  0  0   0
 *  0  0  1   0
 *  0  1  0   1
 *  0  1  1   1
 *  1  0  0   0
 *  1  0  1   1
 *  1  1  0   0
 *  1  1  1   1
 */

static struct netlist_instance *create_mux2(struct netlist_manager *m)
{
	lut_t contents;

	contents = 0ULL;
	lut_on(&contents, 2);
	lut_on(&contents, 3);
	lut_on(&contents, 5);
	lut_on(&contents, 7);
	return create_lut(m, 3, contents);
}

static int eval_expr(struct llhdl_node *n, unsigned int *values)
{
	int sel;
	int neg, pos;

	switch(n->type) {
		case LLHDL_NODE_BOOLEAN:
			return n->p.boolean.value;
		case LLHDL_NODE_SIGNAL:
			assert(n->p.signal.vectorsize == 1);
			return *values & 1;
		case LLHDL_NODE_MUX:
			sel = *values & 1;
			*values >>= 1;
			neg = eval_expr(n->p.mux.negative, values);
			pos = eval_expr(n->p.mux.positive, values);
			if(sel)
				return pos;
			else
				return neg;
		default:
			assert(0);
			return 0;
	}
}

static struct llhdl_node *get_ith_signal(struct llhdl_node *n, unsigned int *i)
{
	struct llhdl_node *t;

	switch(n->type) {
		case LLHDL_NODE_BOOLEAN:
			return NULL;
		case LLHDL_NODE_SIGNAL:
			if((*i)-- == 0)
				return n;
			else
				return NULL;
		case LLHDL_NODE_MUX:
			t = get_ith_signal(n->p.mux.sel, i);
			if(t != NULL) return t;
			t = get_ith_signal(n->p.mux.negative, i);
			if(t != NULL) return t;
			t = get_ith_signal(n->p.mux.positive, i);
			if(t != NULL) return t;
			return NULL;
		default:
			assert(0);
			return NULL;
	}
}

static char *iosuffix(const char *base)
{
	int r;
	char *ret;
	r = asprintf(&ret, "%s_IO", base);
	assert(r != -1);
	return ret;
}

struct sm_objects {
	struct llhdl_module *module;
	struct netlist_manager *netlist;
	struct netlist_sym_store *symbols;
	struct netlist_instance *vcc;
	struct netlist_instance *gnd;
};

static struct netlist_net *resolve_signal(struct sm_objects *obj, struct llhdl_node *n)
{
	struct netlist_sym *sym;
	int is_io;
	char *signame;

	assert(n->type == LLHDL_NODE_SIGNAL);
	is_io = n->p.signal.type != LLHDL_SIGNAL_INTERNAL;
	if(is_io)
		signame = iosuffix(n->p.signal.name);
	else
		signame = n->p.signal.name;
	sym = netlist_sym_lookup(obj->symbols, signame, 'N');
	if(is_io)
		free(signame);
	assert(sym != NULL);
	return sym->user;
}

static struct netlist_instance *map_expr(struct sm_objects *obj, struct llhdl_node *n)
{
	int ni;

	if(n->type == LLHDL_NODE_BOOLEAN) {
		if(n->p.boolean.value)
			return obj->vcc;
		else
			return obj->gnd;
	}
	
	ni = count_lut_inputs(n);
	if(ni > 6) {
		struct netlist_instance *neg_map, *pos_map;
		struct netlist_instance *mux2;
		struct netlist_net *mux2_nets[3];
		
		assert(n->type == LLHDL_NODE_MUX);
		neg_map = map_expr(obj, n->p.mux.negative);
		pos_map = map_expr(obj, n->p.mux.positive);
		mux2 = create_mux2(obj->netlist);

		mux2_nets[0] = resolve_signal(obj, n->p.mux.sel);
		mux2_nets[1] = netlist_m_create_net(obj->netlist);
		netlist_add_branch(mux2_nets[1], neg_map, 1, 0);
		mux2_nets[2] = netlist_m_create_net(obj->netlist);
		netlist_add_branch(mux2_nets[2], pos_map, 1, 0);

		netlist_add_branch(mux2_nets[0], mux2, 0, 0);
		netlist_add_branch(mux2_nets[1], mux2, 0, 1);
		netlist_add_branch(mux2_nets[2], mux2, 0, 2);
		return mux2;
	} else {
		lut_t l;
		unsigned int comb;
		unsigned int i;
		struct netlist_instance *inst;
		struct llhdl_node *signal;

		l = 0ULL;
		comb = (1 << ni);
		for(i=0;i<comb;i++) {
			unsigned int ic = i;
			if(eval_expr(n, &ic))
				lut_on(&l, i);
		}
		inst = create_lut(obj->netlist, ni, l);

		for(i=0;i<ni;i++) {
			unsigned int ic = i;
			signal = get_ith_signal(n, &ic);
			netlist_add_branch(resolve_signal(obj, signal), inst, 0, i);
		}
		
		return inst;
	}
}

static struct edif_param edif_param = {
	.flavor = EDIF_FLAVOR_XILINX,
	.design_name = NULL,
	.cell_library = "UNISIMS",
	.part = "xc6slx45-fgg484-2",
	.manufacturer = "Xilinx"
};

int main(int argc, char *argv[])
{
	struct sm_objects obj;
	struct llhdl_node *n;
	struct netlist_net *net;
	struct netlist_sym *sym;
	
	if(argc != 2) { // FIXME
		fprintf(stderr, "Usage: simple-mapper <input.lhd> <output.edf> <output.sym>\n");
		return 1;
	}

	/* Initialize */
	obj.module = llhdl_parse_file(argv[1]);
	edif_param.design_name = obj.module->name;
	obj.netlist = netlist_m_new();

	obj.vcc = netlist_m_instantiate(obj.netlist, &netlist_xilprims[NETLIST_XIL_VCC]);
	obj.gnd = netlist_m_instantiate(obj.netlist, &netlist_xilprims[NETLIST_XIL_GND]);

	/* Create signals */
	n = obj.module->head;
	while(n != NULL) {
		assert(n->type == LLHDL_NODE_SIGNAL);
		net = netlist_m_create_net(obj.netlist);
		sym = netlist_sym_add(obj.symbols, net->uid, 'N', n->p.signal.name);
		sym->user = net;
		if(n->p.signal.type != LLHDL_SIGNAL_INTERNAL) {
			int isout;
			char *ioname;
			struct netlist_instance *iobuf;
			struct netlist_net *ionet;
			struct netlist_primitive *ioprim;
			struct netlist_instance *ioport;

			isout = n->p.signal.type == LLHDL_SIGNAL_PORT_OUT;

			/* I/O buffer insertion */
			iobuf = netlist_m_instantiate(obj.netlist,
				&netlist_xilprims[isout ? NETLIST_XIL_OBUF : NETLIST_XIL_IBUF]);
			ionet = netlist_m_create_net(obj.netlist);
			ioname = iosuffix(n->p.signal.name);
			sym = netlist_sym_add(obj.symbols, ionet->uid, 'N', ioname);
			sym->user = ionet;
			free(ioname);
			netlist_add_branch(ionet, iobuf, !isout, isout ? NETLIST_XIL_OBUF_I : NETLIST_XIL_IBUF_O);

			/* Create I/O port and connect to buffer */
			ioprim = netlist_create_io_primitive(isout ? NETLIST_PRIMITIVE_PORT_OUT : NETLIST_PRIMITIVE_PORT_IN, n->p.signal.name);
			// FIXME: ioprim is not freed
			ioport = netlist_m_instantiate(obj.netlist, ioprim);
			netlist_add_branch(net, ioport, !isout, 0);
			netlist_add_branch(net, iobuf, isout, isout ? NETLIST_XIL_OBUF_O : NETLIST_XIL_IBUF_I);
		}
		n = n->p.signal.next;
	}

	/* Create logic */
	n = obj.module->head;
	while(n != NULL) {
		assert(n->type == LLHDL_NODE_SIGNAL);
		if(n->p.signal.source != NULL)
			map_expr(&obj, n->p.signal.source);
		n = n->p.signal.next;
	}

	/* Write output files */
	netlist_m_edif_fd(obj.netlist, stdout, &edif_param);

	/* Clean up */
	netlist_m_free(obj.netlist);
	llhdl_free_module(obj.module);

	return 0;
}
