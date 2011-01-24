#include <stdio.h>

#include <banner/banner.h>

#include <llhdl/structure.h>
#include <llhdl/exchange.h>

#include "verilog.h"
#include "transform.h"

int main(int argc, char *argv[])
{
	struct verilog_module *vm;
	struct llhdl_module *lm;
	
	if(argc != 3) {
		banner("Verilog to LLHDL compiler");
		printf("Usage: llhdl-verilog <input.v> <output.lhd>\n");
		return 1;
	}
	
	vm = verilog_parse_file(argv[1]);
	lm = llhdl_new_module();
	
	verilog_dump_module(0, vm);
	transform(lm, vm);

	llhdl_write_file(lm, argv[2]);
	llhdl_free_module(lm);
	verilog_free_module(vm);

	return 0;
}
