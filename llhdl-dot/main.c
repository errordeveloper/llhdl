#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <util.h>

#include <llhdl/structure.h>
#include <llhdl/exchange.h>
#include <llhdl/tools.h>

#include <banner/banner.h>

static int print_labels;

static unsigned int new_id()
{
	static unsigned int next_id;
	return next_id++;
}

static const char *signal_style(int type)
{
	switch(type) {
		case LLHDL_SIGNAL_INTERNAL: return ", style=\"filled,rounded\", fillcolor=lightgray";
		case LLHDL_SIGNAL_PORT_OUT: return ", style=\"filled,bold\", fillcolor=lightgray";
		case LLHDL_SIGNAL_PORT_IN: return ", style=filled, fillcolor=lightgray";
	}
	return NULL;
}

static char *connect_label(struct llhdl_node *n)
{
	static char buf[32];
	
	if(print_labels)
		sprintf(buf, " [label=\"%c:%d\"]", llhdl_get_sign(n) ? 's':'u', llhdl_get_vectorsize(n));
	else
		buf[0] = 0;
	return buf;
}

static unsigned int declare_down(FILE *fd, struct llhdl_node *n)
{
	int this_id;
	int i, arity;
	unsigned int select;
	unsigned int *operands;

	this_id = new_id();
	fprintf(fd, "N%x [label=\"<out> %s", this_id, 
		n->type == LLHDL_NODE_SIGNAL ? n->p.signal.name : llhdl_strtype(n->type));
	switch(n->type) {
		case LLHDL_NODE_SIGNAL:
			fprintf(fd, "\\n%s:%d\"%s];\n", 
				n->p.signal.sign ? "signed":"unsigned",
				n->p.signal.vectorsize,
				signal_style(n->p.signal.type));
			break;
		case LLHDL_NODE_CONSTANT:
			fprintf(fd, "|<value>");
			if(n->p.constant.sign || (n->p.constant.vectorsize != 1))
				fprintf(fd, "%d%c", n->p.constant.vectorsize, n->p.constant.sign ? 's' : 'u');
			mpz_out_str(fd, 10, n->p.constant.value);
			fprintf(fd, "\"];\n");
			break;
		case LLHDL_NODE_LOGIC:
		case LLHDL_NODE_EXTLOGIC:
			fprintf(fd, "|<op> %s", llhdl_strlogic(n->p.logic.op));
			arity = llhdl_get_logic_arity(n->p.logic.op);
			for(i=0;i<arity;i++)
				fprintf(fd, "|<operand%d> %c", i, 'a'+i);
			fprintf(fd, "\"];\n");
			operands = alloc_size(arity*sizeof(unsigned int));
			for(i=0;i<arity;i++)
				operands[i] = declare_down(fd, n->p.logic.operands[i]);
			for(i=0;i<arity;i++)
				fprintf(fd, "N%x:out -> N%x:operand%d%s;\n", operands[i], this_id, i, connect_label(n->p.logic.operands[i]));
			free(operands);
			break;
		case LLHDL_NODE_MUX:
			fprintf(fd, "|<select> select");
			for(i=0;i<n->p.mux.nsources;i++)
				fprintf(fd, "|<source%d> %d", i, i);
			fprintf(fd, "\"];\n");
			select = declare_down(fd, n->p.mux.select);
			operands = alloc_size(n->p.mux.nsources*sizeof(unsigned int));
			for(i=0;i<n->p.mux.nsources;i++)
				operands[i] = declare_down(fd, n->p.mux.sources[i]);
			fprintf(fd, "N%x:out -> N%x:select%s;\n", select, this_id, connect_label(n->p.mux.select));
			for(i=0;i<n->p.mux.nsources;i++)
				fprintf(fd, "N%x:out -> N%x:source%d%s;\n", operands[i], this_id, i, connect_label(n->p.mux.sources[i]));
			free(operands);
			break;
		case LLHDL_NODE_FD:
			fprintf(fd, "|<clock> clock|<data> data\"];\n");
			operands = alloc_size(2*sizeof(unsigned int));
			operands[0] = declare_down(fd, n->p.fd.clock);
			operands[1] = declare_down(fd, n->p.fd.data);
			fprintf(fd, "N%x:out -> N%x:clock%s;\n", operands[0], this_id, connect_label(n->p.fd.clock));
			fprintf(fd, "N%x:out -> N%x:data%s;\n", operands[1], this_id, connect_label(n->p.fd.data));
			free(operands);
			break;
		case LLHDL_NODE_VECT:
			fprintf(fd, "|<sign> %s", n->p.vect.sign ? "signed" : "unsigned");
			for(i=0;i<n->p.vect.nslices;i++)
				fprintf(fd, "|<slice%d> %d-%d", 
					i, n->p.vect.slices[i].start, n->p.vect.slices[i].end);
			fprintf(fd, "\"];\n");
			operands = alloc_size(n->p.vect.nslices*sizeof(unsigned int));
			for(i=0;i<n->p.vect.nslices;i++)
				operands[i] = declare_down(fd, n->p.vect.slices[i].source);
			for(i=0;i<n->p.vect.nslices;i++)
				fprintf(fd, "N%x:out -> N%x:slice%d%s;\n", operands[i], this_id, i, connect_label(n->p.vect.slices[i].source));
			free(operands);
			break;
		default:
			assert(0);
	}
	
	return this_id;
}

static void declare_arcs(FILE *fd, struct llhdl_module *m)
{
	struct llhdl_node *n;
	unsigned int sig, source;
	
	n = m->head;
	while(n != NULL) {
		if(n->p.signal.source != NULL) {
			sig = new_id();
			fprintf(fd, "N%x [label=\"<out> %s\\n%s:%d\"%s];\n",
				sig,
				n->p.signal.name,
				n->p.signal.sign ? "signed":"unsigned",
				n->p.signal.vectorsize,
				signal_style(n->p.signal.type));
				source = declare_down(fd, n->p.signal.source);
				fprintf(fd, "N%x:out -> N%x%s;\n", source, sig, connect_label(n->p.signal.source));
		}
		n = n->p.signal.next;
	}
}

static void help()
{
	banner("Dot (Graphviz) generator");
	printf("Usage: llhdl-dot <input.lhd> <output.dot> [\"labels\"]\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	struct llhdl_module *m;
	FILE *fd;

	if(argc != 3) {
		if((argc == 4) && (strcmp(argv[3], "labels") == 0))
			print_labels = 1;
		else
			help();
	}
	
	m = llhdl_parse_file(argv[1]);
	fd = fopen(argv[2], "w");
	if(fd == NULL) {
		perror("Unable to write output file");
		exit(EXIT_FAILURE);
	}
	
	fprintf(fd, "digraph %s {\nrankdir=BT;\n", m->name);
	fprintf(fd, "node [shape=record];\n");
	declare_arcs(fd, m);
	fprintf(fd, "}\n");
	
	if(fclose(fd) != 0) {
		perror("Unable to close output file");
		exit(EXIT_FAILURE);
	}

	return 0;
}

