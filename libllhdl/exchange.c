#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <llhdl/structure.h>
#include <llhdl/exchange.h>

enum {
	CMD_INVALID,
	CMD_NONE,
	CMD_MODULE,
	CMD_INPUT,
	CMD_OUTPUT,
	CMD_SIGNAL,
	CMD_ASSIGN
};

static int str_to_cmd(const char *str)
{
	if(str == NULL) return CMD_NONE;
	if(strcmp(str, "-") == 0) return CMD_NONE;
	if(strcmp(str, "module") == 0) return CMD_MODULE;
	if(strcmp(str, "input") == 0) return CMD_INPUT;
	if(strcmp(str, "output") == 0) return CMD_OUTPUT;
	if(strcmp(str, "signal") == 0) return CMD_SIGNAL;
	if(strcmp(str, "assign") == 0) return CMD_ASSIGN;
	return CMD_INVALID;
}

static const char delims[] = " \t\n";

static void parse_module(struct llhdl_module *m, char **saveptr)
{
	char *token;

	token = strtok_r(NULL, delims, saveptr);
	if(token == NULL) {
		fprintf(stderr, "Unexpected end of line\n");
		exit(EXIT_FAILURE);
	}
	llhdl_set_module_name(m, token);
}

static void parse_vectorsize_sign(int *vectorsize_affected, int *sign_affected, int *vectorsize, int *sign, char **saveptr)
{
	char *token;

	token = strtok_r(NULL, delims, saveptr);
	if(token == NULL)
		return;
	if(strcmp(token, "signed") == 0) {
		if(*sign_affected) {
			fprintf(stderr, "Redundant sign qualifier\n");
			exit(EXIT_FAILURE);
		}
		*sign_affected = 1;
		*sign = 1;
	} else {
		char *c;
		
		*vectorsize = strtoul(token, &c, 0);
		if(*c != 0) {
			fprintf(stderr, "Invalid sign or vector size qualifier: %s\n", token);
			exit(EXIT_FAILURE);
		}
		if(*vectorsize_affected) {
			fprintf(stderr, "Redundant vector size qualifier\n");
			exit(EXIT_FAILURE);
		}
		*vectorsize_affected = 1;
	}
}

static void parse_signal(struct llhdl_module *m, char **saveptr, int type)
{
	char *token;
	int vectorsize_affected, sign_affected;
	int vectorsize, sign;

	token = strtok_r(NULL, delims, saveptr);
	if(token == NULL) {
		fprintf(stderr, "Unexpected end of line\n");
		exit(EXIT_FAILURE);
	}
	vectorsize_affected = 0;
	sign_affected = 0;
	vectorsize = 1;
	sign = 0;
	parse_vectorsize_sign(&vectorsize_affected, &sign_affected, &vectorsize, &sign, saveptr);
	parse_vectorsize_sign(&vectorsize_affected, &sign_affected, &vectorsize, &sign, saveptr);
	llhdl_create_signal(m, type, sign, token, vectorsize);
}

static void parse_line(struct llhdl_module *m, char *line)
{
	char *saveptr;
	char *str;
	int command;

	str = strtok_r(line, delims, &saveptr);
	command = str_to_cmd(str);
	switch(command) {
		case CMD_NONE:
			return;
		case CMD_MODULE:
			parse_module(m, &saveptr);
			break;
		case CMD_INPUT:
			parse_signal(m, &saveptr, LLHDL_SIGNAL_PORT_IN);
			break;
		case CMD_OUTPUT:
			parse_signal(m, &saveptr, LLHDL_SIGNAL_PORT_OUT);
			break;
		case CMD_SIGNAL:
			parse_signal(m, &saveptr, LLHDL_SIGNAL_INTERNAL);
			break;
		case CMD_ASSIGN:
			/* TODO */
			return;
		default:
			fprintf(stderr, "Invalid command: %s\n", str);
			exit(EXIT_FAILURE);
			break;
	}
	str = strtok_r(NULL, delims, &saveptr);
	if(str != NULL) {
		fprintf(stderr, "Expected end of line, got token: %s\n", str);
		exit(EXIT_FAILURE);
	}
}

struct llhdl_module *llhdl_parse_fd(FILE *fd)
{
	struct llhdl_module *m;
	int r;
	char *line;
	size_t linesize;

	m = llhdl_new_module();
	
	line = NULL;
	linesize = 0;
	while(1) {
		r = getline(&line, &linesize, fd);
		if(r == -1) {
			assert(feof(fd));
			break;
		}
		parse_line(m, line);
	}
	free(line);

	return m;
}

static void write_signals(struct llhdl_module *m, FILE *fd)
{
	struct llhdl_node *n;

	n = m->head;
	while(n != NULL) {
		assert(n->type == LLHDL_NODE_SIGNAL);
		switch(n->p.signal.type) {
			case LLHDL_SIGNAL_INTERNAL:
				fprintf(fd, "signal ");
				break;
			case LLHDL_SIGNAL_PORT_OUT:
				fprintf(fd, "output ");
				break;
			case LLHDL_SIGNAL_PORT_IN:
				fprintf(fd, "input ");
		}
		fprintf(fd, "%s", n->p.signal.name);
		if(n->p.signal.vectorsize > 1)
			fprintf(fd, " %d", n->p.signal.vectorsize);
		if(n->p.signal.sign)
			fprintf(fd, " signed");
		fprintf(fd, "\n");
		n = n->p.signal.next;
	}
}

static void write_assignments(struct llhdl_module *m, FILE *fd)
{
	/* TODO */
}

void llhdl_write_fd(struct llhdl_module *m, FILE *fd)
{
	if(m->name != NULL)
		fprintf(fd, "module %s\n", m->name);
	write_signals(m, fd);
	write_assignments(m, fd);
}

struct llhdl_module *llhdl_parse_file(const char *filename)
{
	FILE *fd;
	struct llhdl_module *m;

	fd = fopen(filename, "r");
	assert(fd != NULL);
	m = llhdl_parse_fd(fd);
	fclose(fd);
	return m;
}

void llhdl_write_file(struct llhdl_module *m, const char *filename)
{
	FILE *fd;
	int r;

	fd = fopen(filename, "w");
	assert(fd != NULL);
	llhdl_write_fd(m, fd);
	r = fclose(fd);
	assert(r == 0);
}
