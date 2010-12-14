#ifndef __NETLIST_EDIF_H
#define __NETLIST_EDIF_H

enum {
	EDIF_FLAVOR_VANILLA,
	EDIF_FLAVOR_XILINX,
};

struct edif_param {
	int flavor;
	char *design_name;
	char *cell_library;
	char *part;
	char *manufacturer;
};

void netlist_m_edif_fd(struct netlist_manager *m, FILE *fd, struct edif_param *param);
void netlist_m_edif_file(struct netlist_manager *m, const char *filename, struct edif_param *param);

#endif /* __NETLIST_EDIF_H */
