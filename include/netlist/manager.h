#ifndef __NETLIST_MANAGER_H
#define __NETLIST_MANAGER_H

struct netlist_manager {
	unsigned int next_uid;		/* < next uid, incremented at each new net/instance */
	struct netlist_instance *head;	/* < instance list head */
};

struct netlist_manager *netlist_m_new();
void netlist_m_free(struct netlist_manager *m);

struct netlist_instance *netlist_m_instantiate(struct netlist_manager *m, struct netlist_primitive *p);
void netlist_m_free_instance(struct netlist_manager *m, struct netlist_instance *inst);
unsigned int netlist_m_connect(struct netlist_manager *m, struct netlist_instance *src, int output, struct netlist_instance *dest, int input);

#endif /* __NETLIST_MANAGER_H */
