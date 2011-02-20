#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <llhdl/structure.h>
#include <llhdl/exchange.h>
#include <llhdl/tools.h>

#include <banner/banner.h>

static void declare_signals(FILE *fd, struct llhdl_module *m)
{
	struct llhdl_node *n;
	
	n = m->head;
	while(n != NULL) {
		fprintf(fd, "N%lx [label=\"<out> %s\\n%s:%d\"];\n", (unsigned long int)n,
			n->p.signal.name, n->p.signal.sign ? "signed":"unsigned", n->p.signal.vectorsize);
		n = n->p.signal.next;
	}
}

static void declare_down(FILE *fd, struct llhdl_node *n)
{
	int i, arity;

	if(n->type == LLHDL_NODE_SIGNAL)
		return;
	fprintf(fd, "N%lx [label=\"<out> %s", (unsigned long int)n, llhdl_strtype(n->type));
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			fprintf(fd, "|<value>");
			if(n->p.constant.sign || (n->p.constant.vectorsize != 1))
				fprintf(fd, "%d%c", n->p.constant.vectorsize, n->p.constant.sign ? 's' : 'u');
			mpz_out_str(fd, 10, n->p.constant.value);
			fprintf(fd, "\"];\n");
			break;
		case LLHDL_NODE_LOGIC:
			fprintf(fd, "|<op> %s", llhdl_strlogic(n->p.logic.op));
			arity = llhdl_get_logic_arity(n->p.logic.op);
			for(i=0;i<arity;i++)
				fprintf(fd, "|<operand%d> %c", i, 'a'+i);
			fprintf(fd, "\"];\n");
			for(i=0;i<arity;i++)
				declare_down(fd, n->p.logic.operands[i]);
			for(i=0;i<arity;i++)
				fprintf(fd, "N%lx:out -> N%lx:operand%d;\n",
					(unsigned long int)n->p.logic.operands[i],
					(unsigned long int)n, i);
			break;
		case LLHDL_NODE_MUX:
			fprintf(fd, "|<select> select");
			for(i=0;i<n->p.mux.nsources;i++)
				fprintf(fd, "|<source%d> %d", i, i);
			fprintf(fd, "\"];\n");
			declare_down(fd, n->p.mux.select);
			fprintf(fd, "N%lx:out -> N%lx:select;\n",
				(unsigned long int)n->p.mux.select,
				(unsigned long int)n);
			for(i=0;i<n->p.mux.nsources;i++)
				fprintf(fd, "N%lx:out -> N%lx:source%d;\n",
					(unsigned long int)n->p.mux.sources[i],
					(unsigned long int)n, i);
			break;
		case LLHDL_NODE_FD:
			fprintf(fd, "|<clock> clock|<data> data\"];\n");
			declare_down(fd, n->p.fd.clock);
			declare_down(fd, n->p.fd.data);
			fprintf(fd, "N%lx:out -> N%lx:clock;\n",
				(unsigned long int)n->p.fd.clock,
				(unsigned long int)n);
			fprintf(fd, "N%lx:out -> N%lx:data;\n",
				(unsigned long int)n->p.fd.data,
				(unsigned long int)n);
			break;
		case LLHDL_NODE_VECT:
			fprintf(fd, "|<sign> %s", n->p.vect.sign ? "signed" : "unsigned");
			for(i=0;i<n->p.vect.nslices;i++)
				fprintf(fd, "|<slice%d> %d-%d", 
					i, n->p.vect.slices[i].start, n->p.vect.slices[i].end);
			fprintf(fd, "\"];\n");
			for(i=0;i<n->p.vect.nslices;i++)
				declare_down(fd, n->p.vect.slices[i].source);
			for(i=0;i<n->p.vect.nslices;i++)
				fprintf(fd, "N%lx:out -> N%lx:slice%d;\n",
					(unsigned long int)n->p.vect.slices[i].source,
					(unsigned long int)n, i);
			break;
		case LLHDL_NODE_ARITH:
			fprintf(fd, "|<op> %s|<operand0> a|<operand1> b\"];\n", llhdl_strarith(n->p.arith.op));
			declare_down(fd, n->p.arith.a);
			declare_down(fd, n->p.arith.b);
			fprintf(fd, "N%lx:out -> N%lx:operand0;\n",
				(unsigned long int)n->p.arith.a,
				(unsigned long int)n);
			fprintf(fd, "N%lx:out -> N%lx:operand1;\n",
				(unsigned long int)n->p.arith.b,
				(unsigned long int)n);
			break;
		default:
			assert(0);
	}
}

static void declare_graph(FILE *fd, struct llhdl_module *m)
{
	struct llhdl_node *n;
	
	n = m->head;
	while(n != NULL) {
		if(n->p.signal.source != NULL) {
			declare_down(fd, n->p.signal.source);
			fprintf(fd, "N%lx:out -> N%lx;\n",
				(unsigned long int)n->p.signal.source, (unsigned long int)n);
		}
		n = n->p.signal.next;
	}
}

int main(int argc, char *argv[])
{
	struct llhdl_module *m;
	FILE *fd;

	if(argc != 3) {
		banner("Dot (Graphviz) generator");
		printf("Usage: llhdl-dot <input.lhd> <output.dot>\n");
		exit(EXIT_FAILURE);
	}
	
	m = llhdl_parse_file(argv[1]);
	fd = fopen(argv[2], "w");
	if(fd == NULL) {
		perror("Unable to write output file");
		exit(EXIT_FAILURE);
	}
	
	fprintf(fd, "digraph %s {\n", m->name);
	fprintf(fd, "node [shape=record];\n");
	declare_signals(fd, m);
	declare_graph(fd, m);
	fprintf(fd, "}\n");
	
	if(fclose(fd) != 0) {
		perror("Unable to close output file");
		exit(EXIT_FAILURE);
	}

	return 0;
}

