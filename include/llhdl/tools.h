#ifndef __LLHDL_TOOLS_H
#define __LLHDL_TOOLS_H

#include <llhdl/structure.h>

const char *llhdl_strtype(int type);
const char *llhdl_strlogic(int op);
const char *llhdl_strarith(int op);

struct llhdl_node *llhdl_dup(struct llhdl_node *n);
int llhdl_equiv(struct llhdl_node *a, struct llhdl_node *b);

int llhdl_get_sign(struct llhdl_node *n);
int llhdl_get_vectorsize(struct llhdl_node *n);

typedef int (*llhdl_walk_c)(struct llhdl_node **n, void *user);
int llhdl_walk(llhdl_walk_c walk_c, void *user, struct llhdl_node **n);
int llhdl_walk_module(llhdl_walk_c walk_c, void *user, struct llhdl_module *m);

void llhdl_clear_clocks(struct llhdl_module *m);
void llhdl_identify_clocks(struct llhdl_module *m);
void llhdl_mark_clock(struct llhdl_node *n, int c);
int llhdl_is_clock(struct llhdl_node *n);

#endif /* __LLHDL_TOOLS_H */

