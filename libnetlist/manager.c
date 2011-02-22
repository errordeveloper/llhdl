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

struct netlist_net *netlist_m_create_net_with_branch(struct netlist_manager *m, struct netlist_instance *inst, int output, int pin_index)
{
	struct netlist_net *net;
	
	net = netlist_m_create_net(m);
	netlist_add_branch(net, inst, output, pin_index);
	return net;
}

void netlist_m_delete_instance(struct netlist_manager *m, struct netlist_instance *inst)
{
	struct netlist_net *net;
	struct netlist_instance *prev;
	
	net = m->nhead;
	while(net != NULL) {
		netlist_disconnect_instance(net, inst);
		net = net->next;
	}
	if(m->ihead == inst)
		m->ihead = m->ihead->next;
	else {
		prev = m->ihead;
		while(prev->next != inst)
			prev = prev->next;
		prev->next = inst->next;
	}
	netlist_free_instance(inst);
}

static int is_driving(struct netlist_manager *m, struct netlist_instance *inst)
{
	struct netlist_net *net;
	struct netlist_branch *b;
	int net_is_driven;
	int net_has_others;
	
	net = m->nhead;
	while(net != NULL) {
		b = net->head;
		net_is_driven = 0;
		net_has_others = 0;
		while(b != NULL) {
			if((b->inst == inst) && (b->output)) {
				net_is_driven = 1;
				if(net_has_others)
					return 1;
			}
			if(b->inst != inst) {
				net_has_others = 1;
				if(net_is_driven)
					return 1;
			}
			b = b->next;
		}
		net = net->next;
	}
	return 0;
}

int netlist_m_prune_pass(struct netlist_manager *m)
{
	int count;
	struct netlist_instance *inst, *inst2;
	
	count = 0;
	inst = m->ihead;
	while(inst != NULL) {
		inst2 = inst->next;
		if(!inst->dont_touch && (inst->p->type == NETLIST_PRIMITIVE_INTERNAL)) {
			if(!is_driving(m, inst)) {
				netlist_m_delete_instance(m, inst);
				count++;
			}
		}
		inst = inst2;
	}
	return count;
}

void netlist_m_prune(struct netlist_manager *m)
{
	int count;
	
	do {
		count = netlist_m_prune_pass(m);
	} while(count > 0);
}
