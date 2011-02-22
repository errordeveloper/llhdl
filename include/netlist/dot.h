#ifndef __NETLIST_DOT_H
#define __NETLIST_DOT_H

void netlist_m_dot_fd(struct netlist_manager *m, FILE *fd, const char *module_name);
void netlist_m_dot_file(struct netlist_manager *m, const char *filename, const char *module_name);

#endif /* __NETLIST_DOT_H */
