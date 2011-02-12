#ifndef __LLHDL_STRUCTURE_H
#define __LLHDL_STRUCTURE_H

#include <gmp.h>

struct llhdl_node;
struct llhdl_module;

struct llhdl_node_constant {
	int sign;
	int vectorsize;
	mpz_t value;
};

enum {
	LLHDL_SIGNAL_INTERNAL,
	LLHDL_SIGNAL_PORT_OUT,
	LLHDL_SIGNAL_PORT_IN
};

struct llhdl_node_signal {
	int type;
	int sign;
	int vectorsize;
	struct llhdl_node *source;
	struct llhdl_node *next;
	char name[];
};

struct llhdl_node_mux {
	struct llhdl_node *sel;
	struct llhdl_node *negative;
	struct llhdl_node *positive;
};

struct llhdl_node_fd {
	struct llhdl_node *clock;
	struct llhdl_node *data;
};

struct llhdl_node_slice {
	struct llhdl_node *source;
	int start;
	int end;
};

struct llhdl_node_cat {
	struct llhdl_node *msb;
	struct llhdl_node *lsb;
};

struct llhdl_node_sign {
	struct llhdl_node *source;
	int sign;
};

enum {
	LLHDL_ARITH_ADD,
	LLHDL_ARITH_SUB,
	LLHDL_ARITH_MUL
};

struct llhdl_node_arith {
	int op;
	struct llhdl_node *a;
	struct llhdl_node *b;
};

enum {
	LLHDL_NODE_CONSTANT,
	LLHDL_NODE_SIGNAL,
	LLHDL_NODE_MUX,
	LLHDL_NODE_FD,
	LLHDL_NODE_SLICE,
	LLHDL_NODE_CAT,
	LLHDL_NODE_SIGN,
	LLHDL_NODE_ARITH
};

struct llhdl_node {
	int type;
	union {
		struct llhdl_node_constant constant;
		struct llhdl_node_signal signal;
		struct llhdl_node_mux mux;
		struct llhdl_node_fd fd;
		struct llhdl_node_slice slice;
		struct llhdl_node_cat cat;
		struct llhdl_node_sign sign;
		struct llhdl_node_arith arith;
	} p;
};

struct llhdl_module {
	char *name;
	struct llhdl_node *head;
};

struct llhdl_module *llhdl_new_module();
void llhdl_free_module(struct llhdl_module *m);
void llhdl_set_module_name(struct llhdl_module *m, const char *name);

struct llhdl_node *llhdl_create_constant(mpz_t value, int sign, int vectorsize);
struct llhdl_node *llhdl_create_signal(struct llhdl_module *m, int type, const char *name, int sign, int vectorsize);
struct llhdl_node *llhdl_create_mux(struct llhdl_node *sel, struct llhdl_node *negative, struct llhdl_node *positive);
struct llhdl_node *llhdl_create_fd(struct llhdl_node *clock, struct llhdl_node *data);
struct llhdl_node *llhdl_create_slice(struct llhdl_node *source, int start, int end);
struct llhdl_node *llhdl_create_cat(struct llhdl_node *msb, struct llhdl_node *lsb);
struct llhdl_node *llhdl_create_sign(struct llhdl_node *source, int sign);
struct llhdl_node *llhdl_create_arith(int op, struct llhdl_node *a, struct llhdl_node *b);
void llhdl_free_node(struct llhdl_node *n); /* < does not free signals */
void llhdl_free_signal(struct llhdl_node *n);

struct llhdl_node *llhdl_find_signal(struct llhdl_module *m, const char *name);

#endif /* __LLHDL_STRUCTURE_H */
