#ifndef __NETLIST_IO_H
#define __NETLIST_IO_H

struct netlist_iop_element {
	struct netlist_primitive *p;
	struct netlist_iop_element *next;
};

struct netlist_iop_manager {
	struct netlist_iop_element *head;
};

struct netlist_iop_manager *netlist_create_iop_manager();
void netlist_free_iop_manager(struct netlist_iop_manager *m);
struct netlist_primitive *netlist_create_io_primitive(struct netlist_iop_manager *m, int type, const char *name);

#endif /* __NETLIST_IO_H */
