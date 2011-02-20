#ifndef __VARIABLES_H
#define __VARIABLES_H

#include "internal.h"

struct tilm_variable {
	struct llhdl_node *n;
	int bit;
	int value;
	struct tilm_variable *next;
};

struct tilm_variables {
	int vectorsize;
	struct tilm_variable **heads;
};

struct tilm_variables *tilm_variables_enumerate(struct llhdl_node *n);
void tilm_variables_dump(struct tilm_variables *r);
int tilm_variables_remaining(struct tilm_variable *v);
int tilm_variable_value(struct tilm_variables *r, int obit, struct llhdl_node *n, int bit);
void tilm_variables_free(struct tilm_variables *r);

#endif

