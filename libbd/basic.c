#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <bd/bd.h>

int bd_belongs_in_pure(int purity, int type)
{
	int is_connect;
	
	is_connect = (type == LLHDL_NODE_CONSTANT) || (type == LLHDL_NODE_SIGNAL) || (type == LLHDL_NODE_VECT);
	switch(purity) {
		case BD_PURE_EMPTY:
		case BD_COMPOUND:
		case BD_PURE_CONNECT:
			return 1;
		case BD_PURE_LOGIC:
			return (type == LLHDL_NODE_LOGIC) || (type == LLHDL_NODE_MUX) || is_connect;
		case BD_PURE_FD:
			return (type == LLHDL_NODE_FD) || is_connect;
		case BD_PURE_ARITH:
			return (type == LLHDL_NODE_ARITH) || is_connect;
		default:
			assert(0);
			return 0;
	}
}

void bd_update_purity(int *previous, int new)
{
	if(new == BD_PURE_EMPTY) return;
	if(new == BD_PURE_CONNECT) {
		if(*previous == BD_PURE_EMPTY)
			*previous = BD_PURE_CONNECT;
		return;
	}
	if((*previous == BD_PURE_EMPTY) || (*previous == BD_PURE_CONNECT))
		*previous = new;
	else if(*previous != new)
		*previous = BD_COMPOUND;
}

void bd_update_purity_node(int *previous, int type)
{
	switch(type) {
		case LLHDL_NODE_CONSTANT:
		case LLHDL_NODE_SIGNAL:
		case LLHDL_NODE_VECT:
			bd_update_purity(previous, BD_PURE_CONNECT);
			break;
		case LLHDL_NODE_LOGIC:
		case LLHDL_NODE_MUX:
			bd_update_purity(previous, BD_PURE_LOGIC);
			break;
		case LLHDL_NODE_FD:
			bd_update_purity(previous, BD_PURE_FD);
			break;
		case LLHDL_NODE_ARITH:
			bd_update_purity(previous, BD_PURE_ARITH);
			break;
		default:
			assert(0);
			break;
	}
}

int bd_is_pure(struct llhdl_node *n)
{
	int purity;
	int arity;
	int i;
	
	purity = BD_PURE_EMPTY;
	if(n != NULL) {
		bd_update_purity_node(&purity, n->type);
		switch(n->type) {
			case LLHDL_NODE_CONSTANT:
			case LLHDL_NODE_SIGNAL:
			case LLHDL_NODE_VECT:
				break;
			case LLHDL_NODE_LOGIC:
				arity = llhdl_get_logic_arity(n->p.logic.op);
				for(i=0;i<arity;i++)
					bd_update_purity(&purity, bd_is_pure(n->p.logic.operands[i]));
				break;
			case LLHDL_NODE_MUX:
				bd_update_purity(&purity, bd_is_pure(n->p.mux.select));
				for(i=0;i<n->p.mux.nsources;i++)
					bd_update_purity(&purity, bd_is_pure(n->p.mux.sources[i]));
			case LLHDL_NODE_FD:
				bd_update_purity(&purity, bd_is_pure(n->p.fd.data));
				break;
			case LLHDL_NODE_ARITH:
				bd_update_purity(&purity, bd_is_pure(n->p.arith.a));
				bd_update_purity(&purity, bd_is_pure(n->p.arith.b));
				break;
			default:
				assert(0);
				break;
		}
	}
	return purity;
}

struct purify_arc_param {
	struct llhdl_module *module;
	const char *name;
	int rename_count;
};

/* If node n is not compatible with purity, create a new signal and replace n with that signal */
static void purify_arc(struct purify_arc_param *p, struct llhdl_node **n, int purity)
{
	int i, arity;

	if(*n == NULL) return;
	if(bd_belongs_in_pure(purity, (*n)->type)) {
		bd_update_purity_node(&purity, (*n)->type);
		switch((*n)->type) {
			case LLHDL_NODE_CONSTANT:
			case LLHDL_NODE_SIGNAL:
				break;
			case LLHDL_NODE_LOGIC:
				arity = llhdl_get_logic_arity((*n)->p.logic.op);
				for(i=0;i<arity;i++)
					purify_arc(p, &(*n)->p.logic.operands[i], purity);
				break;
			case LLHDL_NODE_MUX:
				purify_arc(p, &(*n)->p.mux.select, purity);
				for(i=0;i<(*n)->p.mux.nsources;i++)
					purify_arc(p, &(*n)->p.mux.sources[i], purity);
				break;
			case LLHDL_NODE_FD:
				purify_arc(p, &(*n)->p.fd.data, purity);
				break;
			case LLHDL_NODE_VECT:
				for(i=0;i<(*n)->p.vect.nslices;i++)
					purify_arc(p, &(*n)->p.vect.slices[i].source, purity);
				break;
			case LLHDL_NODE_ARITH:
				purify_arc(p, &(*n)->p.arith.a, purity);
				purify_arc(p, &(*n)->p.arith.b, purity);
				break;
			default:
				assert(0);
				break;
		}
	} else {
		struct llhdl_node *new_arc;
		char *new_name;
		int r;
		
		new_arc = *n;
		
		r = asprintf(&new_name, "%s$%d", p->name, p->rename_count);
		if(r == -1) abort();
		p->rename_count++;
		*n = llhdl_create_signal(p->module, LLHDL_SIGNAL_INTERNAL, new_name, 
			llhdl_get_sign(new_arc), llhdl_get_vectorsize(new_arc));
		free(new_name);
		
		(*n)->p.signal.source = new_arc;
		purify_arc(p, &(*n)->p.signal.source, BD_PURE_EMPTY);
	}
}

void bd_purify(struct llhdl_module *m)
{
	struct llhdl_node *n;
	struct purify_arc_param p;
	
	p.module = m;
	n = m->head;
	while(n != NULL) {
		assert(n->type == LLHDL_NODE_SIGNAL);
		p.name = n->p.signal.name;
		p.rename_count = 0;
		/* This can add signals to the module, but they are added
		 * at the head and do not impact us. */
		purify_arc(&p, &n->p.signal.source, BD_PURE_EMPTY);
		n = n->p.signal.next;
	}
}
