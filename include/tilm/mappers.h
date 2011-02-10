#ifndef __TILM_MAPPERS_H
#define __TILM_MAPPERS_H

#include <tilm/tilm.h>
#include <llhdl/structure.h>

struct tilm_result *tilm_shannon_map(struct tilm_param *p, struct llhdl_node *top);
struct tilm_result *tilm_bdspga_map(struct tilm_param *p, struct llhdl_node *top);

#endif /* __TILM_MAPPERS_H */

