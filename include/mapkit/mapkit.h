#ifndef __MAPKIT_MAPKIT_H
#define __MAPKIT_MAPKIT_H

#include <llhdl/structure.h>

/* Process callback. Called on every node in attempt to map them. */
typedef void (*mapkit_process_c)(struct llhdl_node **n, void *user);

/* Frees the process user pointer. This callback is optional and can be set to NULL. */
typedef void (*mapkit_free_c)(void *user);

struct mapkit_process_desc {
	mapkit_process_c process_c;
	mapkit_free_c free_c;
	void *user;
	struct mapkit_process_desc *next;
};

struct mapkit_result {
	int ninput_nodes;
	struct llhdl_node ***input_nodes;
	void **input_nets;
	void **output_nets;
};

/* Get the VCC or GND nets, selected by constant <v>.
 *  <user> is the user pointer from the mapkit_sc structure
 */
typedef void * (*mapkit_constant_c)(int v, void *user);

/* Retrieve the net corresponding to <bit> of the signal <n>.
 *  <output> set to 1 if connecting an output of <a>, 0 if connecting an input of <a>
 *  <user> is the user pointer from the mapkit_sc structure
 */
typedef void * (*mapkit_signal_c)(struct llhdl_node *n, int bit, void *user);

/* Join nets <a> and <b>.
 *  <user> is the user pointer from the mapkit_sc structure
 */
typedef void (*mapkit_join_c)(void *a, void *b, void *user);

struct mapkit_sc {
	struct llhdl_module *module;
	struct mapkit_process_desc *process_head;
	mapkit_constant_c constant_c;
	mapkit_signal_c signal_c;
	mapkit_join_c join_c;
	void *user;
};

#define MAPKIT_CALL_CONSTANT(_sc, v) ((_sc)->constant_c(v, (_sc)->user))
#define MAPKIT_CALL_SIGNAL(_sc, n, bit) ((_sc)->signal_c(n, bit, (_sc)->user))
#define MAPKIT_CALL_JOIN(_sc, a, b) ((_sc)->join_c(a, b, (_sc)->user))

struct mapkit_sc *mapkit_new(struct llhdl_module *module, mapkit_constant_c constant_c, mapkit_signal_c signal_c, mapkit_join_c join_c, void *user);
void mapkit_register_process(struct mapkit_sc *sc, mapkit_process_c process_c, mapkit_free_c free_c, void *user);
void mapkit_free(struct mapkit_sc *sc);

struct mapkit_result *mapkit_create_result(int ninput_nodes, int ninput_nets, int noutput_nets);
void mapkit_free_result(struct mapkit_result *r);
void mapkit_consume(struct mapkit_sc *sc, struct llhdl_node *n, struct mapkit_result *r);

void mapkit_metamap(struct mapkit_sc *sc);

void mapkit_interconnect_arc(struct mapkit_sc *sc, struct llhdl_node *n);

#endif /* __MAPKIT_MAPKIT_H */

