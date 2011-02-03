#ifndef __VERILOG_H
#define __VERILOG_H

#include <gmp.h>

struct verilog_constant {
	int vectorsize;
	int sign;
	mpz_t value;
};

enum {
	VERILOG_SIGNAL_REGWIRE,
	VERILOG_SIGNAL_OUTPUT,
	VERILOG_SIGNAL_INPUT
};

struct verilog_signal {
	int type;
	int vectorsize;
	int sign;
	struct verilog_signal *next;
	void *user;
	char name[];
};

enum {
	VERILOG_NODE_CONSTANT,
	VERILOG_NODE_SIGNAL,
	VERILOG_NODE_EQL,
	VERILOG_NODE_NEQ,
	VERILOG_NODE_OR,
	VERILOG_NODE_AND,
	VERILOG_NODE_TILDE,
	VERILOG_NODE_XOR,
	VERILOG_NODE_ALT,
};

struct verilog_node {
	int type;
	void *branches[];
};

struct verilog_assignment {
	struct verilog_signal *target;
	int blocking;
	struct verilog_node *source;
};

struct verilog_statement;

struct verilog_condition {
	struct verilog_node *condition;
	struct verilog_statement *negative;
	struct verilog_statement *positive;
};

enum {
	VERILOG_STATEMENT_ASSIGNMENT,
	VERILOG_STATEMENT_CONDITION
};

struct verilog_statement {
	int type;
	struct verilog_statement *next;		/* < next statement at this level */
	union {
		struct verilog_assignment assignment;
		struct verilog_condition condition;
	} p;
};

struct verilog_process {
	struct verilog_signal *clock;		/* < NULL if combinatorial */
	struct verilog_statement *head;		/* < first statement in this process */
	struct verilog_process *next;
};

struct verilog_module {
	char *name;
	struct verilog_signal *shead;
	struct verilog_process *phead;
};

/* structure manipulation */
struct verilog_constant *verilog_new_constant(int vectorsize, int sign, mpz_t value);
struct verilog_constant *verilog_new_constant_str(char *str);
void verilog_free_constant(struct verilog_constant *c);

struct verilog_signal *verilog_find_signal(struct verilog_module *m, const char *signal);
struct verilog_signal *verilog_new_update_signal(struct verilog_module *m, int type, const char *name, int vectorsize, int sign);
void verilog_free_signal(struct verilog_module *m, struct verilog_signal *s);
void verilog_free_signal_list(struct verilog_signal *head);

int verilog_get_node_arity(int type);
struct verilog_node *verilog_new_constant_node(struct verilog_constant *constant);
struct verilog_node *verilog_new_signal_node(struct verilog_signal *signal);
struct verilog_node *verilog_new_op_node(int type);
void verilog_free_node(struct verilog_node *n);

struct verilog_statement *verilog_new_assignment(struct verilog_signal *target, int blocking, struct verilog_node *source);
struct verilog_statement *verilog_new_condition(struct verilog_node *condition, struct verilog_statement *negative, struct verilog_statement *positive);
void verilog_free_statement(struct verilog_statement *s);
void verilog_free_statement_list(struct verilog_statement *head);

struct verilog_process *verilog_new_process(struct verilog_module *m, struct verilog_signal *clock, struct verilog_statement *head);
void verilog_free_process(struct verilog_module *m, struct verilog_process *p);
void verilog_free_process_list(struct verilog_process *head);

struct verilog_module *verilog_new_module();
void verilog_set_module_name(struct verilog_module *m, const char *name);
void verilog_free_module(struct verilog_module *m);

/* parsing */
struct verilog_module *verilog_parse_fd(FILE *fd);
struct verilog_module *verilog_parse_file(const char *filename);

/* dump (for debugging) */
void verilog_dump_signal_list(int level, struct verilog_signal *head);
void verilog_dump_node(struct verilog_node *n);
void verilog_dump_statement_list(int level, struct verilog_statement *head);
void verilog_dump_process_list(int level, struct verilog_process *head);
void verilog_dump_module(int level, struct verilog_module *m);

/* utility functions */
enum {
	VERILOG_BL_EMPTY,
	VERILOG_BL_BLOCKING,
	VERILOG_BL_NONBLOCKING,
	VERILOG_BL_MIXED
};

int verilog_statements_blocking(struct verilog_statement *s);
int verilog_process_blocking(struct verilog_process *p);

#endif /* __VERILOG_H */
