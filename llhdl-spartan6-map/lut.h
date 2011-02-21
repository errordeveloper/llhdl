#ifndef __LUT_H
#define __LUT_H

#include "flow.h"

struct netlist_instance *lut_create(struct flow_sc *sc, int inputs, mpz_t contents);
void lut_register(struct flow_sc *sc);

#endif /* __LUT_H */

