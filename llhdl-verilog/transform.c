#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>
#include <gmp.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

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

static int convert_logictype(int verilog_type)
{
	switch(verilog_type) {
		case VERILOG_NODE_NEQ: return LLHDL_LOGIC_XOR;
		case VERILOG_NODE_OR: return LLHDL_LOGIC_OR;
		case VERILOG_NODE_AND: return LLHDL_LOGIC_AND;
		case VERILOG_NODE_TILDE: return LLHDL_LOGIC_NOT;
		case VERILOG_NODE_XOR: return LLHDL_LOGIC_XOR;
	}
	assert(0);
	return 0;
}

static void transfer_signals(struct llhdl_module *lm, struct verilog_module *vm)
{
	struct verilog_signal *s;

	s = vm->shead;
	while(s != NULL) {
		s->llhdl_signal = llhdl_create_signal(lm, convert_sigtype(s->type), s->name, s->sign, s->vectorsize);
		s = s->next;
	}
}

static struct llhdl_node *compile_node(struct verilog_node *n)
{
	struct llhdl_node *r;
	
	r = NULL;
	switch(n->type) {
		case VERILOG_NODE_CONSTANT: {
			struct verilog_constant *c;
			c = n->branches[0];
			r = llhdl_create_constant(c->value, c->sign, c->vectorsize);
			break;
		}
		case VERILOG_NODE_SIGNAL: {
			struct verilog_signal *s;
			s = n->branches[0];
			r = s->llhdl_signal;
			break;
		}
		case VERILOG_NODE_EQL: {
			struct llhdl_node *operands[2];
			struct llhdl_node *xor;
			operands[0] = compile_node(n->branches[0]);
			operands[1] = compile_node(n->branches[1]);
			xor = llhdl_create_logic(LLHDL_LOGIC_XOR, operands);
			r = llhdl_create_logic(LLHDL_LOGIC_NOT, &xor);
			break;
		}
		case VERILOG_NODE_NEQ:
		case VERILOG_NODE_OR:
		case VERILOG_NODE_AND:
		case VERILOG_NODE_TILDE:
		case VERILOG_NODE_XOR: {
			struct llhdl_node **operands;
			int i, arity;
			arity = verilog_get_node_arity(n->type);
			operands = alloc_size(arity*sizeof(struct llhdl_node *));
			for(i=0;i<arity;i++)
				operands[i] = compile_node(n->branches[i]);
			r = llhdl_create_logic(convert_logictype(n->type), operands);
			free(operands);
			break;
		}
		case VERILOG_NODE_ALT: {
			struct llhdl_node *operands[3];
			int i;
			for(i=0;i<3;i++)
				operands[i] = compile_node(n->branches[i]);
			r = llhdl_create_mux(2, operands[0], &operands[1]);
			break;
		}
	}
	return r;
}

struct compile_statement_param {
	int bl;
	struct llhdl_module *lm;
};

struct compile_condition {
	struct llhdl_node *expr;
	int negate;
	struct compile_condition *next;
};

static void compile_statements(struct compile_statement_param *csp, struct verilog_statement *s, struct compile_condition *conditions);

static struct llhdl_node *generate_cond_muxes(struct compile_condition *condition, struct llhdl_node *others, struct llhdl_node *final)
{
	struct llhdl_node *sources[2];

	if(condition == NULL)
		return final;
	if(condition->negate) {
		sources[0] = generate_cond_muxes(condition->next, others, final);
		sources[1] = others;
	} else {
		sources[0] = others;
		sources[1] = generate_cond_muxes(condition->next, others, final);
	}
	return llhdl_create_mux(2, llhdl_dup(condition->expr), sources);
}

static void compile_assignment(struct compile_statement_param *csp, struct verilog_statement *s, struct compile_condition *conditions)
{
	struct llhdl_node *ls;
	struct llhdl_node *expr;
	struct llhdl_node **target;
	struct compile_condition *condition;

	ls = s->p.assignment.target->llhdl_signal;
	expr = compile_node(s->p.assignment.source);

	target = &ls->p.signal.source;
	condition = conditions;

	/* Walk existing conditional muxes that match the specified condition */
	if(*target != NULL) {
		while((condition != NULL)
		  && ((*target)->type == LLHDL_NODE_MUX)
		  && ((*target)->p.mux.nsources == 2)
		  && (llhdl_equiv(condition->expr, (*target)->p.mux.select))) {
			if(condition->negate)
				target = &(*target)->p.mux.sources[0];
			else
				target = &(*target)->p.mux.sources[1];
			condition = condition->next;
		}
	}
	
	/* Generate extra conditional muxes */
	expr = generate_cond_muxes(condition, ls, expr);
	
	llhdl_free_node(*target);
	*target = expr;
}

static void compile_condition(struct compile_statement_param *csp, struct verilog_statement *s, struct compile_condition *conditions)
{
	struct compile_condition new_condition;
	
	new_condition.expr = compile_node(s->p.condition.condition);
	new_condition.next = conditions;
	new_condition.negate = 1;
	compile_statements(csp, s->p.condition.negative, &new_condition);
	new_condition.negate = 0;
	compile_statements(csp, s->p.condition.positive, &new_condition);
	llhdl_free_node(new_condition.expr);
}

static void compile_statements(struct compile_statement_param *csp, struct verilog_statement *s, struct compile_condition *conditions)
{
	while(s != NULL) {
		switch(s->type) {
			case VERILOG_STATEMENT_ASSIGNMENT:
				compile_assignment(csp, s, conditions);
				break;
			case VERILOG_STATEMENT_CONDITION:
				compile_condition(csp, s, conditions);
				break;
			default:
				assert(0);
				break;
		}
		s = s->next;
	}
}

static void transfer_process(struct llhdl_module *lm, struct verilog_process *p)
{
	int bl;
	struct compile_statement_param csp;
	
	bl = verilog_process_blocking(p);
	if(bl == VERILOG_BL_MIXED) {
		fprintf(stderr, "Invalid use of blocking/nonblocking assignments in process\n");
		exit(EXIT_FAILURE);
	}
	
	csp.bl = bl == VERILOG_BL_BLOCKING;
	csp.lm = lm;
	
	compile_statements(&csp, p->head, NULL);
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
