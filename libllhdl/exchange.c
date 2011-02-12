#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>

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

enum {
	OP_INVALID,
	OP_MUX,
	OP_FD,
	OP_SLICE,
	OP_CAT,
	OP_SIGN,
	OP_UNSIGN,
	OP_ADD,
	OP_SUB,
	OP_MUL
};

static int str_to_op(const char *str)
{
	if(strcmp(str, "mux") == 0) return OP_MUX;
	if(strcmp(str, "fd") == 0) return OP_FD;
	if(strcmp(str, "slice") == 0) return OP_SLICE;
	if(strcmp(str, "cat") == 0) return OP_CAT;
	if(strcmp(str, "sign") == 0) return OP_SIGN;
	if(strcmp(str, "unsign") == 0) return OP_UNSIGN;
	if(strcmp(str, "add") == 0) return OP_ADD;
	if(strcmp(str, "sub") == 0) return OP_SUB;
	if(strcmp(str, "mul") == 0) return OP_MUL;
	return OP_INVALID;
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
	llhdl_create_signal(m, type, token, sign, vectorsize);
}

static struct llhdl_node *parse_constant(char *t)
{
	int sign;
	int vectorsize;
	mpz_t v;
	char *c;
	char *value;
	struct llhdl_node *n;

	sign = 1;
	c = strchr(t, 's');
	if(c == NULL) {
		sign = 0;
		c = strchr(t, 'u');
	}
	
	if(c == NULL) {
		vectorsize = 1;
		value = t;
	} else {
		*c = 0;
		value = c + 1;
		vectorsize = strtoul(t, &c, 0);
		if(*c != 0) {
			fprintf(stderr, "Invalid integer width value: %s\n", t);
			exit(EXIT_FAILURE);
		}
	}
	
	if(mpz_init_set_str(v, value, 0) != 0) {
		fprintf(stderr, "Invalid integer value: %s\n", value);
		exit(EXIT_FAILURE);
	}
	n = llhdl_create_constant(v, sign, vectorsize);
	mpz_clear(v);
	return n;
}

static struct llhdl_node *parse_expr(struct llhdl_module *m, char **saveptr);

static struct llhdl_node *parse_operator(struct llhdl_module *m, char *op, char **saveptr)
{
	int opc;
	struct llhdl_node *p1, *p2, *p3;

	opc = str_to_op(op);
	switch(opc) {
		case OP_MUX:
			p1 = parse_expr(m, saveptr);
			p2 = parse_expr(m, saveptr);
			p3 = parse_expr(m, saveptr);
			return llhdl_create_mux(p1, p2, p3);
		case OP_FD:
			p1 = parse_expr(m, saveptr);
			p2 = parse_expr(m, saveptr);
			return llhdl_create_fd(p1, p2);
		case OP_SLICE: {
			struct llhdl_node *n;
			int start, end;
			
			p1 = parse_expr(m, saveptr);
			p2 = parse_expr(m, saveptr);
			p3 = parse_expr(m, saveptr);
			
			if((p2->type != LLHDL_NODE_CONSTANT) || (p3->type != LLHDL_NODE_CONSTANT)) {
				fprintf(stderr, "Start and end of slice must be constant\n");
				exit(EXIT_FAILURE);
			}
			if(mpz_fits_slong_p(p2->p.constant.value))
				start = mpz_get_si(p2->p.constant.value);
			else
				start = -1;
			if(mpz_fits_slong_p(p3->p.constant.value))
				end = mpz_get_si(p3->p.constant.value);
			else
				end = -1;
			n = llhdl_create_slice(p1, start, end);
			llhdl_free_node(p2);
			llhdl_free_node(p3);
			return n;
		}
		case OP_CAT:
			p1 = parse_expr(m, saveptr);
			p2 = parse_expr(m, saveptr);
			return llhdl_create_cat(p1, p2);
		case OP_SIGN:
			p1 = parse_expr(m, saveptr);
			return llhdl_create_sign(p1, 1);
		case OP_UNSIGN:
			p1 = parse_expr(m, saveptr);
			return llhdl_create_sign(p1, 0);
		case OP_ADD:
			p1 = parse_expr(m, saveptr);
			p2 = parse_expr(m, saveptr);
			return llhdl_create_arith(LLHDL_ARITH_ADD, p1, p2);
		case OP_SUB:
			p1 = parse_expr(m, saveptr);
			p2 = parse_expr(m, saveptr);
			return llhdl_create_arith(LLHDL_ARITH_SUB, p1, p2);
		case OP_MUL:
			p1 = parse_expr(m, saveptr);
			p2 = parse_expr(m, saveptr);
			return llhdl_create_arith(LLHDL_ARITH_MUL, p1, p2);
		default:
			fprintf(stderr, "Unknown operator: %s\n", op);
			exit(EXIT_FAILURE);
			return NULL;
	}
}

