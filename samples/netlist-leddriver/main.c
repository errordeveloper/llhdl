#include <stdio.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/edif.h>
#include <netlist/io.h>
#include <netlist/xilprims.h>
#include <netlist/symbol.h>

static struct edif_param edif_param = {
	.flavor = EDIF_FLAVOR_XILINX,
	.design_name = "leddriver",
	.cell_library = "UNISIMS",
	.part = "xc6slx45-fgg484-2",
	.manufacturer = "Xilinx"
};

int main(int argc, char *argv[])
{
	struct netlist_primitive *my_in, *my_out;
	struct netlist_manager *m;
	struct netlist_instance *ibuf, *obuf;
	struct netlist_instance *inp, *outp;
	struct netlist_sym_store *symbols;
	struct netlist_net *net_btn1, *net_internal, *net_led1;

	my_in = netlist_create_io_primitive(NETLIST_PRIMITIVE_PORT_IN, "btn1");
	my_out = netlist_create_io_primitive(NETLIST_PRIMITIVE_PORT_OUT, "led1");
	
	m = netlist_m_new();
	symbols = netlist_sym_newstore();

	inp = netlist_m_instantiate(m, my_in);
	outp = netlist_m_instantiate(m, my_out);
	
	ibuf = netlist_m_instantiate(m, &netlist_xilprims[NETLIST_XIL_IBUF]);
	obuf = netlist_m_instantiate(m, &netlist_xilprims[NETLIST_XIL_OBUF]);

	net_btn1 = netlist_m_create_net(m);
	netlist_sym_add(symbols, net_btn1->uid, 'N', "btn1");
	net_internal = netlist_m_create_net(m);
	net_led1 = netlist_m_create_net(m);
	netlist_sym_add(symbols, net_led1->uid, 'N', "led1");

	/* input pin->ibuf input */
	netlist_add_branch(net_btn1, inp, 1, 0);
	netlist_add_branch(net_btn1, ibuf, 0, NETLIST_XIL_IBUF_I);

	/* ibuf output->obuf input */
	netlist_add_branch(net_internal, ibuf, 1, NETLIST_XIL_IBUF_O);
	netlist_add_branch(net_internal, obuf, 0, NETLIST_XIL_OBUF_I);

	/* obuf output->output pin */
	netlist_add_branch(net_led1, obuf, 1, NETLIST_XIL_OBUF_O);
	netlist_add_branch(net_led1, outp, 0, 0);
	
	netlist_m_edif_file(m, "leddriver.edf", &edif_param);
	netlist_sym_to_file(symbols, "leddriver.sym");
	
	netlist_m_free(m);
	netlist_sym_freestore(symbols);

	netlist_free_io_primitive(my_in);
	netlist_free_io_primitive(my_out);

	return 0;
}
