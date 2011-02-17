#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util.h>
#include <gmp.h>

#include <llhdl/structure.h>

int llhdl_get_logic_arity(int op)
{
	switch(op) {
		case LLHDL_LOGIC_NOT: return 1;
		case LLHDL_LOGIC_AND: return 2;
		case LLHDL_LOGIC_OR: return 2;
		case LLHDL_LOGIC_XOR: return 2;
		default: assert(0);
	}
	return 0;
}

struct llhdl_module *llhdl_new_module()
{
	struct llhdl_module *m;

	m = alloc_type(struct llhdl_module);
	m->name = NULL;
	m->head = NULL;

	return m;
}

void llhdl_free_module(struct llhdl_module *m)
{
	struct llhdl_node *n1, *n2;
	
	free(m->name);

	/* We must traverse the list twice,
	 * otherwise invalid signal references may appear in
	 * llhdl_free_node().
	 */
	n1 = m->head;
	while(n1 != NULL) {
		assert(n1->type == LLHDL_NODE_SIGNAL);
		llhdl_free_node(n1->p.signal.source);
		n1 = n1->p.signal.next;
	}
	n1 = m->head;
	while(n1 != NULL) {
		assert(n1->type == LLHDL_NODE_SIGNAL);
		n2 = n1->p.signal.next;
		free(n1);
		n1 = n2;
	}
	free(m);
}

void llhdl_set_module_name(struct llhdl_module *m, const char *name)
{
	free(m->name);
	if(name == NULL)
		m->name = NULL;
	else
		m->name = stralloc(name);
}

static struct llhdl_node *alloc_base_node(int payload_size, int type)
{
	struct llhdl_node *n;

	n = alloc_size(sizeof(int)+payload_size);
	n->type = type;
	return n;
}

struct llhdl_node *llhdl_create_constant(mpz_t value, int sign, int vectorsize)
{
	struct llhdl_node *n;

	n = alloc_base_node(sizeof(struct llhdl_node_constant), LLHDL_NODE_CONSTANT);
	mpz_init_set(n->p.constant.value, value);
	n->p.constant.sign = sign;
	n->p.constant.vectorsize = vectorsize;
	return n;
}

struct llhdl_node *llhdl_create_signal(struct llhdl_module *m, int type, const char *name, int sign, int vectorsize)
{
	struct llhdl_node *n;
	int len;

	len = strlen(name);
	n = alloc_base_node(sizeof(struct llhdl_node_signal)+len+1, LLHDL_NODE_SIGNAL);
	n->p.signal.type = type;
	n->p.signal.sign = sign;
	n->p.signal.vectorsize = vectorsize;
	n->p.signal.source = NULL;
	n->p.signal.next = m->head;
	memcpy(n->p.signal.name, name, len+1);
	m->head = n;
	return n;
}

struct llhdl_node *llhdl_create_logic(int op, struct llhdl_node **operands)
{
	struct llhdl_node *n;
	int i;
	int arity;

	arity = llhdl_get_logic_arity(op);
	n = alloc_base_node(sizeof(struct llhdl_node_logic)+arity*sizeof(struct llhdl_node *), LLHDL_NODE_LOGIC);
	n->p.logic.op = op;
	for(i=0;i<arity;i++)
		n->p.logic.operands[i] = operands[i];
	return n;
}

struct llhdl_node *llhdl_create_mux(int nsources, struct llhdl_node *select, struct llhdl_node **sources)
{
	struct llhdl_node *n;
	int i;
	
	n = alloc_base_node(sizeof(struct llhdl_node_mux)+nsources*sizeof(struct llhdl_node *), LLHDL_NODE_MUX);
	n->p.mux.nsources = nsources;
	n->p.mux.select = select;
	for(i=0;i<nsources;i++)
		n->p.mux.sources[i] = sources[i];
	return n;
}

struct llhdl_node *llhdl_create_fd(struct llhdl_node *clock, struct llhdl_node *data)
{
	struct llhdl_node *n;

	n = alloc_base_node(sizeof(struct llhdl_node_fd), LLHDL_NODE_FD);
	n->p.fd.clock = clock;
	n->p.fd.data = data;
	return n;
}

struct llhdl_node *llhdl_create_vect(int sign, int nslices, struct llhdl_slice *slices)
{
	struct llhdl_node *n;
	int i;

	n = alloc_base_node(sizeof(struct llhdl_node_vect)+nslices*sizeof(struct llhdl_slice), LLHDL_NODE_VECT);
	n->p.vect.sign = sign;
	n->p.vect.nslices = nslices;
	for(i=0;i<nslices;i++) {
		if((slices[i].start < 0) || (slices[i].end < 0) || (slices[i].start > slices[i].end)) {
			fprintf(stderr, "Invalid bounds of LLHDL slice (%d - %d)\n", slices[i].start, slices[i].end);
			exit(EXIT_FAILURE);
		}
		n->p.vect.slices[i] = slices[i];
	}
	return n;
}

struct llhdl_node *llhdl_create_arith(int op, struct llhdl_node *a, struct llhdl_node *b)
{
	struct llhdl_node *n;

	n = alloc_base_node(sizeof(struct llhdl_node_arith), LLHDL_NODE_ARITH);
	n->p.arith.op = op;
	n->p.arith.a = a;
	n->p.arith.b = b;
	return n;
}

void llhdl_free_node(struct llhdl_node *n)
{
	int i;
	int arity;

	if(n == NULL)
		return;
	if(n->type == LLHDL_NODE_SIGNAL)
		return;

	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			mpz_clear(n->p.constant.value);
			break;
		case LLHDL_NODE_LOGIC:
			arity = llhdl_get_logic_arity(n->p.logic.op);
			for(i=0;i<arity;i++)
				llhdl_free_node(n->p.logic.operands[i]);
			break;
		case LLHDL_NODE_MUX:
			llhdl_free_node(n->p.mux.select);
			for(i=0;i<n->p.mux.nsources;i++)
				llhdl_free_node(n->p.mux.sources[i]);
			break;
		case LLHDL_NODE_FD:
			llhdl_free_node(n->p.fd.clock);
			llhdl_free_node(n->p.fd.data);
			break;
		case LLHDL_NODE_VECT:
			for(i=0;i<n->p.vect.nslices;i++)
				llhdl_free_node(n->p.vect.slices[i].source);
			break;
		case LLHDL_NODE_ARITH:
			llhdl_free_node(n->p.arith.a);
			llhdl_free_node(n->p.arith.b);
			break;
		default:
			assert(0);
			break;
	}
	free(n);
}

void llhdl_free_signal(struct llhdl_node *n)
{
	assert(n->type == LLHDL_NODE_SIGNAL);
	llhdl_free_node(n->p.signal.source);
	free(n);
}

struct llhdl_node *llhdl_find_signal(struct llhdl_module *m, const char *name)
{
	struct llhdl_node *n;

	n = m->head;
	while(n != NULL) {
		assert(n->type == LLHDL_NODE_SIGNAL);
		if(strcmp(n->p.signal.name, name) == 0)
			return n;
		n = n->p.signal.next;
	}
	return NULL;
}