static struct llhdl_node *parse_expr(struct llhdl_module *m, char **saveptr)
{
	char *token;
	char type;

	token = strtok_r(NULL, delims, saveptr);
	if(token == NULL) {
		fprintf(stderr, "Unexpected end of expression\n");
		exit(EXIT_FAILURE);
	}
	type = *token;
	switch(type) {
		case '0'...'9':
			return parse_constant(token);
		case '#':
			token++;
			return parse_operator(m, token, saveptr);
		default: {
			struct llhdl_node *n;

			n = llhdl_find_signal(m, token);
			if(n == NULL) {
				fprintf(stderr, "Reference to unknown signal: %s\n", token);
				exit(EXIT_FAILURE);
			}
			return n;
		}
	}
}

static void parse_assign(struct llhdl_module *m, char **saveptr)
{
	char *token;
	struct llhdl_node *target_signal;

	token = strtok_r(NULL, delims, saveptr);
	if(token == NULL) {
		fprintf(stderr, "Unexpected end of line\n");
		exit(EXIT_FAILURE);
	}
	target_signal = llhdl_find_signal(m, token);
	if(target_signal == NULL) {
		fprintf(stderr, "Assignment to unknown signal %s\n", token);
		exit(EXIT_FAILURE);
	}
	if(target_signal->p.signal.source != NULL) {
		fprintf(stderr, "Conflicting assignments on signal %s\n", token);
		exit(EXIT_FAILURE);
	}
	target_signal->p.signal.source = parse_expr(m, saveptr);
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
			parse_assign(m, &saveptr);
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

static void write_expr(FILE *fd, struct llhdl_node *n)
{
	fprintf(fd, " ");
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			if(n->p.constant.sign || (n->p.constant.vectorsize != 1))
				fprintf(fd, "%d%c", n->p.constant.sign ? 's' : 'u', n->p.constant.vectorsize);
			mpz_out_str(fd, 10, n->p.constant.value);
			break;
		case LLHDL_NODE_SIGNAL:
			fprintf(fd, "%s", n->p.signal.name);
			break;
		case LLHDL_NODE_MUX:
			fprintf(fd, "#mux");
			write_expr(fd, n->p.mux.sel);
			write_expr(fd, n->p.mux.negative);
			write_expr(fd, n->p.mux.positive);
			break;
		case LLHDL_NODE_FD:
			fprintf(fd, "#fd");
			write_expr(fd, n->p.fd.clock);
			write_expr(fd, n->p.fd.data);
			break;
		case LLHDL_NODE_SLICE:
			fprintf(fd, "#slice");
			write_expr(fd, n->p.slice.source);
			fprintf(fd, " %d %d", n->p.slice.start, n->p.slice.end);
			break;
		case LLHDL_NODE_CAT:
			fprintf(fd, "#cat");
			write_expr(fd, n->p.cat.msb);
			write_expr(fd, n->p.cat.lsb);
			break;
		case LLHDL_NODE_SIGN:
			if(n->p.sign.sign)
				fprintf(fd, "#sign");
			else
				fprintf(fd, "#unsign");
			write_expr(fd, n->p.sign.source);
			break;
		case LLHDL_NODE_ARITH:
			switch(n->p.arith.op) {
				case LLHDL_ARITH_ADD:
					fprintf(fd, "#add");
					break;
				case LLHDL_ARITH_SUB:
					fprintf(fd, "#sub");
					break;
				case LLHDL_ARITH_MUL:
					fprintf(fd, "#mul");
					break;
				default:
					assert(0);
					break;
			}
			write_expr(fd, n->p.arith.a);
			write_expr(fd, n->p.arith.b);
			break;
	}
}

static void write_assignments(struct llhdl_module *m, FILE *fd)
{
	struct llhdl_node *n;

	n = m->head;
	while(n != NULL) {
		assert(n->type == LLHDL_NODE_SIGNAL);
		if(n->p.signal.source != NULL) {
			fprintf(fd, "assign %s", n->p.signal.name);
			write_expr(fd, n->p.signal.source);
			fprintf(fd, "\n");
		}
		n = n->p.signal.next;
	}
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
	if(fd == NULL) {
		perror("llhdl_parse_file");
		exit(EXIT_FAILURE);
	}
	m = llhdl_parse_fd(fd);
	fclose(fd);
	return m;
}

void llhdl_write_file(struct llhdl_module *m, const char *filename)
{
	FILE *fd;
	int r;

	fd = fopen(filename, "w");
	if(fd == NULL) {
		perror("llhdl_write_file");
		exit(EXIT_FAILURE);
	}
	llhdl_write_fd(m, fd);
	r = fclose(fd);
	assert(r == 0);
}
