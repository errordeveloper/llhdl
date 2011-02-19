#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>

#include <util.h>

#include <llhdl/structure.h>
#include <llhdl/exchange.h>

enum {
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
	fprintf(stderr, "Invalid command: %s\n", str);
	exit(EXIT_FAILURE);
	return 0;
}

enum {
	OP_NOT,
	OP_AND,
	OP_OR,
	OP_XOR,
	OP_MUX,
	OP_FD,
	OP_VECT,
	OP_ADD,
	OP_SUB,
	OP_MUL
};

static int str_to_op(const char *str)
{
	if(strcmp(str, "not") == 0) return OP_NOT;
	if(strcmp(str, "and") == 0) return OP_AND;
	if(strcmp(str, "or") == 0) return OP_OR;
	if(strcmp(str, "xor") == 0) return OP_XOR;
	if(strcmp(str, "mux") == 0) return OP_MUX;
	if(strcmp(str, "fd") == 0) return OP_FD;
	if(strcmp(str, "vect") == 0) return OP_VECT;
	if(strcmp(str, "add") == 0) return OP_ADD;
	if(strcmp(str, "sub") == 0) return OP_SUB;
	if(strcmp(str, "mul") == 0) return OP_MUL;
	fprintf(stderr, "Invalid operation: %s\n", str);
	exit(EXIT_FAILURE);
	return 0;
}

static int op_to_llhdl(int op)
{
	switch(op) {
		case OP_NOT: return LLHDL_LOGIC_NOT;
		case OP_AND: return LLHDL_LOGIC_AND;
		case OP_OR: return LLHDL_LOGIC_OR;
		case OP_XOR: return LLHDL_LOGIC_XOR;
		case OP_ADD: return LLHDL_ARITH_ADD;
		case OP_SUB: return LLHDL_ARITH_SUB;
		case OP_MUL: return LLHDL_ARITH_MUL;
		default:
			assert(0);
			return 0;
	}
}

