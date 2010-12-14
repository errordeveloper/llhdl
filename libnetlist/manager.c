#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <netlist/net.h>
#include <netlist/manager.h>

struct netlist_manager *netlist_m_new()
{
	struct netlist_manager *m;

	m = malloc(sizeof(struct netlist_manager));
	assert(m != NULL);
	m->next_uid = 0;
	m->head = NULL;
	return m;
}

void netlist_m_free(struct netlist_manager *m)
{
	struct netlist_instance *i1, *i2;

	i1 = m->head;
	while(i1 != NULL) {
		i2 = i1->next;
		netlist_free_instance(i1);
		i1 = i2;
	}
	free(m);
}

struct netlist_instance *netlist_m_instantiate(struct netlist_manager *m, struct netlist_primitive *p)
{
	struct netlist_instance *inst;

	inst = netlist_instantiate(m->next_uid++, p);
	inst->next = m->head;
	m->head = inst;
	return inst;
}

void netlist_m_free_instance(struct netlist_manager *m, struct netlist_instance *inst, int break_connections)
{
	netlist_free_instance(inst);
	if(break_connections) {
		/* TODO: break_connections */
	}
}

void netlist_m_connect(struct netlist_manager *m, struct netlist_instance *src, int output, struct netlist_instance *dest, int input)
{
	netlist_connect(m->next_uid++, src, output, dest, input);
}

void netlist_m_disconnect(struct netlist_manager *m, struct netlist_instance *src, int output, struct netlist_instance *dest, int input)
{
	netlist_disconnect(src, output, dest, input);
}
