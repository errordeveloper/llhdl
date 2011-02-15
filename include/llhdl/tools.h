#ifndef __LLHDL_TOOLS_H
#define __LLHDL_TOOLS_H

#include <llhdl/structure.h>

enum {
	LLHDL_COMPOUND,

	LLHDL_PURE_EMPTY,
	LLHDL_PURE_CONNECT,

	LLHDL_PURE_LOGIC,
	LLHDL_PURE_FD,
	LLHDL_PURE_ARITH
};

int llhdl_belongs_in_pure(int purity, int type);
void llhdl_update_purity(int *previous, int new);
void llhdl_update_purity_node(int *previous, int type);
int llhdl_is_pure(struct llhdl_node *n);

struct llhdl_node *llhdl_dup(struct llhdl_node *n);
int llhdl_equiv(struct llhdl_node *a, struct llhdl_node *b);

int llhdl_get_sign(struct llhdl_node *n);
int llhdl_get_vectorsize(struct llhdl_node *n);

#endif /* __LLHDL_TOOLS_H */

