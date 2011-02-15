#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <gmp.h>

#include "parser.h"
#include "scanner.h"
#include "verilog.h"

struct verilog_constant *verilog_new_constant(int vectorsize, int sign, mpz_t value)
{
	struct verilog_constant *c;

	c = alloc_type(struct verilog_constant);
	c->vectorsize = vectorsize;
	c->sign = sign;
	mpz_init_set(c->value, value);
	return c;
}

struct verilog_constant *verilog_new_constant_str(char *str)
{
	char *q;
	struct verilog_constant *c;
	int base;

	base = 0;
	c = alloc_type(struct verilog_constant);
	q = strchr(str, '\'');
	if(q == NULL) {
		c->vectorsize = 32;
		c->sign = 0;
		base = 10;
	} else {
		*q = 0;
		q++;
		c->vectorsize = atoi(str);
		c->sign = 0; // TODO
		switch(*q) {
			case 'b':
				base = 2;
				break;
			case 'o':
				base = 8;
				break;
			case 'd':
				base = 10;
				break;
			case 'h':
				base = 16;
				break;
			default:
				assert(0);
				break;
		}
		str = q+1;
	}
	mpz_init_set_str(c->value, str, base);
	return c;
}

void verilog_free_constant(struct verilog_constant *c)
{
	mpz_clear(c->value);
	free(c);
}

struct verilog_signal *verilog_find_signal(struct verilog_module *m, const char *signal)
{
	struct verilog_signal *s;

	s = m->shead;
	while(s != NULL) {
		if(strcmp(s->name, signal) == 0)
			return s;
		s = s->next;
	}
	return NULL;
}

struct verilog_signal *verilog_new_update_signal(struct verilog_module *m, int type, const char *name, int vectorsize, int sign)
{
	struct verilog_signal *s;
	int len;

	s = verilog_find_signal(m, name);
	if(s != NULL) {
		s->type = type;
		if(s->vectorsize != vectorsize) return NULL;
		if(s->sign != sign) return NULL;
		return s;
	}
	len = strlen(name)+1;
	s = alloc_size(sizeof(struct verilog_signal)+len);
	s->type = type;
	s->vectorsize = vectorsize;
	s->sign = sign;
	s->next = m->shead;
	s->llhdl_signal = NULL;
	memcpy(s->name, name, len);
	m->shead = s;
	return s;
}

void verilog_free_signal(struct verilog_module *m, struct verilog_signal *s)
{
	if(s == m->shead) {
		m->shead = m->shead->next;
	} else {
		struct verilog_signal *before_s;
		before_s = m->shead;
		while(before_s->next != s) {
			before_s = before_s->next;
			assert(before_s != NULL);
		}
		before_s->next = s->next;
	}
	free(s);
}

void verilog_free_signal_list(struct verilog_signal *head)
{
	struct verilog_signal *s1, *s2;
	
	s1 = head;
	while(s1 != NULL) {
		s2 = s1->next;
		free(s1);
		s1 = s2;
	}
}

int verilog_get_node_arity(int type)
{
	switch(type) {
		case VERILOG_NODE_CONSTANT: return 1;
		case VERILOG_NODE_SIGNAL: return 1;
		case VERILOG_NODE_EQL: return 2;
		case VERILOG_NODE_NEQ: return 2;
		case VERILOG_NODE_OR: return 2;
		case VERILOG_NODE_AND: return 2;
		case VERILOG_NODE_TILDE: return 1;
		case VERILOG_NODE_XOR: return 2;
		case VERILOG_NODE_ALT: return 3;
	}
	assert(0);
}

struct verilog_node *verilog_new_constant_node(struct verilog_constant *constant)
{
	struct verilog_node *n;

	n = alloc_size(sizeof(int)+sizeof(void *));
	n->type = VERILOG_NODE_CONSTANT;
	n->branches[0] = constant;

	return n;
}

struct verilog_node *verilog_new_signal_node(struct verilog_signal *signal)
{
	struct verilog_node *n;

	n = alloc_size(sizeof(int)+sizeof(void *));
	n->type = VERILOG_NODE_SIGNAL;
	n->branches[0] = signal;

	return n;
}

struct verilog_node *verilog_new_op_node(int type)
{
	struct verilog_node *n;
	int arity;
	int i;

	arity = verilog_get_node_arity(type);
	n = alloc_size(sizeof(int)+arity*sizeof(void *));
	n->type = type;
	for(i=0;i<arity;i++)
		n->branches[i] = NULL;
	return n;
}

void verilog_free_node(struct verilog_node *n)
{
	if((n->type != VERILOG_NODE_CONSTANT) && (n->type != VERILOG_NODE_SIGNAL)) {
		int arity;
		int i;

		arity = verilog_get_node_arity(n->type);
		for(i=0;i<arity;i++)
			verilog_free_node(n->branches[i]);
	}
	if(n->type == VERILOG_NODE_CONSTANT)
		verilog_free_constant(n->branches[0]);
	free(n);
}

static struct verilog_statement *alloc_base_statement(int extra, int type)
{
	struct verilog_statement *s;
	
