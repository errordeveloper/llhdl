#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

struct enumerated_signal {
	struct verilog_signal *orig;
	int value;
	struct enumerated_signal *next;
};

struct enumeration {
	struct enumerated_signal *head;
};

static struct enumeration *new_enumeration()
{
	struct enumeration *e;

	e = malloc(sizeof(struct enumeration));
	assert(e != NULL);
	e->head = NULL;
	return e;
}

static void free_enumeration(struct enumeration *e)
{
	struct enumerated_signal *s1, *s2;
	
	s1 = e->head;
	while(s1 != NULL) {
		s2 = s1->next;
		free(s1);
		s1 = s2;
	}
	free(e);
}

static struct enumerated_signal *find_signal_in_enumeration(struct enumeration *e, struct verilog_signal *orig)
{
	struct enumerated_signal *s;

	s = e->head;
	while(s != NULL) {
		if(s->orig == orig) return s;
		s = s->next;
	}
	return NULL;
}

static void add_enumeration(struct enumeration *e, struct verilog_signal *orig)
{
	struct enumerated_signal *new;
	
	if(find_signal_in_enumeration(e, orig) != NULL)
		return;
	new = malloc(sizeof(struct enumerated_signal));
	assert(new != NULL);
	new->orig = orig;
	new->value = 0;
	new->next = e->head;
	e->head = new;
}

static void enumerate_node(struct enumeration *e, struct verilog_node *n)
{
	if((n->type != VERILOG_NODE_CONSTANT) && (n->type != VERILOG_NODE_SIGNAL)) {
		int arity;
		int i;

		arity = verilog_get_node_arity(n->type);
		for(i=0;i<arity;i++)
			enumerate_node(e, n->branches[i]);
	}
	if(n->type == VERILOG_NODE_SIGNAL)
		add_enumeration(e, n->branches[0]);
}

static void enumerate_process(struct enumeration *e, struct verilog_process *p)
{
	struct verilog_assignment *a;

	a = p->head;
	while(a != NULL) {
		enumerate_node(e, a->source);
		a = a->next;
	}
}

static void transfer_process(struct llhdl_module *lm, struct verilog_process *p)
{
	struct enumeration *e;
	struct enumerated_signal *s;

	e = new_enumeration();
	enumerate_process(e, p);

	printf("Process:");
	s = e->head;
	while(s != NULL) {
		printf(" %s", s->orig->name);
		s = s->next;
	}
	printf("\n");
	
	free_enumeration(e);
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
