#ifndef __COMMONSTRUCT_H
#define __COMMONSTRUCT_H

#include <gmp.h>
#include <netlist/net.h>
#include "flow.h"

struct netlist_net *cs_constant_net(struct flow_sc *sc, int v);
struct netlist_instance *cs_create_lut(struct flow_sc *sc, int inputs, mpz_t contents);

#endif /* __COMMONSTRUCT_H */
