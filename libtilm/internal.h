#ifndef __INTERNAL_H
#define __INTERNAL_H

#include <tilm/tilm.h>

struct tilm_sc {
	struct mapkit_sc *mapkit;
	int mapper_id;
	int max_inputs;
	void *extra_mapper_param;
	tilm_create_net_c create_net;
	tilm_create_lut_c create_lut;
	tilm_create_mux_c create_mux;
	void *user;
};

#define TILM_CALL_CREATE_NET(_sc)			((_sc)->create_net((_sc)->user))
#define TILM_CALL_CREATE_LUT(_sc, inputs, contents)	((_sc)->create_lut(inputs, contents, (_sc)->user))
#define TILM_CALL_CREATE_MUX(_sc, muxlevel, user)	((_sc)->create_mux(muxlevel, (_sc)->user))

/* Mapkit callbacks */
#define TILM_CALL_CONSTANT(_sc, v)			((_sc)->mapkit->constant(v,(_sc)->mapkit->user))
#define TILM_CALL_SIGNAL(_sc, n, bit)			((_sc)->mapkit->signal_c(n, bit, (_sc)->mapkit->user))
#define TILM_CALL_JOIN(_sc, a, b)			((_sc)->mapkit->join_c(a, b, (_sc)->mapkit->user))

void tilm_process_shannon(struct tilm_sc *sc, struct llhdl_node **n);
void tilm_process_bdspga(struct tilm_sc *sc, struct llhdl_node **n);

#endif /* __INTERNAL_H */
