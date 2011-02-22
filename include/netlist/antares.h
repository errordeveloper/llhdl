#ifndef __NETLIST_ANTARES_H
#define __NETLIST_ANTARES_H

#include <netlist/manager.h>

void netlist_m_antares_fd(struct netlist_manager *m, FILE *fd, const char *module_name, const char *part);
void netlist_m_antares_file(struct netlist_manager *m, const char *filename, const char *module_name, const char *part);

#endif /* __NETLIST_ANTARES_H */
