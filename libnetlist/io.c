#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include <netlist/net.h>
#include <netlist/io.h>

static struct netlist_primitive *create_io_primitive(int type, const char *name)
{
	struct netlist_primitive *p;
	char **name_list;

	p = alloc_type(struct netlist_primitive);

	p->type = type;
	p->name = type == NETLIST_PRIMITIVE_PORT_IN ? "INPUT_PORT" : "OUTPUT_PORT";
	p->attribute_count = 0;
	p->attribute_names = NULL;
	p->default_attributes = NULL;

	name_list = alloc_type(char *);
	*name_list = stralloc(name);

	switch(type) {
		case NETLIST_PRIMITIVE_PORT_IN:
			p->inputs = 0;
			p->input_names = NULL;
			p->outputs = 1;
			p->output_names = name_list;
			break;
		case NETLIST_PRIMITIVE_PORT_OUT:
			p->inputs = 1;
			p->input_names = name_list;
			p->outputs = 0,
			p->output_names = NULL;
			break;
		default:
			assert(0);
			break;
	}
	
	return p;
}

static void free_io_primitive(struct netlist_primitive *p)
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

struct netlist_iop_manager *netlist_create_iop_manager()
{
	struct netlist_iop_manager *m;

	m = alloc_type(struct netlist_iop_manager);
	m->head = NULL;
	return m;
}

void netlist_free_iop_manager(struct netlist_iop_manager *m)
{
	struct netlist_iop_element *e1, *e2;

	e1 = m->head;
	while(e1 != NULL) {
		free_io_primitive(e1->p);
		e2 = e1->next;
		free(e1);
		e1 = e2;
	}
	free(m);
}

struct netlist_primitive *netlist_create_io_primitive(struct netlist_iop_manager *m, int type, const char *name)
{
	struct netlist_primitive *p;
	struct netlist_iop_element *e;

	p = create_io_primitive(type, name);
	e = alloc_type(struct netlist_iop_element);
	e->p = p;
	e->next = m->head;
	m->head = e;
	return p;
}
