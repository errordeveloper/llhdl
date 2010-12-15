#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <netlist/symbol.h>

struct netlist_sym_store *netlist_sym_newstore()
{
	struct netlist_sym_store *store;

	store = malloc(sizeof(struct netlist_sym_store));
	assert(store != NULL);
	store->head = NULL;
	return store;
}

void netlist_sym_freestore(struct netlist_sym_store *store)
{
	struct netlist_sym *s, *s2;
	
	s = store->head;
	while(s != NULL) {
		s2 = s->next;
		free(s);
		s = s2;
	}
	free(store);
}

void netlist_sym_add(struct netlist_sym_store *store, unsigned int uid, char type, const char *name)
{
	int len;
	struct netlist_sym *s;

	len = strlen(name);
	s = malloc(sizeof(struct netlist_sym)+len+1);
	assert(s != NULL);
	s->next = store->head;
	s->uid = uid;
	s->type = type;
	memcpy(s->name, name, len);
	store->head = s;
}

int netlist_sym_lookup(struct netlist_sym_store *store, const char *name, char type, unsigned int *uid)
{
	struct netlist_sym *it;

	it = store->head;
	while(it != NULL) {
		if(((type == 0x00) || (it->type == type)) && (strcmp(name, it->name) == 0)) {
			if(uid != NULL)
				*uid = it->uid;
			return 1;
		}
		it = it->next;
	}
	return 0;
}

int netlist_sym_revlookup(struct netlist_sym_store *store, unsigned int uid, char **name, char *type)
{
	struct netlist_sym *it;

	it = store->head;
	while(it != NULL) {
		if(it->uid == uid) {
			if(name != NULL)
				*name = it->name;
			if(type != NULL)
				*type = it->type;
			return 1;
		}
		it = it->next;
	}
	return 0;
}

void netlist_sym_to_fd(struct netlist_sym_store *store, FILE *fd)
{
	struct netlist_sym *it;

	it = store->head;
	while(it != NULL) {
		fprintf(fd, "%08x %c %s\n", it->uid, it->type, it->name);
		it = it->next;
	}
}

void netlist_sym_to_file(struct netlist_sym_store *store, const char *filename)
{
	FILE *fd;
	int r;

	fd = fopen(filename, "w");
	assert(fd != NULL);
	netlist_sym_to_fd(store, fd);
	r = fclose(fd);
	assert(r == 0);
}

void netlist_sym_from_fd(struct netlist_sym_store *store, FILE *fd)
{
}

void netlist_sym_from_file(struct netlist_sym_store *store, const char *filename)
{
	FILE *fd;
	
	fd = fopen(filename, "r");
	assert(fd != NULL);
	netlist_sym_from_fd(store, fd);
	fclose(fd);
}
