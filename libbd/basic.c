#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

struct purify_arc_param {
	struct llhdl_module *module;
	const char *name;
	int rename_count;
};

/* If node n is not compatible with purity, create a new signal and replace n with that signal */
static void purify_arc(struct purify_arc_param *p, struct llhdl_node **n, int purity)
{
	if(*n == NULL) return;
	if(llhdl_belongs_in_pure(purity, (*n)->type)) {
		llhdl_update_purity_node(&purity, (*n)->type);
		switch((*n)->type) {
			case LLHDL_NODE_CONSTANT:
			case LLHDL_NODE_SIGNAL:
				break;
			case LLHDL_NODE_MUX:
				purify_arc(p, &(*n)->p.mux.negative, purity);
				purify_arc(p, &(*n)->p.mux.positive, purity);
				break;
			case LLHDL_NODE_FD:
				purify_arc(p, &(*n)->p.fd.data, purity);
				break;
			case LLHDL_NODE_SLICE:
				purify_arc(p, &(*n)->p.slice.source, purity);
				break;
			case LLHDL_NODE_CAT:
				purify_arc(p, &(*n)->p.cat.msb, purity);
				purify_arc(p, &(*n)->p.cat.lsb, purity);
				break;
			case LLHDL_NODE_SIGN:
				purify_arc(p, &(*n)->p.sign.source, purity);
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
		purify_arc(p, &(*n)->p.signal.source, LLHDL_PURE_EMPTY);
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
		purify_arc(&p, &n->p.signal.source, LLHDL_PURE_EMPTY);
		n = n->p.signal.next;
	}
}


void bd_devectorize(struct llhdl_module *m)
{
}

