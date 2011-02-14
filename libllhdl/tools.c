#include <assert.h>
#include <gmp.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <util.h>

int llhdl_belongs_in_pure(int purity, int type)
{
	int is_connect;
	
	is_connect = (type == LLHDL_NODE_CONSTANT) || (type == LLHDL_NODE_SIGNAL) || (type == LLHDL_NODE_VECT);
	switch(purity) {
		case LLHDL_PURE_EMPTY:
		case LLHDL_COMPOUND:
		case LLHDL_PURE_CONNECT:
			return 1;
		case LLHDL_PURE_LOGIC:
			return (type == LLHDL_NODE_LOGIC) || (type == LLHDL_NODE_MUX) || is_connect;
		case LLHDL_PURE_FD:
			return (type == LLHDL_NODE_FD) || is_connect;
		case LLHDL_PURE_ARITH:
			return (type == LLHDL_NODE_ARITH) || is_connect;
		default:
			assert(0);
			return 0;
	}
}

void llhdl_update_purity(int *previous, int new)
{
	if(new == LLHDL_PURE_EMPTY) return;
	if(new == LLHDL_PURE_CONNECT) {
		if(*previous == LLHDL_PURE_EMPTY)
			*previous = LLHDL_PURE_CONNECT;
		return;
	}
	if((*previous == LLHDL_PURE_EMPTY) || (*previous == LLHDL_PURE_CONNECT))
		*previous = new;
	else if(*previous != new)
		*previous = LLHDL_COMPOUND;
}

void llhdl_update_purity_node(int *previous, int type)
{
	switch(type) {
		case LLHDL_NODE_CONSTANT:
		case LLHDL_NODE_SIGNAL:
		case LLHDL_NODE_VECT:
			llhdl_update_purity(previous, LLHDL_PURE_CONNECT);
			break;
		case LLHDL_NODE_LOGIC:
		case LLHDL_NODE_MUX:
			llhdl_update_purity(previous, LLHDL_PURE_LOGIC);
			break;
		case LLHDL_NODE_FD:
			llhdl_update_purity(previous, LLHDL_PURE_FD);
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
	int arity;
	int i;
	
	purity = LLHDL_PURE_EMPTY;
	if(n != NULL) {
		llhdl_update_purity_node(&purity, n->type);
		switch(n->type) {
			case LLHDL_NODE_CONSTANT:
			case LLHDL_NODE_SIGNAL:
			case LLHDL_NODE_VECT:
				break;
			case LLHDL_NODE_LOGIC:
				arity = llhdl_get_logic_arity(n->p.logic.op);
				for(i=0;i<arity;i++)
					llhdl_update_purity(&purity, llhdl_is_pure(n->p.logic.operands[i]));
				break;
			case LLHDL_NODE_MUX:
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.mux.select));
				for(i=0;i<n->p.mux.nsources;i++)
					llhdl_update_purity(&purity, llhdl_is_pure(n->p.mux.sources[i]));
			case LLHDL_NODE_FD:
				llhdl_update_purity(&purity, llhdl_is_pure(n->p.fd.data));
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
	int arity;
	int i;
	int sign;
	
	if(n == NULL) return 0;
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			return n->p.constant.sign;
		case LLHDL_NODE_SIGNAL:
			return n->p.signal.sign;
		case LLHDL_NODE_LOGIC:
			sign = 1;
			arity = llhdl_get_logic_arity(n->p.logic.op);
			for(i=0;i<arity;i++)
				if(!llhdl_get_sign(n->p.logic.operands[i]))
					sign = 0;
			return sign;
		case LLHDL_NODE_MUX:
			sign = 1;
			for(i=0;i<n->p.mux.nsources;i++)
				if(!llhdl_get_sign(n->p.mux.sources[i]))
					sign = 0;
			return sign;
		case LLHDL_NODE_FD:
			return llhdl_get_sign(n->p.fd.data);
		case LLHDL_NODE_VECT:
			return n->p.vect.sign;
		case LLHDL_NODE_ARITH:
			return llhdl_get_sign(n->p.arith.a) && llhdl_get_sign(n->p.arith.b);
		default:
			assert(0);
			break;
	}
}

int llhdl_get_vectorsize(struct llhdl_node *n)
{
	int arity;
	int v, count;
	int i;
	
	if(n == NULL) return 0;
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			return n->p.constant.vectorsize;
		case LLHDL_NODE_SIGNAL:
			return n->p.signal.vectorsize;
		case LLHDL_NODE_LOGIC:
			arity = llhdl_get_logic_arity(n->p.logic.op);
			count = 0;
			for(i=0;i<arity;i++) {
				v = llhdl_get_vectorsize(n->p.logic.operands[i]);
				if(v > count)
					count = v;
			}
			return count;
		case LLHDL_NODE_MUX:
			count = 0;
			for(i=0;i<n->p.mux.nsources;i++) {
				v = llhdl_get_vectorsize(n->p.mux.sources[i]);
				if(v > count)
					count = v;
			}
			return count;
		case LLHDL_NODE_FD:
			return llhdl_get_vectorsize(n->p.fd.data);
		case LLHDL_NODE_VECT:
			count = 0;
			for(i=0;i<n->p.vect.nslices;i++)
				count += n->p.vect.slices[i].end - n->p.vect.slices[i].start + 1;
			return count;
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
