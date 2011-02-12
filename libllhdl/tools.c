#include <assert.h>
#include <gmp.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <util.h>

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
			return (type == LLHDL_NODE_MUX) || (type == LLHDL_NODE_SIGNAL) || (type == LLHDL_NODE_CONSTANT);
		case LLHDL_PURE_FD:
			return (type == LLHDL_NODE_FD) || (type == LLHDL_NODE_SIGNAL);
		case LLHDL_PURE_VECTOR:
			return (type == LLHDL_NODE_SLICE) || (type == LLHDL_NODE_CAT) || (type == LLHDL_NODE_SIGN)
				|| (type == LLHDL_NODE_SIGNAL) || (type == LLHDL_NODE_CONSTANT);
		case LLHDL_PURE_ARITH:
			return (type == LLHDL_NODE_ARITH) || (type == LLHDL_NODE_SIGNAL) || (type == LLHDL_NODE_CONSTANT);
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
		case LLHDL_NODE_CONSTANT:
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
			case LLHDL_NODE_CONSTANT:
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

int llhdl_compare_constants(struct llhdl_node *n1, struct llhdl_node *n2)
{
	assert(n1->type == LLHDL_NODE_CONSTANT);
	assert(n2->type == LLHDL_NODE_CONSTANT);
	return (n1->p.constant.sign == n2->p.constant.sign) 
		&& (n1->p.constant.vectorsize == n2->p.constant.vectorsize) 
		&& (mpz_cmp(n1->p.constant.value, n2->p.constant.value) == 0);
}

int llhdl_get_sign(struct llhdl_node *n)
{
	if(n == NULL) return 0;
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			return n->p.constant.sign;
		case LLHDL_NODE_SIGNAL:
			return n->p.signal.sign;
		case LLHDL_NODE_MUX:
			return llhdl_get_sign(n->p.mux.negative) && llhdl_get_sign(n->p.mux.positive);
		case LLHDL_NODE_FD:
			return llhdl_get_sign(n->p.fd.data);
		case LLHDL_NODE_SLICE:
			return llhdl_get_sign(n->p.slice.source);
		case LLHDL_NODE_CAT:
			return llhdl_get_sign(n->p.cat.msb) && llhdl_get_sign(n->p.cat.lsb);
		case LLHDL_NODE_SIGN:
			return n->p.sign.sign;
		case LLHDL_NODE_ARITH:
			return llhdl_get_sign(n->p.arith.a) && llhdl_get_sign(n->p.arith.b);
		default:
			assert(0);
			break;
	}
}

int llhdl_get_vectorsize(struct llhdl_node *n)
{
	if(n == NULL) return 0;
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			return n->p.constant.vectorsize;
		case LLHDL_NODE_SIGNAL:
			return n->p.signal.vectorsize;
		case LLHDL_NODE_MUX:
			return max(llhdl_get_vectorsize(n->p.mux.negative), llhdl_get_vectorsize(n->p.mux.positive));
		case LLHDL_NODE_FD:
			return llhdl_get_vectorsize(n->p.fd.data);
		case LLHDL_NODE_SLICE:
			return n->p.slice.end - n->p.slice.start + 1;
		case LLHDL_NODE_CAT:
			return llhdl_get_vectorsize(n->p.cat.msb) + llhdl_get_vectorsize(n->p.cat.lsb);
		case LLHDL_NODE_SIGN:
			return llhdl_get_vectorsize(n->p.sign.source);
		case LLHDL_NODE_ARITH:
			switch(n->p.arith.op) {
				case LLHDL_ARITH_ADD:
				case LLHDL_ARITH_SUB:
					return max(llhdl_get_vectorsize(n->p.arith.a),
						llhdl_get_vectorsize(n->p.arith.b)) + 1;
				case LLHDL_ARITH_MUL:
					return llhdl_get_vectorsize(n->p.arith.a) + llhdl_get_vectorsize(n->p.arith.b);
				default:
					assert(0);
					break;
			}
		default:
			assert(0);
			break;
	}
}
