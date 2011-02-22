#include <stdio.h>
#include <stdlib.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/dot.h>

void netlist_m_dot_fd(struct netlist_manager *m, FILE *fd)
{
}

void netlist_m_dot_file(struct netlist_manager *m, const char *filename)
{
	FILE *fd;
	int r;

	fd = fopen(filename, "w");
	if(fd == NULL) {
		perror("netlist_m_antares_file");
		exit(EXIT_FAILURE);
	}
	netlist_m_dot_fd(m, fd);
	r = fclose(fd);
	if(r != 0) {
		perror("netlist_m_antares_file");
		exit(EXIT_FAILURE);
	}
}
