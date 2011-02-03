#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

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
	struct enumerated_signal *xref;
	struct enumerated_signal *next;
};

struct enumeration {
	struct enumerated_signal *ihead;
	int ocount;
	struct enumerated_signal *ohead;
};

static struct enumeration *new_enumeration()
{
	struct enumeration *e;

	e = alloc_type(struct enumeration);
	e->ihead = NULL;
	e->ocount = 0;
	e->ohead = NULL;
	return e;
}

static void free_enumeration(struct enumeration *e)
{
	struct enumerated_signal *s1, *s2;
	
	s1 = e->ihead;
	while(s1 != NULL) {
		s2 = s1->next;
		free(s1);
		s1 = s2;
	}
	s1 = e->ohead;
	while(s1 != NULL) {
		s2 = s1->next;
		free(s1);
		s1 = s2;
	}
	free(e);
}

static struct enumerated_signal *find_signal_in_enumeration(struct enumeration *e, struct verilog_signal *orig, int input)
{
	struct enumerated_signal *s;

	s = input ? e->ihead : e->ohead;
	while(s != NULL) {
		if(s->orig == orig) return s;
		s = s->next;
	}
	return NULL;
}

static void add_enumeration(struct enumeration *e, struct verilog_signal *orig, int input)
{
	struct enumerated_signal *new;
	
	if(find_signal_in_enumeration(e, orig, input) != NULL)
		return;
	new = alloc_type(struct enumerated_signal);
	new->orig = orig;
	new->value = -1;
	new->xref = NULL;
	if(input) {
		new->next = e->ihead;
		e->ihead = new;
	} else {
		new->next = e->ohead;
		e->ohead = new;
		e->ocount++;
	}
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
		add_enumeration(e, n->branches[0], 1);
}

static void enumerate_statements(struct enumeration *e, struct verilog_statement *s)
{
	while(s != NULL) {
		switch(s->type) {
			case VERILOG_STATEMENT_ASSIGNMENT:
				enumerate_node(e, s->p.assignment.source);
				add_enumeration(e, s->p.assignment.target, 0);
				break;
			case VERILOG_STATEMENT_CONDITION:
				enumerate_node(e, s->p.condition.condition);
				enumerate_statements(e, s->p.condition.negative);
				enumerate_statements(e, s->p.condition.positive);
				break;
			default:
				assert(0);
				break;
		}
		s = s->next;
	}
}

/* Clock (if any) is not included in the process enumeration */
static void enumerate_process(struct enumeration *e, struct verilog_process *p)
{
	enumerate_statements(e, p->head);
}

static void enumerate_xref(struct enumeration *e)
{
	struct enumerated_signal *s, *found;
	
	s = e->ohead;
	while(s != NULL) {
		found = find_signal_in_enumeration(e, s->orig, 1);
		if(found != NULL) {
			found->xref = s;
			s->xref = found;
		}
		s = s->next;
	}
}

static int evaluate_node(struct verilog_node *n, struct enumeration *e)
{
	int values[3];
	int i;
	int arity;

	if((n->type != VERILOG_NODE_CONSTANT) && (n->type != VERILOG_NODE_SIGNAL)) {
		arity = verilog_get_node_arity(n->type);
		for(i=0;i<arity;i++)
			values[i] = evaluate_node(n->branches[i], e);
	}

	switch(n->type) {
		case VERILOG_NODE_CONSTANT:
			return ((struct verilog_constant *)n->branches[0])->value;
		case VERILOG_NODE_SIGNAL: {
			struct enumerated_signal *s;
			s = find_signal_in_enumeration(e, n->branches[0], 1);
			/* If the process is using blocking assignments, 
			 * use the value assigned in the process, if any.
			 */
			if((s->xref != NULL) && (s->xref->value != -1))
				return s->xref->value;
			return s->value;
		}
		case VERILOG_NODE_EQL: return values[0] == values[1];
		case VERILOG_NODE_NEQ: return values[0] != values[1];
		case VERILOG_NODE_OR: return values[0] | values[1];
		case VERILOG_NODE_AND: return values[0] & values[1];
		case VERILOG_NODE_TILDE: return ~values[0];
		case VERILOG_NODE_XOR: return values[0] ^ values[1];
		case VERILOG_NODE_ALT: return values[0] ? values[1] : values[2];
		default:
			assert(0);
			return 0;
	}
}

