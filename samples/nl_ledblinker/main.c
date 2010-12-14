#include <stdio.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/edif.h>

static char *attribute_names[] = {"MYATTR", NULL};
static char *attrs[] = {"42", NULL};
static char *input_names[] = {"I", NULL};
static char *output_names[] = {"O", NULL};

static struct netlist_primitive test = {
	.name = "TEST",
	.attribute_names = attribute_names,
	.default_attributes = attrs,
	.inputs = 1,
	.input_names = input_names,
	.outputs = 1,
	.output_names = output_names
};

static struct edif_param edif_param = {
	.flavor = EDIF_FLAVOR_XILINX,
	.design_name = "ledblinker",
	.cell_library = "UNISIMS",
	.part = "xc6slx45-fgg484-2",
	.manufacturer = "Xilinx"
};

int main(int argc, char *argv[])
{
	struct netlist_manager *m;
	struct netlist_instance *i1, *i2;

	m = netlist_m_new();
	i1 = netlist_m_instantiate(m, &test);
	i2 = netlist_m_instantiate(m, &test);
	netlist_m_connect(m, i1, 0, i2, 0);
	netlist_m_edif_fd(m, stdout, &edif_param);
	netlist_m_free(m);

	return 0;
}
