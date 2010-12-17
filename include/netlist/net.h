#ifndef __NETLIST_NET_H
#define __NETLIST_NET_H

enum {
	NETLIST_PRIMITIVE_INTERNAL,
	NETLIST_PRIMITIVE_PORT_OUT,
	NETLIST_PRIMITIVE_PORT_IN
};

struct netlist_primitive {
	int type;			/* < internal or I/O port */
	char *name;			/* < name of the primitive */
	int attribute_count;		/* < number of attributes */
	char **attribute_names;		/* < names of attributes */
	char **default_attributes;	/* < default values of attributes */
	int inputs;			/* < number of inputs */
	char **input_names;		/* < names of the inputs */
	int outputs;			/* < number of outputs */
	char **output_names;		/* < names of the outputs */
};

struct netlist_instance {
	unsigned int uid;		/* < unique identifier */
	struct netlist_primitive *p;	/* < what primitive we are an instance of */
	char **attributes;		/* < attributes of this instance */
	struct netlist_instance *next;	/* < next instance in this manager */
};

struct netlist_branch {
	struct netlist_instance *inst;	/* < target instance */
	int output;			/* < 1 if targeting an output */
	int pin_index;			/* < index of input/output */
	struct netlist_branch *next;	/* < next branch on this net */
};

struct netlist_net {
	unsigned int uid;		/* < unique identifier */
	struct netlist_branch *head;	/* < first branch on this net */
	struct netlist_net *next;	/* < next net in this manager */
};

/* low level functions, often not used directly */
struct netlist_instance *netlist_instantiate(unsigned int uid, struct netlist_primitive *p);
void netlist_free_instance(struct netlist_instance *inst);
void netlist_set_attribute(struct netlist_instance *inst, const char *attr, const char *value);

struct netlist_net *netlist_create_net(unsigned int uid);
void netlist_add_branch(struct netlist_net *net, struct netlist_instance *inst, int output, int pin_index);
void netlist_free_net(struct netlist_net *net);

#endif /* __NETLIST_NET_H */
