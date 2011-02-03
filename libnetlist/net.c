#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include <netlist/net.h>

struct netlist_instance *netlist_instantiate(unsigned int uid, struct netlist_primitive *p)
{
	struct netlist_instance *new;
	int i;

	new = alloc_type(struct netlist_instance);
	
	new->uid = uid;
	new->p = p;

	if(p->attribute_count > 0) {
		new->attributes = alloc_size(p->attribute_count*sizeof(void *));
		for(i=0;i<p->attribute_count;i++)
			new->attributes[i] = stralloc(p->default_attributes[i]);
	} else
		new->attributes = NULL;

	new->next = NULL;

	return new;
}

void netlist_free_instance(struct netlist_instance *inst)
{
	int i;

	for(i=0;i<inst->p->attribute_count;i++)
		free(inst->attributes[i]);
	free(inst->attributes);
	
	free(inst);
}

static int find_attribute(struct netlist_instance *inst, const char *attr)
{
	int i;

	for(i=0;i<inst->p->attribute_count;i++)
		if(strcmp(inst->p->attribute_names[i], attr) == 0) return i;
	return -1;
}

void netlist_set_attribute(struct netlist_instance *inst, const char *attr, const char *value)
{
	int a;

	a = find_attribute(inst, attr);
	assert(a != -1);
	
	free(inst->attributes[a]);
	if(value == NULL)
		inst->attributes[a] = stralloc(inst->p->default_attributes[a]);
	else
		inst->attributes[a] = stralloc(value);
}

struct netlist_net *netlist_create_net(unsigned int uid)
{
	struct netlist_net *net;

	net = alloc_type(struct netlist_net);
	net->uid = uid;
	net->head = NULL;
	net->next = NULL;
	return net;
}

void netlist_add_branch(struct netlist_net *net, struct netlist_instance *inst, int output, int pin_index)
{
	struct netlist_branch *branch;

	if(output)
		assert(pin_index < inst->p->outputs);
	else
		assert(pin_index < inst->p->inputs);
	
	branch = alloc_type(struct netlist_branch);
	branch->inst = inst;
	branch->output = output;
	branch->pin_index = pin_index;
	branch->next = net->head;
	net->head = branch;
}

void netlist_free_net(struct netlist_net *net)
{
	struct netlist_branch *b1, *b2;

	b1 = net->head;
	while(b1 != NULL) {
		b2 = b1->next;
		free(b1);
		b1 = b2;
	}
	free(net);
}
