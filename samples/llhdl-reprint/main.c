#include <stdio.h>

#include <llhdl/structure.h>
#include <llhdl/exchange.h>

int main(int argc, char *argv[])
{
	struct llhdl_module *m;
	
	if(argc != 2) {
		fprintf(stderr, "Missing input filename\n");
		return 1;
	}
	
	m = llhdl_parse_file(argv[1]);
	llhdl_write_fd(m, stdout);
	llhdl_free_module(m);

	return 0;
}