	s = alloc_size(sizeof(int)+sizeof(void *)+extra);
	s->type = type;
	s->next = NULL;
	return s;
}

struct verilog_statement *verilog_new_assignment(struct verilog_signal *target, int blocking, struct verilog_node *source)
{
	struct verilog_statement *s;

	s = alloc_base_statement(sizeof(struct verilog_assignment), VERILOG_STATEMENT_ASSIGNMENT);
	s->p.assignment.target = target;
	s->p.assignment.blocking = blocking;
	s->p.assignment.source = source;
	return s;
}

struct verilog_statement *verilog_new_condition(struct verilog_node *condition, struct verilog_statement *negative, struct verilog_statement *positive)
{
	struct verilog_statement *s;

	s = alloc_base_statement(sizeof(struct verilog_condition), VERILOG_STATEMENT_CONDITION);
	s->p.condition.condition = condition;
	s->p.condition.negative = negative;
	s->p.condition.positive = positive;
	return s;
}

void verilog_free_statement(struct verilog_statement *s)
{
	switch(s->type) {
		case VERILOG_STATEMENT_ASSIGNMENT:
			verilog_free_node(s->p.assignment.source);
			break;
		case VERILOG_STATEMENT_CONDITION:
			verilog_free_node(s->p.condition.condition);
			verilog_free_statement_list(s->p.condition.negative);
			verilog_free_statement_list(s->p.condition.positive);
			break;
		default:
			assert(0);
			break;
	}
	free(s);
}

void verilog_free_statement_list(struct verilog_statement *head)
{
	struct verilog_statement *s1, *s2;
	
	s1 = head;
	while(s1 != NULL) {
		s2 = s1->next;
		verilog_free_statement(s1);
		s1 = s2;
	}
}

struct verilog_process *verilog_new_process(struct verilog_module *m, struct verilog_signal *clock, struct verilog_statement *head)
{
	struct verilog_process *p;

	p = alloc_type(struct verilog_process);
	p->clock = clock;
	p->head = head;
	p->next = m->phead;
	m->phead = p;
	return p;
}

void verilog_free_process(struct verilog_module *m, struct verilog_process *p)
{
	if(p == m->phead) {
		m->phead = m->phead->next;
	} else {
		struct verilog_process *before_p;
		before_p = m->phead;
		while(before_p->next != p) {
			before_p = before_p->next;
			assert(before_p != NULL);
		}
		before_p->next = p->next;
	}
	verilog_free_statement_list(p->head);
	free(p);
}

void verilog_free_process_list(struct verilog_process *head)
{
	struct verilog_process *p1, *p2;
	
	p1 = head;
	while(p1 != NULL) {
		p2 = p1->next;
		verilog_free_statement_list(p1->head);
		free(p1);
		p1 = p2;
	}
}

struct verilog_module *verilog_new_module()
{
	struct verilog_module *m;

	m = alloc_type(struct verilog_module);
	m->name = NULL;
	m->shead = NULL;
	m->phead = NULL;
	return m;
}

void verilog_set_module_name(struct verilog_module *m, const char *name)
{
	free(m->name);
	m->name = stralloc(name);
}

void verilog_free_module(struct verilog_module *m)
{
	verilog_free_signal_list(m->shead);
	verilog_free_process_list(m->phead);
	free(m->name);
	free(m);
}

extern void *ParseAlloc(void *(*mallocProc)(size_t));
extern void ParseFree(void *p, void (*freeProc)(void *));
extern void Parse(void *yyp, int yymajor, void *yyminor, struct verilog_module *outm);
extern void ParseTrace(FILE *TraceFILE, char *zTracePrompt);

struct verilog_module *verilog_parse_fd(FILE *fd)
{
	struct scanner *s;
	int tok;
	void *p;
	char *stoken;
	struct verilog_module *m;

	//ParseTrace(stdout, "parser: ");
	s = scanner_new(fd);
	m = verilog_new_module();
	p = ParseAlloc(malloc);
	assert(p != NULL);
	tok = scanner_scan(s);
	while(tok != TOK_EOF) {
		stoken = scanner_get_token(s);
		Parse(p, tok, stoken, m);
		tok = scanner_scan(s);
	}
	Parse(p, TOK_EOF, NULL, m);
	ParseFree(p, free);
	scanner_free(s);

	return m;
}

struct verilog_module *verilog_parse_file(const char *filename)
{
	FILE *fd;
	struct verilog_module *m;

	fd = fopen(filename, "r");
	if(fd == NULL) {
		perror("verilog_parse_file");
		exit(EXIT_FAILURE);
	}
	m = verilog_parse_fd(fd);
	fclose(fd);
	return m;
}

static void indent(int n)
{
	int i;
	
	for(i=0;i<n;i++)
		printf("  ");
}

void verilog_dump_signal_list(int level, struct verilog_signal *head)
{
	level++;
	while(head != NULL) {
		indent(level);
		switch(head->type) {
			case VERILOG_SIGNAL_REGWIRE:
				printf("signal ");
				break;
			case VERILOG_SIGNAL_OUTPUT:
				printf("output ");
				break;
			case VERILOG_SIGNAL_INPUT:
				printf("input ");
				break;
		}
		if(head->sign)
			printf("signed ");
		if(head->vectorsize > 1)
			printf("<%d> ", head->vectorsize);
		printf("%s\n", head->name);
		head = head->next;
	}
}

