#ifndef __NETLIST_IO_H
#define __NETLIST_IO_H

struct netlist_primitive *netlist_create_io_primitive(int type, const char *name);
void netlist_free_io_primitive(struct netlist_primitive *p);

#endif /* __NETLIST_IO_H */
