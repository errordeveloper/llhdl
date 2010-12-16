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
	printf(" -> mapped to LUT%d (%s)\n", inputs, val);
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

static int eval_expr(struct llhdl_node *n, unsigned int values)
{
	switch(n->type) {
		case LLHDL_NODE_BOOLEAN:
			return n->p.boolean.value;
		case LLHDL_NODE_SIGNAL:
			assert(n->p.signal.vectorsize == 1);
			return values & 1;
		case LLHDL_NODE_MUX:
			if(values & 1)
				return eval_expr(n->p.mux.positive, values >> 1);
			else
				return eval_expr(n->p.mux.negative, values >> 1);
		default:
			assert(0);
			return 0;
	}
}

static struct netlist_instance *vcc, *gnd;

static struct netlist_instance *map_expr(struct netlist_manager *m, struct llhdl_node *n)
{
	int ni;

	if(n->type == LLHDL_NODE_BOOLEAN) {
		if(n->p.boolean.value)
			return vcc;
		else
			return gnd;
	}
	
	ni = count_lut_inputs(n);
	if(ni > 6) {
		struct netlist_instance *neg_map, *pos_map;
		struct netlist_instance *mux2;
		
		assert(n->type == LLHDL_NODE_MUX);
		neg_map = map_expr(m, n->p.mux.negative);
		pos_map = map_expr(m, n->p.mux.positive);
		mux2 = create_mux2(m);
		// TODO: netlist_m_connect(m, "n->p.mux.sel", 0, mux2, 0);
		netlist_m_connect(m, neg_map, 0, mux2, 1);
		netlist_m_connect(m, pos_map, 0, mux2, 2);
		return mux2;
	} else {
		lut_t l;
		unsigned int comb;
		unsigned int i;
		struct netlist_instance *inst;

		l = 0ULL;
		comb = (1 << ni);
		for(i=0;i<comb;i++)
			if(eval_expr(n, i))
				lut_on(&l, i);
		inst = create_lut(m, ni, l);
		// TODO: connect signals
		return inst;
	}
}

static struct edif_param edif_param = {
	.flavor = EDIF_FLAVOR_XILINX,
	.design_name = "ledblinker",
	.cell_library = "UNISIMS",
	.part = "xc6slx45-fgg484-2",
	.manufacturer = "Xilinx"
};

int main(int argc, char *argv[])
{
	struct llhdl_module *module;
	struct netlist_manager *netlist;
	struct llhdl_node *n;
	
	if(argc != 2) { // FIXME
		fprintf(stderr, "Usage: simple-mapper <input.lhd> <output.edf> <output.sym>\n");
		return 1;
	}
	
	module = llhdl_parse_file(argv[1]);
	netlist = netlist_m_new();

	vcc = netlist_m_instantiate(netlist, &netlist_xilprims[NETLIST_XIL_VCC]);
	gnd = netlist_m_instantiate(netlist, &netlist_xilprims[NETLIST_XIL_GND]);

	n = module->head;
	while(n != NULL) {
		assert(n->type == LLHDL_NODE_SIGNAL);
		if(n->p.signal.source != NULL) {
			printf("%s: %d\n", n->p.signal.name, count_lut_inputs(n->p.signal.source));
			map_expr(netlist, n->p.signal.source);
		}
		n = n->p.signal.next;
	}

	netlist_m_edif_fd(netlist, stdout, &edif_param);

	netlist_m_free(netlist);
	llhdl_free_module(module);

	return 0;
}