static void run_statements(struct verilog_statement *s, struct enumeration *e)
{
	struct enumerated_signal *target;

	while(s != NULL) {
		switch(s->type) {
			case VERILOG_STATEMENT_ASSIGNMENT:
				target = find_signal_in_enumeration(e, s->p.assignment.target, 0);
				assert(target != NULL);
				target->value = evaluate_node(s->p.assignment.source, e);
				break;
			case VERILOG_STATEMENT_CONDITION:
				if(evaluate_node(s->p.condition.condition, e))
					run_statements(s->p.condition.positive, e);
				else
					run_statements(s->p.condition.negative, e);
				break;
			default:
				assert(0);
				break;
		}
		s = s->next;
	}
}

static void clear_outputs(struct enumeration *e)
{
	struct enumerated_signal *s;
	
	s = e->ohead;
	while(s != NULL) {
		s->value = -1;
		s = s->next;
	}
}

static void run_process(struct verilog_process *p, struct enumeration *e)
{
	clear_outputs(e);
	run_statements(p->head, e);
}

static int simple_equiv(struct llhdl_node *n1, struct llhdl_node *n2)
{
	if(n1->type != n2->type)
		return 0;
	switch(n1->type) {
		case LLHDL_NODE_BOOLEAN:
			return (n1->p.boolean.value == n2->p.boolean.value);
		case LLHDL_NODE_MUX:
			return ((n1->p.mux.sel == n2->p.mux.sel)
				&& simple_equiv(n1->p.mux.negative, n2->p.mux.negative)
				&& simple_equiv(n1->p.mux.positive, n2->p.mux.positive));
		default:
			return 0;
	}
}

static struct llhdl_node **make_llhdl_nodes(struct verilog_process *p, struct enumeration *e, struct enumerated_signal *s)
{
	struct llhdl_node **ret;
	struct enumerated_signal *os;
	int i;
	
	ret = alloc_size(sizeof(void *)*e->ocount);
	
	if(s == NULL) {
		run_process(p, e);
		os = e->ohead;
		for(i=0;i<e->ocount;i++) {
			assert(os != NULL);
			assert(os->value != -1);
			ret[i] = llhdl_create_boolean(os->value);
			os = os->next;
		}
		assert(os == NULL);
	} else {
		struct llhdl_node *sel;
		struct llhdl_node **neg, **pos;

		sel = s->orig->user;
		s->value = 0;
		neg = make_llhdl_nodes(p, e, s->next);
		s->value = 1;
		pos = make_llhdl_nodes(p, e, s->next);
		
		for(i=0;i<e->ocount;i++) {
			if(simple_equiv(neg[i], pos[i])) {
				llhdl_free_node(neg[i]);
				ret[i] = pos[i];
			} else
				ret[i] = llhdl_create_mux(sel, neg[i], pos[i]);
		}
		free(neg);
		free(pos);
	}
	
	return ret;
}

static void register_process(struct verilog_process *p, struct enumeration *e)
{
	struct enumerated_signal *s;
	struct llhdl_node *ls;
	
	if(p->clock != NULL) {
		s = e->ohead;
		while(s != NULL) {
			ls = s->orig->user;
			assert(ls->type == LLHDL_NODE_SIGNAL);
			assert(ls->p.signal.source != NULL);
			ls->p.signal.source = llhdl_create_fd(p->clock->user, ls->p.signal.source);
			s = s->next;
		}
	}
}

static void transfer_process(struct llhdl_module *lm, struct verilog_process *p)
{
	struct enumeration *e;
	int bl;
	struct llhdl_node **nodes;
	int i;
	struct enumerated_signal *s;
	struct llhdl_node *target;

	/* Create the signal enumeration */
	e = new_enumeration();
	enumerate_process(e, p);
	bl = verilog_process_blocking(p);
	if(bl == VERILOG_BL_MIXED) {
		fprintf(stderr, "Invalid use of blocking/nonblocking assignments in process\n");
		exit(EXIT_FAILURE);
	}
	if(bl == VERILOG_BL_BLOCKING)
		enumerate_xref(e);

	/* Generate the BDDs */
	nodes = make_llhdl_nodes(p, e, e->ihead);
	i = 0;
	s = e->ohead;
	while(s != NULL) {
		target = s->orig->user;
		assert(target->type == LLHDL_NODE_SIGNAL);
		target->p.signal.source = nodes[i];
		s = s->next;
		i++;
	}
	free(nodes);
	
	/* If the process is clocked, insert registers on all outputs */
	register_process(p, e);

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
