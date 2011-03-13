#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <netlist/net.h>
#include <netlist/manager.h>
#include <netlist/antares.h>

static void write_ports(struct netlist_manager *m, FILE *fd)
{
	struct netlist_instance *inst;
	int i;
	
	inst = m->ihead;
	while(inst != NULL) {
		if((inst->p->type == NETLIST_PRIMITIVE_PORT_OUT) || (inst->p->type == NETLIST_PRIMITIVE_PORT_IN)) {
			if(inst->p->type == NETLIST_PRIMITIVE_PORT_OUT)
				fprintf(fd, "out I%08x", inst->uid);
			else
				fprintf(fd, "in I%08x", inst->uid);
			for(i=0;i<inst->p->attribute_count;i++)
				fprintf(fd, " attr %s %s",
					inst->p->attribute_names[i],
					inst->attributes[i]);
			fprintf(fd, "\n");
		}
		inst = inst->next;
	}
}

static void write_instances(struct netlist_manager *m, FILE *fd)
{
	struct netlist_instance *inst;
	int i;
	
	inst = m->ihead;
	while(inst != NULL) {
		if(inst->p->type == NETLIST_PRIMITIVE_INTERNAL) {
			fprintf(fd, "inst I%08x %s", inst->uid, inst->p->name);
			for(i=0;i<inst->p->attribute_count;i++)
				fprintf(fd, " attr %s %s",
					inst->p->attribute_names[i],
					inst->attributes[i]);
			fprintf(fd, "\n");
		}
		inst = inst->next;
	}
}

static int net_driven_by(struct netlist_net *net, struct netlist_instance *inst, int pin)
{
	struct netlist_branch *b;
	
	b = net->head;
	while(b != NULL) {
		if((b->inst == inst) && b->output && (b->pin_index == pin))
			return 1;
		b = b->next;
	}
	return 0;
}

static void write_nets_at_instance_outpin(struct netlist_manager *m, FILE *fd, struct netlist_instance *inst, int pin)
{
	struct netlist_net *net;
	struct netlist_branch *b;
	
	fprintf(fd, "net I%08x %s", inst->uid, inst->p->output_names[pin]);
	net = m->nhead;
	while(net != NULL) {
		if(net_driven_by(net, inst, pin)) {
			b = net->head;
			while(b != NULL) {
				if(!b->output)
					fprintf(fd, " end I%08x %s", b->inst->uid, b->inst->p->input_names[b->pin_index]);
				b = b->next;
			}
		}
		net = net->next;
	}
	fprintf(fd, "\n");
}

static void write_nets(struct netlist_manager *m, FILE *fd)
{
	struct netlist_instance *inst;
	int i;
	
	inst = m->ihead;
	while(inst != NULL) {
		for(i=0;i<inst->p->outputs;i++)
			write_nets_at_instance_outpin(m, fd, inst, i);
		inst = inst->next;
	}
}

void netlist_m_antares_fd(struct netlist_manager *m, FILE *fd, const char *module_name, const char *part)
{
	fprintf(fd, "module %s\n", module_name);
	fprintf(fd, "part %s\n", part);
	write_ports(m, fd);
	write_instances(m, fd);
	write_nets(m, fd);
}

void netlist_m_antares_file(struct netlist_manager *m, const char *filename, const char *module_name, const char *part)
{
	FILE *fd;
	int r;

	fd = fopen(filename, "w");
	if(fd == NULL) {
		perror("netlist_m_antares_file");
		exit(EXIT_FAILURE);
	}
	netlist_m_antares_fd(m, fd, module_name, part);
	r = fclose(fd);
	if(r != 0) {
		perror("netlist_m_antares_file");
		exit(EXIT_FAILURE);
	}
}
