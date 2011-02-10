#include <assert.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

static void update_purity(int *previous, int new)
{
	if(new == LLHDL_PURE_EMPTY) return;
	if(new == LLHDL_PURE_SIGNAL) {
		if(*previous == LLHDL_PURE_EMPTY)
			*previous = LLHDL_PURE_SIGNAL;
		return;
	}
	if(new == LLHDL_PURE_CONSTANT) {
		switch(*previous) {
			case LLHDL_PURE_EMPTY:
			case LLHDL_PURE_SIGNAL:
				*previous = LLHDL_PURE_CONSTANT;
				break;
			case LLHDL_PURE_CONSTANT:
			case LLHDL_PURE_BDD:
			case LLHDL_PURE_VECTOR:
			case LLHDL_PURE_ARITH:
				/* nothing to do */
				break;
			default:
				*previous = LLHDL_COMPOUND;
				break;
		}
		return;
	}
	if((*previous == LLHDL_PURE_EMPTY) || (*previous == LLHDL_PURE_SIGNAL))
		*previous = new;
	else if(*previous != new)
		*previous = LLHDL_COMPOUND;
}

int llhdl_is_pure(struct llhdl_node *n)
{
	int purity;
	
	purity = LLHDL_PURE_EMPTY;
	if(n != NULL) {
		switch(n->type) {
			case LLHDL_NODE_BOOLEAN:
			case LLHDL_NODE_INTEGER:
				update_purity(&purity, LLHDL_PURE_CONSTANT);
				break;
			case LLHDL_NODE_SIGNAL:
				update_purity(&purity, LLHDL_PURE_SIGNAL);
				break;
			case LLHDL_NODE_MUX:
				update_purity(&purity, LLHDL_PURE_BDD);
				update_purity(&purity, llhdl_is_pure(n->p.mux.negative));
				update_purity(&purity, llhdl_is_pure(n->p.mux.positive));
				break;
			case LLHDL_NODE_FD:
				update_purity(&purity, LLHDL_PURE_FD);
				update_purity(&purity, llhdl_is_pure(n->p.fd.data));
				break;
			case LLHDL_NODE_SLICE:
				update_purity(&purity, LLHDL_PURE_VECTOR);
				update_purity(&purity, llhdl_is_pure(n->p.slice.source));
				break;
			case LLHDL_NODE_CAT:
				update_purity(&purity, LLHDL_PURE_VECTOR);
				update_purity(&purity, llhdl_is_pure(n->p.cat.msb));
				update_purity(&purity, llhdl_is_pure(n->p.cat.lsb));
				break;
			case LLHDL_NODE_SIGN:
				update_purity(&purity, LLHDL_PURE_VECTOR);
				update_purity(&purity, llhdl_is_pure(n->p.sign.source));
				break;
			case LLHDL_NODE_ARITH:
				update_purity(&purity, LLHDL_PURE_ARITH);
				update_purity(&purity, llhdl_is_pure(n->p.arith.a));
				update_purity(&purity, llhdl_is_pure(n->p.arith.b));
				break;
			default:
				assert(0);
				break;
		}
	}
	return purity;
}

