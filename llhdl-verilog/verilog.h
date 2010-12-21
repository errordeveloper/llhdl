#ifndef __VERILOG_H
#define __VERILOG_H

struct verilog_constant {
	int vectorsize;
	int sign;
	long long int value;
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
	struct verilog_assignment *next;
};

/* TODO: assignment list should be replaced by statement list (for if, case, etc.) */
struct verilog_process {
	struct verilog_signal *clock;		/* < NULL if combinatorial */
	struct verilog_assignment *head;
	struct verilog_assignment *tail;	/* < tail of the assignment list, for speedy add at the end */
	struct verilog_process *next;
};

struct verilog_module {
	char *name;
	struct verilog_signal *shead;
	struct verilog_process *phead;
};

struct verilog_constant *verilog_new_constant(int vectorsize, int sign, long long int value);
struct verilog_constant *verilog_new_constant_str(char *str);

struct verilog_signal *verilog_find_signal(struct verilog_module *m, const char *signal);
struct verilog_signal *verilog_new_update_signal(struct verilog_module *m, int type, const char *name, int vectorsize, int sign);
void verilog_free_signal(struct verilog_module *m, struct verilog_signal *s);

int verilog_get_node_arity(int type);
struct verilog_node *verilog_new_constant_node(struct verilog_constant *constant);
struct verilog_node *verilog_new_signal_node(struct verilog_signal *signal);
struct verilog_node *verilog_new_op_node(int type);

struct verilog_assignment *verilog_new_assignment(struct verilog_signal *target, int blocking, struct verilog_node *source);

struct verilog_process *verilog_new_process_assign(struct verilog_module *m, struct verilog_assignment *a);
void verilog_free_process(struct verilog_module *m, struct verilog_process *p);

struct verilog_module *verilog_new_module();
void verilog_set_module_name(struct verilog_module *m, const char *name);
void verilog_free_module(struct verilog_module *m);

struct verilog_module *verilog_parse_fd(FILE *fd);
struct verilog_module *verilog_parse_file(const char *filename);

#endif /* __VERILOG_H */
