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

struct netlist_instance;

struct netlist_net {
	unsigned int uid;		/* < unique identifier */
	struct netlist_instance *dest;	/* < destination instance of this net */
	int input;			/* < input port of the destination instance */
	struct netlist_net *next;	/* < next net on this output port */
};

struct netlist_instance {
	unsigned int uid;		/* < unique identifier */
	struct netlist_primitive *p;	/* < what primitive we are an instance of */
	char **attributes;		/* < attributes of this instance */
	struct netlist_net **outputs;	/* < table of nets connected to outputs */
	struct netlist_instance *next;	/* < next instance in this manager */
};

/* low level functions, typically not used directly */
struct netlist_instance *netlist_instantiate(unsigned int uid, struct netlist_primitive *p);
void netlist_free_instance(struct netlist_instance *inst);
void netlist_connect(unsigned int uid, struct netlist_instance *src, int output, struct netlist_instance *dest, int input);

#endif /* __NETLIST_NET_H */
