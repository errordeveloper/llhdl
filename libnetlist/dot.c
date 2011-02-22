#include <stdio.h>
#include <stdlib.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/dot.h>

static void write_instances(struct netlist_manager *m, FILE *fd)
{
	struct netlist_instance *inst;
	int i;
	
	inst = m->ihead;
	while(inst != NULL) {
		fprintf(fd, "I%08x [label=\"", inst->uid);
		
		fprintf(fd, "{");
		for(i=0;i<inst->p->inputs;i++) {
			if(i != 0) fprintf(fd, "|");
			fprintf(fd, "<%s> %s", inst->p->input_names[i], inst->p->input_names[i]);
		}
		fprintf(fd, "}|");
		
		fprintf(fd, "{%s", inst->p->name);
		for(i=0;i<inst->p->attribute_count;i++) {
			fprintf(fd, "|%s=%s",
				inst->p->attribute_names[i],
				inst->attributes[i]);
		}
		fprintf(fd, "}|");
		
		fprintf(fd, "{");
		for(i=0;i<inst->p->outputs;i++) {
			if(i != 0) fprintf(fd, "|");
			fprintf(fd, "<%s> %s", inst->p->output_names[i], inst->p->output_names[i]);
		}
		fprintf(fd, "}\"");
		
		
		if(inst->p->type == NETLIST_PRIMITIVE_PORT_OUT)
			fprintf(fd, ", style=\"filled,bold\", fillcolor=lightgray");
		if(inst->p->type == NETLIST_PRIMITIVE_PORT_IN)
			fprintf(fd, ", style=filled, fillcolor=lightgray");
		
		fprintf(fd, "];\n");
		inst = inst->next;
	}
}

static struct netlist_branch *find_driver(struct netlist_net *net)
{
	struct netlist_branch *b;
	
	b = net->head;
	while(b != NULL) {
		if(b->output)
			return b;
		b = b->next;
	}
	return NULL;
}

static void write_nets(struct netlist_manager *m, FILE *fd)
{
	struct netlist_net *net;
	struct netlist_branch *driver;
	struct netlist_branch *target;
	
	net = m->nhead;
	while(net != NULL) {
		driver = find_driver(net);
		if(driver != NULL) {
			target = net->head;
			while(target != NULL) {
				if(!target->output)
					fprintf(fd, "I%08x:%s -> I%08x:%s;\n",
						driver->inst->uid, driver->inst->p->output_names[driver->pin_index],
						target->inst->uid, target->inst->p->input_names[target->pin_index]);
				target = target->next;
			}
		}
		net = net->next;
	}
}

void netlist_m_dot_fd(struct netlist_manager *m, FILE *fd, const char *module_name)
{
	fprintf(fd, "digraph %s {\n", module_name);
	fprintf(fd, "node [shape=record];\n");
	write_instances(m, fd);
	write_nets(m, fd);
	fprintf(fd, "}\n");
}

void netlist_m_dot_file(struct netlist_manager *m, const char *filename, const char *module_name)
{
	FILE *fd;
	int r;

	fd = fopen(filename, "w");
	if(fd == NULL) {
		perror("netlist_m_dot_file");
		exit(EXIT_FAILURE);
	}
	netlist_m_dot_fd(m, fd, module_name);
	r = fclose(fd);
	if(r != 0) {
		perror("netlist_m_dot_file");
		exit(EXIT_FAILURE);
	}
}
