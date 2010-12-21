#include <assert.h>
#include <stdio.h>

#include <llhdl/structure.h>

#include "verilog.h"
#include "transform.h"

static int convert_sigtype(int verilog_type)
{
	switch(verilog_type) {
		case VERILOG_SIGNAL_REGWIRE: return LLHDL_SIGNAL_INTERNAL;
		case VERILOG_SIGNAL_OUTPUT: return LLHDL_SIGNAL_PORT_OUT;
		case VERILOG_SIGNAL_INPUT: return LLHDL_SIGNAL_PORT_IN;
	}
	assert(0);
	return 0;
}

static void transfer_signals(struct llhdl_module *lm, struct verilog_module *vm)
{
	struct verilog_signal *s;

	s = vm->shead;
	while(s != NULL) {
		s->user = llhdl_create_signal(lm, convert_sigtype(s->type), s->sign, s->name, s->vectorsize);
		s = s->next;
	}
}

static void transfer_process(struct llhdl_module *lm, struct verilog_process *p)
{
}

static void transfer_processes(struct llhdl_module *lm, struct verilog_module *vm)
{
	struct verilog_process *p;

	p = vm->phead;
	while(p != NULL) {
		transfer_process(lm, p);
		p = p->next;
	}
}

void transform(struct llhdl_module *lm, struct verilog_module *vm)
{
	llhdl_set_module_name(lm, vm->name);
	transfer_signals(lm, vm);
	transfer_processes(lm, vm);
}
