#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include <banner/banner.h>

#include <llhdl/structure.h>
#include <llhdl/exchange.h>

#include "verilog.h"
#include "transform.h"

static void help()
{
	banner("Verilog to LLHDL compiler");
	printf("Usage: llhdl-verilog [parameters] <input.v>\n");
	printf("The input module in Verilog HDL (input.v) is mandatory.\n");
	printf("Parameters are:\n");
	printf("  -h Display this help text and exit.\n");
	printf("  -o <output.lhd> Set the name of the output file.\n");
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
	r = asprintf(&out, "%s.lhd", inname);
	if(r == -1) abort();
	free(inname);
	return out;
}

int main(int argc, char *argv[])
{
	int opt;
	char *inname;
	char *outname;
	struct verilog_module *vm;
	struct llhdl_module *lm;
	
	outname = NULL;
	while((opt = getopt(argc, argv, "ho:")) != -1) {
		switch(opt) {
			case 'h':
				help();
				exit(EXIT_SUCCESS);
				break;
			case 'o':
				free(outname);
				outname = stralloc(optarg);
				break;
			default:
				fprintf(stderr, "Invalid option passed. Use -h for help.\n");
				exit(EXIT_FAILURE);
				break;
		}
	}
	
	if((argc - optind) != 1) {
		fprintf(stderr, "llhdl-verilog: missing input file. Use -h for help.\n");
		exit(EXIT_FAILURE);
	}
	inname = argv[optind];
	if(outname == NULL)
		outname = mk_outname(inname);
	
	vm = verilog_parse_file(inname);
	lm = llhdl_new_module();
	
	transform(lm, vm);

	llhdl_write_file(lm, outname);

	llhdl_free_module(lm);
	verilog_free_module(vm);
	free(outname);

	return 0;
}
