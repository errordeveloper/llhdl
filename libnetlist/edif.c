#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/edif.h>

static void write_imports(struct netlist_manager *m, FILE *fd, struct edif_param *param)
{
	/* TODO */
}

static void write_io(struct netlist_manager *m, FILE *fd, struct edif_param *param)
{
	/* TODO */
}

static void write_instantiations(struct netlist_manager *m, FILE *fd, struct edif_param *param)
{
	struct netlist_instance *inst;
	int i;

	inst = m->head;
	while(inst != NULL) {
		fprintf(fd,
			"(instance I%08x\n"
			"(viewRef view_1 (cellRef %s (libraryRef %s)))\n",
			inst->uid,
			inst->p->name,
			param->cell_library);
		if(param->flavor == EDIF_FLAVOR_XILINX)
			fprintf(fd, "(property XSTLIB (boolean (true)) (owner \"Xilinx\"))\n");
		i = 0;
		while(inst->attributes[i] != NULL) {
			if(param->flavor == EDIF_FLAVOR_XILINX)
				fprintf(fd, "(property %s (string \"%s\") (owner \"Xilinx\"))",
					inst->p->attribute_names[i],
					inst->attributes[i]);
			else
				fprintf(fd, "(property %s (string \"%s\"))",
					inst->p->attribute_names[i],
					inst->attributes[i]);
			i++;
		}
		fprintf(fd, ")\n");
			
		inst = inst->next;
	}
}

static void write_connections(struct netlist_manager *m, FILE *fd, struct edif_param *param)
{
	struct netlist_instance *inst;
	int output;
	struct netlist_net *net;

	inst = m->head;
	while(inst != NULL) {
		for(output=0;output<inst->p->outputs;output++) {
			net = inst->outputs[output];
			if(net != NULL) {
				fprintf(fd, "(net N%08x\n", net->uid);
				fprintf(fd, "(joined\n");
				/* output */
				fprintf(fd, "(portRef %s (instanceRef I%08x))\n",
					inst->p->output_names[output],
					inst->uid);
				/* inputs */
				while(net != NULL) {
					fprintf(fd, "(portRef %s (instanceRef I%08x))\n",
						net->dest->p->input_names[net->input],
						net->dest->uid);
					net = net->next;
				}
				fprintf(fd, ")\n");
				fprintf(fd, ")\n");
			}
		}
		inst = inst->next;
	}
}

void netlist_m_edif_fd(struct netlist_manager *m, FILE *fd, struct edif_param *param)
{
	/* start EDIF */
	fprintf(fd,
		"(edif %s\n"
		"(edifVersion 2 0 0)\n"
		"(edifLevel 0)\n"
		"(keywordMap (keywordLevel 0))\n", param->design_name);

	write_imports(m, fd, param);

	/* start design library, top level cell, and netlist view */
	fprintf(fd,
		"(library %s_lib\n"
		"(edifLevel 0)\n"
		"(technology (numberDefinition))\n",
		param->design_name);
	fprintf(fd,
		"(cell %s\n"
		"(cellType GENERIC)\n",
		param->design_name);
	fprintf(fd,
		"(view view_1\n"
		"(viewType NETLIST)\n");

	fprintf(fd, "(interface\n");
	write_io(m, fd, param);
	fprintf(fd, ")\n");

	fprintf(fd, "(contents\n");
	write_instantiations(m, fd, param);
	write_connections(m, fd, param);
	fprintf(fd, ")\n");

	/* end view, top level cell and design library */
	fprintf(fd, ")\n");
	fprintf(fd, ")\n");
	fprintf(fd, ")\n");

	/* write design */
	fprintf(fd, 
		"(design %s\n"
		"(cellRef %s (libraryRef %s_lib))\n"
		"(property PART (string \"%s\") (owner \"%s\"))\n"
		")\n",
		param->design_name,
		param->design_name,
		param->design_name,
		param->part,
		param->manufacturer);
	
	/* end EDIF */
	fprintf(fd, ")\n");
}

void netlist_m_edif_file(struct netlist_manager *m, const char *filename, struct edif_param *param)
{
	FILE *fd;
	int r;

	fd = fopen(filename, "w");
	assert(fd != NULL);
	netlist_m_edif_fd(m, fd, param);
	r = fclose(fd);
	assert(r == 0);
}
