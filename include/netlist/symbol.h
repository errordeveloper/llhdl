#ifndef __NETLIST_SYMBOL_H
#define __NETLIST_SYMBOL_H

#include <stdio.h>

struct netlist_sym {
	struct netlist_sym *next;
	void *user;
	unsigned int uid;
	char type;
	char name[];
};

struct netlist_sym_store {
	struct netlist_sym *head;
};

struct netlist_sym_store *netlist_sym_newstore();
void netlist_sym_freestore(struct netlist_sym_store *store);

struct netlist_sym *netlist_sym_add(struct netlist_sym_store *store, unsigned int uid, char type, const char *name);
/* lookup by name and type (type=0 matches any) */
struct netlist_sym *netlist_sym_lookup(struct netlist_sym_store *store, const char *name, char type);
/* lookup by uid */
struct netlist_sym *netlist_sym_revlookup(struct netlist_sym_store *store, unsigned int uid);

void netlist_sym_to_fd(struct netlist_sym_store *store, FILE *fd);
void netlist_sym_to_file(struct netlist_sym_store *store, const char *filename);

void netlist_sym_from_fd(struct netlist_sym_store *store, FILE *fd);
void netlist_sym_from_file(struct netlist_sym_store *store, const char *filename);

#endif /* __NETLIST_SYMBOL_H */
