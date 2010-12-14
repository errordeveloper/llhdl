#include <stdio.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/edif.h>
#include <netlist/io.h>
#include <netlist/xilprims.h>

static struct edif_param edif_param = {
	.flavor = EDIF_FLAVOR_XILINX,
	.design_name = "ledblinker",
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

	my_in = netlist_create_io_primitive(NETLIST_PRIMITIVE_PORT_IN, "btn1");
	my_out = netlist_create_io_primitive(NETLIST_PRIMITIVE_PORT_OUT, "led1");
	
	m = netlist_m_new();

	inp = netlist_m_instantiate(m, my_in);
	outp = netlist_m_instantiate(m, my_out);
	
	ibuf = netlist_m_instantiate(m, &netlist_xilprims[NETLIST_XIL_IBUF]);
	obuf = netlist_m_instantiate(m, &netlist_xilprims[NETLIST_XIL_OBUF]);

	netlist_m_connect(m, inp, 0, ibuf, NETLIST_XIL_IBUF_I);
	netlist_m_connect(m, ibuf, NETLIST_XIL_IBUF_I, obuf, NETLIST_XIL_IBUF_O);
	netlist_m_connect(m, obuf, NETLIST_XIL_OBUF_O, outp, 0);
	
	netlist_m_edif_fd(m, stdout, &edif_param);
	
	netlist_m_free(m);

	netlist_free_io_primitive(my_in);
	netlist_free_io_primitive(my_out);

	return 0;
}
