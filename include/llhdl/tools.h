#ifndef __LLHDL_TOOLS_H
#define __LLHDL_TOOLS_H

#include <llhdl/structure.h>

enum {
	LLHDL_COMPOUND,

	LLHDL_PURE_EMPTY,
	LLHDL_PURE_SIGNAL, /* Signals can be part of everything */
	LLHDL_PURE_CONSTANT, /* Constants can be part of all BDD, vector and arithmetic arcs */
	
	LLHDL_PURE_BDD,
	LLHDL_PURE_FD,
	LLHDL_PURE_VECTOR,
	LLHDL_PURE_ARITH
};

int llhdl_is_pure(struct llhdl_node *n);

#endif /* __LLHDL_TOOLS_H */

