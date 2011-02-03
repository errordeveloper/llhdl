#include <stdlib.h>
#include <string.h>
#include <util.h>

#include <netlist/net.h>
#include <netlist/manager.h>

struct netlist_manager *netlist_m_new()
{
	struct netlist_manager *m;

	m = alloc_type(struct netlist_manager);
	m->next_uid = 0;
	m->ihead = NULL;
	m->nhead = NULL;
	return m;
}

void netlist_m_free(struct netlist_manager *m)
{
	struct netlist_instance *i1, *i2;
	struct netlist_net *n1, *n2;

	i1 = m->ihead;
	while(i1 != NULL) {
		i2 = i1->next;
		netlist_free_instance(i1);
		i1 = i2;
	}
	n1 = m->nhead;
	while(n1 != NULL) {
		n2 = n1->next;
		netlist_free_net(n1);
		n1 = n2;
	}
	free(m);
}

struct netlist_instance *netlist_m_instantiate(struct netlist_manager *m, struct netlist_primitive *p)
{
	struct netlist_instance *inst;

	inst = netlist_instantiate(m->next_uid++, p);
	inst->next = m->ihead;
	m->ihead = inst;
	return inst;
}

struct netlist_net *netlist_m_create_net(struct netlist_manager *m)
{
	struct netlist_net *net;

	net = netlist_create_net(m->next_uid++);
	net->next = m->nhead;
	m->nhead = net;
	return net;
}
