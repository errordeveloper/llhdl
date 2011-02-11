#include <assert.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

int llhdl_belongs_in_pure(int purity, int type)
{
	switch(purity) {
		case LLHDL_COMPOUND:
		case LLHDL_PURE_EMPTY:
		case LLHDL_PURE_SIGNAL:
			return 1;
		case LLHDL_PURE_CONSTANT:
			return type != LLHDL_NODE_FD;
		case LLHDL_PURE_BDD:
			return (type == LLHDL_NODE_MUX) || (type == LLHDL_NODE_SIGNAL) || (type == LLHDL_NODE_BOOLEAN) || (type == LLHDL_NODE_INTEGER);
		case LLHDL_PURE_FD:
			return (type == LLHDL_NODE_FD) || (type == LLHDL_NODE_SIGNAL);
		case LLHDL_PURE_VECTOR:
			return (type == LLHDL_NODE_SLICE) || (type == LLHDL_NODE_CAT) || (type == LLHDL_NODE_SIGN)
				|| (type == LLHDL_NODE_SIGNAL) || (type == LLHDL_NODE_BOOLEAN) || (type == LLHDL_NODE_INTEGER);
		case LLHDL_PURE_ARITH:
			return (type == LLHDL_NODE_ARITH) || (type == LLHDL_NODE_SIGNAL) || (type == LLHDL_NODE_BOOLEAN) || (type == LLHDL_NODE_INTEGER);
		default:
			assert(0);
			return 0;
	}
}

void llhdl_update_purity(int *previous, int new)
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

void llhdl_update_purity_node(int *previous, int type)
{
	switch(type) {
		case LLHDL_NODE_BOOLEAN:
		case LLHDL_NODE_INTEGER:
			llhdl_update_purity(previous, LLHDL_PURE_CONSTANT);
			break;
		case LLHDL_NODE_SIGNAL:
			llhdl_update_purity(previous, LLHDL_PURE_SIGNAL);
			break;
		case LLHDL_NODE_MUX:
			llhdl_update_purity(previous, LLHDL_PURE_BDD);
			break;
		case LLHDL_NODE_FD:
			llhdl_update_purity(previous, LLHDL_PURE_FD);
			break;
		case LLHDL_NODE_SLICE:
			llhdl_update_purity(previous, LLHDL_PURE_VECTOR);
			break;
		case LLHDL_NODE_CAT:
			llhdl_update_purity(previous, LLHDL_PURE_VECTOR);
			break;
		case LLHDL_NODE_SIGN:
			llhdl_update_purity(previous, LLHDL_PURE_VECTOR);
			break;
		case LLHDL_NODE_ARITH:
			llhdl_update_purity(previous, LLHDL_PURE_ARITH);
			break;
		default:
			assert(0);
			break;
	}
}

int llhdl_is_pure(struct llhdl_node *n)
{
	int purity;
	
	purity = LLHDL_PURE_EMPTY;
	if(n != NULL) {
		llhdl_update_purity_node(&purity, n->type);
		switch(n->type) {
			case LLHDL_NODE_BOOLEAN:
			case LLHDL_NODE_INTEGER:
			case LLHDL_NODE_SIGNAL:
				break;
			case LLHDL_NODE_MUX:
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.mux.negative));
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.mux.positive));
				break;
			case LLHDL_NODE_FD:
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.fd.data));
				break;
			case LLHDL_NODE_SLICE:
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.slice.source));
				break;
			case LLHDL_NODE_CAT:
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.cat.msb));
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.cat.lsb));
				break;
			case LLHDL_NODE_SIGN:
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.sign.source));
				break;
			case LLHDL_NODE_ARITH:
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.arith.a));
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.arith.b));
				break;
			default:
				assert(0);
				break;
		}
	}
	return purity;
}

