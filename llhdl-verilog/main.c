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
	verilog_dump_module(m);
	verilog_free_module(m);

	return 0;
}
