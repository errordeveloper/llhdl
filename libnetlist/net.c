#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <netlist/net.h>

struct netlist_instance *netlist_instantiate(unsigned int uid, struct netlist_primitive *p)
{
	struct netlist_instance *new;
	int i;

	new = malloc(sizeof(struct netlist_instance));
	assert(new != NULL);
	
	new->uid = uid;
	new->p = p;

	if(p->attribute_count > 0) {
		new->attributes = malloc(p->attribute_count*sizeof(void *));
		assert(new->attributes);
		for(i=0;i<p->attribute_count;i++) {
			new->attributes[i] = strdup(p->default_attributes[i]);
			assert(new->attributes[i] != NULL);
		}
	} else
		new->attributes = NULL;

	if(p->outputs > 0) {
		new->outputs = malloc(p->outputs*sizeof(void *));
		assert(new->outputs != NULL);
		memset(new->outputs, 0, p->outputs*sizeof(void *));
	} else
		new->outputs = NULL;

	new->next = NULL;

	return new;
}

void netlist_free_instance(struct netlist_instance *inst)
{
	int i;
	struct netlist_net *n1, *n2;

	for(i=0;i<inst->p->attribute_count;i++)
		free(inst->attributes[i]);
	free(inst->attributes);

	for(i=0;i<inst->p->outputs;i++) {
		n1 = inst->outputs[i];
		while(n1 != NULL) {
			n2 = n1->next;
			free(n1);
			n1 = n2;
		}
	}
	free(inst->outputs);
	
	free(inst);
}

void netlist_connect(unsigned int uid, struct netlist_instance *src, int output, struct netlist_instance *dest, int input)
{
	struct netlist_net *n;

	n = malloc(sizeof(struct netlist_net));
	assert(n != NULL);
	n->uid = uid;
	n->dest = dest;
	n->input = input;
	n->next = src->outputs[output];
	src->outputs[output] = n;
}
