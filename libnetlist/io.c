#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <netlist/net.h>
#include <netlist/io.h>

static char *empty_set[] = {NULL};

struct netlist_primitive *netlist_create_io_primitive(int type, const char *name)
{
	struct netlist_primitive *p;
	char **name_list;

	p = malloc(sizeof(struct netlist_primitive));
	assert(p != NULL);

	p->type = type;
	p->name = type == NETLIST_PRIMITIVE_PORT_IN ? "INPUT_PORT" : "OUTPUT_PORT";
	p->attribute_names = empty_set;
	p->default_attributes = empty_set;

	name_list = malloc(sizeof(void *));
	assert(name_list != NULL);
	*name_list = strdup(name);
	assert(*name_list != NULL);

	switch(type) {
		case NETLIST_PRIMITIVE_PORT_IN:
			p->inputs = 0;
			p->input_names = empty_set;
			p->outputs = 1;
			p->output_names = name_list;
			break;
		case NETLIST_PRIMITIVE_PORT_OUT:
			p->inputs = 1;
			p->input_names = name_list;
			p->outputs = 0,
			p->output_names = empty_set;
			break;
		default:
			assert(0);
			break;
	}
	
	return p;
}

void netlist_free_io_primitive(struct netlist_primitive *p)
{
	if(p->inputs > 0) {
		free(*p->input_names);
		free(p->input_names);
	}
	if(p->outputs > 0) {
		free(*p->output_names);
		free(p->output_names);
	}
	free(p);
}