static int str_to_int(const char *str)
{
	char *c;
	int r;
	
	r = strtol(str, &c, 0);
	if(*c != 0) {
		fprintf(stderr, "Invalid integer: %s\n", str);
		exit(EXIT_FAILURE);
	}
	return r;
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
	if(strcmp(token, "s") == 0) {
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
static int parse_expr_i(struct llhdl_module *m, char **saveptr);
static int parse_expr_s(struct llhdl_module *m, char **saveptr);

static struct llhdl_node **parse_nexpr(struct llhdl_module *m, char **saveptr, int n)
{
	int i;
	struct llhdl_node **branches;
	
	branches = alloc_size(n*sizeof(struct llhdl_node *));
	for(i=0;i<n;i++)
		branches[i] = parse_expr(m, saveptr);
	
	return branches;
}

static struct llhdl_node *parse_operator(struct llhdl_module *m, char *op, char **saveptr)
{
	int opc;
	struct llhdl_node **branches;
	struct llhdl_node *n;
	int i, count;
	struct llhdl_node *select;
	int sign;
	struct llhdl_slice *slices;

	branches = NULL;
	opc = str_to_op(op);
	switch(opc) {
		case OP_NOT:
		case OP_AND:
		case OP_OR:
		case OP_XOR:
			branches = parse_nexpr(m, saveptr, llhdl_get_logic_arity(op_to_llhdl(opc)));
			n = llhdl_create_logic(op_to_llhdl(opc), branches);
			break;
		case OP_MUX:
			count = parse_expr_i(m, saveptr);
			select = parse_expr(m, saveptr);
			branches = parse_nexpr(m, saveptr, count);
			n = llhdl_create_mux(count, select, branches);
			break;
		case OP_FD:
			branches = parse_nexpr(m, saveptr, 2);
			n = llhdl_create_fd(branches[0], branches[1]);
			break;
		case OP_VECT:
			sign = parse_expr_s(m, saveptr);
			count = parse_expr_i(m, saveptr);
			assert(count > 0);
			slices = alloc_size(count*sizeof(struct llhdl_slice));
			for(i=0;i<count;i++) {
				slices[i].source = parse_expr(m, saveptr);
				slices[i].start = parse_expr_i(m, saveptr);
				slices[i].end = parse_expr_i(m, saveptr);
			}
			n = llhdl_create_vect(sign, count, slices);
			free(slices);
			break;
		case OP_ADD:
		case OP_SUB:
		case OP_MUL:
			branches = parse_nexpr(m, saveptr, 2);
			n = llhdl_create_arith(op_to_llhdl(opc), branches[0], branches[1]);
			break;
		default:
			assert(0);
			return NULL;
	}
	free(branches);
	return n;
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

static int parse_expr_i(struct llhdl_module *m, char **saveptr)
{
	char *token;

	token = strtok_r(NULL, delims, saveptr);
	if(token == NULL) {
		fprintf(stderr, "Unexpected end of expression\n");
		exit(EXIT_FAILURE);
	}
	return str_to_int(token);
}

static int parse_expr_s(struct llhdl_module *m, char **saveptr)
{
	char *token;

	token = strtok_r(NULL, delims, saveptr);
	if(token == NULL) {
		fprintf(stderr, "Unexpected end of expression\n");
		exit(EXIT_FAILURE);
	}
	if(strcmp(token, "s") == 0) return 1;
	if(strcmp(token, "u") == 0) return 0;
	fprintf(stderr, "Unexpected sign qualifier: %s\n", token);
	exit(EXIT_FAILURE);
	return 0;
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
			fprintf(fd, " s");
		fprintf(fd, "\n");
		n = n->p.signal.next;
	}
}

static void write_expr(FILE *fd, struct llhdl_node *n)
{
	int arity;
	int i;
	
	fprintf(fd, " ");
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			if(n->p.constant.sign || (n->p.constant.vectorsize != 1))
				fprintf(fd, "%d%c", n->p.constant.vectorsize, n->p.constant.sign ? 's' : 'u');
			mpz_out_str(fd, 10, n->p.constant.value);
			break;
		case LLHDL_NODE_SIGNAL:
			fprintf(fd, "%s", n->p.signal.name);
			break;
		case LLHDL_NODE_LOGIC:
			switch(n->p.logic.op) {
				case LLHDL_LOGIC_NOT:
					fprintf(fd, "#not");
					break;
				case LLHDL_LOGIC_AND:
					fprintf(fd, "#and");
					break;
				case LLHDL_LOGIC_OR:
					fprintf(fd, "#or");
					break;
				case LLHDL_LOGIC_XOR:
					fprintf(fd, "#xor");
					break;
				default:
					assert(0);
					break;
			}
			arity = llhdl_get_logic_arity(n->p.logic.op);
			for(i=0;i<arity;i++)
				write_expr(fd, n->p.logic.operands[i]);
			break;
		case LLHDL_NODE_MUX:
			fprintf(fd, "#mux %d", n->p.mux.nsources);
			write_expr(fd, n->p.mux.select);
			for(i=0;i<n->p.mux.nsources;i++)
				write_expr(fd, n->p.mux.sources[i]);
			break;
		case LLHDL_NODE_FD:
			fprintf(fd, "#fd");
			write_expr(fd, n->p.fd.clock);
			write_expr(fd, n->p.fd.data);
			break;
		case LLHDL_NODE_VECT:
			fprintf(fd, "#vect %c %d",
				n->p.vect.sign ? 's' : 'u',
				n->p.vect.nslices);
			for(i=0;i<n->p.vect.nslices;i++) {
				write_expr(fd, n->p.vect.slices[i].source);
				fprintf(fd, " %d %d", n->p.vect.slices[i].start, n->p.vect.slices[i].end);
			}
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
		default:
			assert(0);
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
