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
	printf("Usage: llhdl-dot [parameters] <input.lhd>\n");
	printf("The input design in LLHDL interchange format (input.lhd) is mandatory.\n");
	printf("Parameters are:\n");
	printf("  -h Display this help text and exit.\n");
	printf("  -o <output.dot> Set the name of the output file.\n");
	printf("  -l Label arcs.\n");
}

static char *mk_outname(char *inname)
{
	char *c;
	int r;
	char *out;
	
	inname = stralloc(inname);
	c = strrchr(inname, '.');
	if(c != NULL)
		*c = 0;
	r = asprintf(&out, "%s.dot", inname);
	if(r == -1) abort();
	free(inname);
	return out;
}

int main(int argc, char *argv[])
{
	int opt;
	char *inname;
	char *outname;
	struct llhdl_module *m;
	FILE *fd;

	outname = NULL;
	while((opt = getopt(argc, argv, "ho:l")) != -1) {
		switch(opt) {
			case 'h':
				help();
				exit(EXIT_SUCCESS);
				break;
			case 'o':
				free(outname);
				outname = stralloc(optarg);
				break;
			case 'l':
				print_labels = 1;
				break;
			default:
				fprintf(stderr, "Invalid option passed. Use -h for help.\n");
				exit(EXIT_FAILURE);
				break;
		}
	}
	
	if((argc - optind) != 1) {
		fprintf(stderr, "llhdl-dot: missing input file. Use -h for help.\n");
		exit(EXIT_FAILURE);
	}
	inname = argv[optind];
	if(outname == NULL)
		outname = mk_outname(inname);
	
	m = llhdl_parse_file(inname);
	fd = fopen(outname, "w");
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
	
	llhdl_free_module(m);
	free(outname);

	return 0;
}

