#ifndef __NETLIST_MANAGER_H
#define __NETLIST_MANAGER_H

struct netlist_manager {
	unsigned int next_uid;		/* < next uid, incremented at each new net/instance */
	struct netlist_instance *ihead;	/* < instance list head */
	struct netlist_net *nhead;	/* < net list head */
};

struct netlist_manager *netlist_m_new();
void netlist_m_free(struct netlist_manager *m);

struct netlist_instance *netlist_m_instantiate(struct netlist_manager *m, struct netlist_primitive *p);
struct netlist_net *netlist_m_create_net(struct netlist_manager *m);
struct netlist_net *netlist_m_create_net_with_branch(struct netlist_manager *m, struct netlist_instance *inst, int output, int pin_index);

#endif /* __NETLIST_MANAGER_H */
