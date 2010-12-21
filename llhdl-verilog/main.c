#include <stdio.h>

#include "verilog.h"

int main(int argc, char *argv[])
{
	struct verilog_module *m;
	
	if(argc != 2) { // FIXME
		fprintf(stderr, "Usage: llhdl-verilog <input.v> <output.lhd>\n");
		return 1;
	}
	m = verilog_parse_file(argv[1]);
	if(m == NULL) {
		fprintf(stderr, "Parse failed\n");
		return 2;
	}
	printf("Module name: %s\n", m->name);
	verilog_free_module(m);

	return 0;
}