void verilog_dump_node(struct verilog_node *n)
{
	switch(n->type) {
		case VERILOG_NODE_CONSTANT: {
			struct verilog_constant *c;
			c = n->branches[0];
			printf("(");
			mpz_out_str(stdout, 10, c->value);
			printf("<%d%s>)", c->vectorsize, c->sign ? "s":"");
			break;
		}
		case VERILOG_NODE_SIGNAL: {
			struct verilog_signal *s;
			s = n->branches[0];
			printf("(%s)", s->name);
			break;
		}
		case VERILOG_NODE_EQL:
			printf("(");
			verilog_dump_node(n->branches[0]);
			printf("==");
			verilog_dump_node(n->branches[1]);
			printf(")");
			break;
		case VERILOG_NODE_NEQ:
			printf("(");
			verilog_dump_node(n->branches[0]);
			printf("!=");
			verilog_dump_node(n->branches[1]);
			printf(")");
			break;
		case VERILOG_NODE_OR:
			printf("(");
			verilog_dump_node(n->branches[0]);
			printf("|");
			verilog_dump_node(n->branches[1]);
			printf(")");
			break;
		case VERILOG_NODE_AND:
			printf("(");
			verilog_dump_node(n->branches[0]);
			printf("&");
			verilog_dump_node(n->branches[1]);
			printf(")");
			break;
		case VERILOG_NODE_TILDE:
			printf("(~");
			verilog_dump_node(n->branches[0]);
			printf(")");
			break;
		case VERILOG_NODE_XOR:
			printf("(");
			verilog_dump_node(n->branches[0]);
			printf("&");
			verilog_dump_node(n->branches[1]);
			printf(")");
			break;
		case VERILOG_NODE_ALT:
			printf("(");
			verilog_dump_node(n->branches[0]);
			printf("?");
			verilog_dump_node(n->branches[1]);
			printf(":");
			verilog_dump_node(n->branches[2]);
			printf(")");
			break;
	}
}

void verilog_dump_statement_list(int level, struct verilog_statement *head)
{
	level++;
	while(head != NULL) {
		switch(head->type) {
			case VERILOG_STATEMENT_ASSIGNMENT:
				indent(level);
				printf("%s ", head->p.assignment.target->name);
				if(head->p.assignment.blocking)
					printf("= ");
				else
					printf("<= ");
				verilog_dump_node(head->p.assignment.source);
				printf("\n");
				break;
			case VERILOG_STATEMENT_CONDITION:
				indent(level);
				printf("if");
				verilog_dump_node(head->p.condition.condition);
				printf(":\n");
				verilog_dump_statement_list(level, head->p.condition.positive);
				if(head->p.condition.negative != NULL) {
					indent(level);
					printf("else:\n");
					verilog_dump_statement_list(level, head->p.condition.negative);
				}
				break;
			default:
				assert(0);
				break;
		}
		head = head->next;
	}
}

void verilog_dump_process_list(int level, struct verilog_process *head)
{
	level++;
	while(head != NULL) {
		indent(level);
		if(head->clock == NULL)
			printf("process (comb):\n");
		else
			printf("process (clock=%s):\n", head->clock->name);
		verilog_dump_statement_list(level, head->head);
		head = head->next;
	}
}

void verilog_dump_module(int level, struct verilog_module *m)
{
	level++;
	indent(level);
	printf("Module %s:\n", m->name);
	verilog_dump_signal_list(level, m->shead);
	verilog_dump_process_list(level, m->phead);
}

static void verilog_update_bl(int *previous, int new)
{
	if(new == VERILOG_BL_EMPTY)
		return;
	if(*previous == VERILOG_BL_EMPTY) {
		*previous = new;
		return;
	}
	if(*previous != new)
		*previous = VERILOG_BL_MIXED;
}

int verilog_statements_blocking(struct verilog_statement *s)
{
	int r;
	
	r = VERILOG_BL_EMPTY;
	while(s != NULL) {
		switch(s->type) {
			case VERILOG_STATEMENT_ASSIGNMENT:
				if(s->p.assignment.blocking)
					verilog_update_bl(&r, VERILOG_BL_BLOCKING);
				else
					verilog_update_bl(&r, VERILOG_BL_NONBLOCKING);
				break;
			case VERILOG_STATEMENT_CONDITION:
				verilog_update_bl(&r, verilog_statements_blocking(s->p.condition.negative));
				verilog_update_bl(&r, verilog_statements_blocking(s->p.condition.positive));
				break;
			default:
				assert(0);
				break;
		}
		s = s->next;
	}
	return r;
}

int verilog_process_blocking(struct verilog_process *p)
{
	int r;
	
	r = verilog_statements_blocking(p->head);

	/* Non-clocked processes must use blocking assignments */
	if((p->clock == NULL) && (r == VERILOG_BL_NONBLOCKING))
		r = VERILOG_BL_MIXED;

	return r;
}
