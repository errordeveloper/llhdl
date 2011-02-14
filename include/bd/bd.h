#ifndef __BD_H
#define __BD_H

#include <llhdl/structure.h>

#define BD_CAPABILITY_ADD	(0x00000001)
#define BD_CAPABILITY_SUB	(0x00000002)
#define BD_CAPABILITY_MUL	(0x00000004)

void bd_replace(struct llhdl_module *m, unsigned int capabilities);
void bd_purify(struct llhdl_module *m);
void bd_flow(struct llhdl_module *m, unsigned int capabilities);

#endif /* __BD_H */

