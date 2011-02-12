#include <bd/bd.h>

void bd_replace(struct llhdl_module *m, unsigned int capabilities)
{
}

void bd_flow(struct llhdl_module *m, unsigned int capabilities)
{
	bd_replace(m, capabilities);
	bd_purify(m);
	bd_devectorize(m);
}

