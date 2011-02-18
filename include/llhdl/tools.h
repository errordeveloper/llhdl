#ifndef __LLHDL_TOOLS_H
#define __LLHDL_TOOLS_H

#include <llhdl/structure.h>

struct llhdl_node *llhdl_dup(struct llhdl_node *n);
int llhdl_equiv(struct llhdl_node *a, struct llhdl_node *b);

int llhdl_get_sign(struct llhdl_node *n);
int llhdl_get_vectorsize(struct llhdl_node *n);

#endif /* __LLHDL_TOOLS_H */

