#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <util.h>

#include <netlist/symbol.h>

struct netlist_sym_store *netlist_sym_newstore()
{
	struct netlist_sym_store *store;

	store = alloc_type(struct netlist_sym_store);
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

struct netlist_sym *netlist_sym_add(struct netlist_sym_store *store, unsigned int uid, char type, const char *name)
{
	int len;
	struct netlist_sym *s;

	len = strlen(name);
	s = alloc_size(sizeof(struct netlist_sym)+len+1);
	s->next = store->head;
	s->user = NULL;
	s->uid = uid;
	s->type = type;
	memcpy(s->name, name, len+1);
	store->head = s;
	return s;
}

struct netlist_sym *netlist_sym_lookup(struct netlist_sym_store *store, const char *name, char type)
{
	struct netlist_sym *it;

	it = store->head;
	while(it != NULL) {
		if(((type == 0x00) || (it->type == type)) && (strcmp(name, it->name) == 0))
			return it;
		it = it->next;
	}
	return NULL;
}

struct netlist_sym *netlist_sym_revlookup(struct netlist_sym_store *store, unsigned int uid)
{
	struct netlist_sym *it;

	it = store->head;
	while(it != NULL) {
		if(it->uid == uid)
			return it;
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
	if(fd == NULL) {
		perror("netlist_sym_to_file");
		exit(EXIT_FAILURE);
	}
	netlist_sym_to_fd(store, fd);
	r = fclose(fd);
	if(r != 0) {
		perror("netlist_sym_to_file");
		exit(EXIT_FAILURE);
	}
}

void netlist_sym_from_fd(struct netlist_sym_store *store, FILE *fd)
{
	unsigned int uid;
	char type;
	char *name;
	int n;
	
	while(1) {
		n = fscanf(fd, "%x %c %as", &uid, &type, &name);
		if(n != 3) {
			if(!feof(fd)) {
				perror("netlist_sym_from_fd");
				exit(EXIT_FAILURE);
			}
			break;
		}
		netlist_sym_add(store, uid, type, name);
		free(name);
	}
}

void netlist_sym_from_file(struct netlist_sym_store *store, const char *filename)
{
	FILE *fd;
	
	fd = fopen(filename, "r");
	if(fd == NULL) {
		perror("netlist_sym_from_file");
		exit(EXIT_FAILURE);
	}
	netlist_sym_from_fd(store, fd);
	fclose(fd);
}
