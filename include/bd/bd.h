#ifndef __BD_H
#define __BD_H

#include <llhdl/structure.h>

enum {
	BD_COMPOUND,

	BD_PURE_EMPTY,
	BD_PURE_CONNECT,

	BD_PURE_LOGIC,
	BD_PURE_FD,
	BD_PURE_ARITH
};

int bd_belongs_in_pure(int purity, int type);
void bd_update_purity(int *previous, int new);
void bd_update_purity_node(int *previous, int type);
int bd_is_pure(struct llhdl_node *n);

#define BD_CAPABILITY_ADD	(0x00000001)
#define BD_CAPABILITY_SUB	(0x00000002)
#define BD_CAPABILITY_MUL	(0x00000004)

void bd_replace(struct llhdl_module *m, unsigned int capabilities);
void bd_purify(struct llhdl_module *m);
void bd_flow(struct llhdl_module *m, unsigned int capabilities);

#endif /* __BD_H */

