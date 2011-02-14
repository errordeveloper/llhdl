#ifndef __RG_H
#define __RG_H

#include <llhdl/structure.h>
#include <tilm/tilm.h>

struct tilm_rg_input_assoc {
	struct tilm_input_assoc payload;
	struct tilm_rg_input_assoc *next;
};

struct tilm_rg {
	struct tilm_rg_input_assoc *head;
};

struct tilm_rg *tilm_rg_new();
void tilm_rg_free(struct tilm_rg *rg);

void tilm_rg_add(struct tilm_rg *rg, struct llhdl_node *signal, int bit, void *lut, int n);
int tilm_rg_count(struct tilm_rg *rg);
struct tilm_result *tilm_rg_generate(struct tilm_rg *rg, void **out_luts, int vectorsize);

#endif /* __RG_H */

