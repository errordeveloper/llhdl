#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "scanner.h"
#include "verilog.h"

struct verilog_constant *verilog_new_constant(int vectorsize, int sign, long long int value)
{
	struct verilog_constant *c;

	c = malloc(sizeof(struct verilog_constant));
	assert(c != NULL);
	c->vectorsize = vectorsize;
	c->sign = sign;
	c->value = value;
	return c;
}

struct verilog_constant *verilog_new_constant_str(char *str)
{
	char *q;
	struct verilog_constant *c;
	int base;

	base = 0;
	c = malloc(sizeof(struct verilog_constant));
	assert(c != NULL);
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
			case 'x':
				base = 16;
				break;
			default:
				assert(0);
				break;
		}
		str = q+1;
	}
	c->value = strtoll(str, NULL, base);
	return c;
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
	s = malloc(sizeof(struct verilog_signal)+len);
	assert(s != NULL);
	s->type = type;
	s->vectorsize = vectorsize;
	s->sign = sign;
	s->next = m->shead;
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

int verilog_get_node_arity(int type)
{
	switch(type) {
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

	n = malloc(sizeof(int)+sizeof(void *));
	assert(n != NULL);
	n->type = VERILOG_NODE_CONSTANT;
	n->branches[0] = constant;

	return n;
}

struct verilog_node *verilog_new_signal_node(struct verilog_signal *signal)
{
	struct verilog_node *n;

	n = malloc(sizeof(int)+sizeof(void *));
	assert(n != NULL);
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
	n = malloc(sizeof(int)+arity*sizeof(void *));
	assert(n != NULL);
	n->type = type;
	for(i=0;i<arity;i++)
		n->branches[i] = NULL;
	return n;
}

struct verilog_assignment *verilog_new_assignment(struct verilog_signal *target, int blocking, struct verilog_node *source)
{
	struct verilog_assignment *a;

	a = malloc(sizeof(struct verilog_assignment));
	assert(a != NULL);
	a->target = target;
	a->blocking = blocking;
	a->source = source;
	return a;
}

struct verilog_process *verilog_new_process_assign(struct verilog_module *m, struct verilog_assignment *a)
{
	struct verilog_process *p;

	p = malloc(sizeof(struct verilog_process));
	assert(p != NULL);
	p->clock = NULL;
	p->head = p->tail = a;
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
	free(p);
}

struct verilog_module *verilog_new_module()
{
	struct verilog_module *m;

	m = malloc(sizeof(struct verilog_module));
	assert(m != NULL);
	m->name = NULL;
	m->shead = NULL;
	m->phead = NULL;
	return m;
}

void verilog_set_module_name(struct verilog_module *m, const char *name)
{
	free(m->name);
	m->name = strdup(name);
}

static void free_node(struct verilog_node *n)
{
	if((n->type != VERILOG_NODE_CONSTANT) && (n->type != VERILOG_NODE_SIGNAL)) {
		int arity;
		int i;

		arity = verilog_get_node_arity(n->type);
		for(i=0;i<arity;i++)
			free_node(n->branches[i]);
	}
	if(n->type == VERILOG_NODE_CONSTANT)
		free(n->branches[0]);
	free(n);
}

void verilog_free_module(struct verilog_module *m)
{
	struct verilog_signal *s1, *s2;
	struct verilog_process *p1, *p2;
	struct verilog_assignment *a1, *a2;

	s1 = m->shead;
	while(s1 != NULL) {
		s2 = s1->next;
		free(s1);
		s1 = s2;
	}

	p1 = m->phead;
	while(p1 != NULL) {
		a1 = p1->head;
		while(a1 != NULL) {
			free_node(a1->source);
			a2 = a1->next;
			free(a1);
			a1 = a2;
		}
		p2 = p1->next;
		free(p1);
		p1 = p2;
	}

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

void verilog_dump_signal(struct verilog_signal *s)
{
	switch(s->type) {
		case VERILOG_SIGNAL_REGWIRE:
			printf("  signal ");
			break;
		case VERILOG_SIGNAL_OUTPUT:
			printf("  output ");
			break;
		case VERILOG_SIGNAL_INPUT:
			printf("  input ");
			break;
	}
	if(s->sign)
		printf("signed ");
	if(s->vectorsize > 1)
		printf("<%d> ", s->vectorsize);
	printf("%s\n", s->name);
}

void verilog_dump_node(struct verilog_node *n)
{
	switch(n->type) {
		case VERILOG_NODE_CONSTANT: {
			struct verilog_constant *c;
			c = n->branches[0];
			printf("(%lld<%d%s>)", c->value, c->vectorsize, c->sign ? "s":"");
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

void verilog_dump_assignment(struct verilog_assignment *a)
{
	printf("    %s ", a->target->name);
	if(a->blocking)
		printf("= ");
	else
		printf("<= ");
	verilog_dump_node(a->source);
	printf("\n");
}

void verilog_dump_process(struct verilog_process *p)
{
	struct verilog_assignment *a;

	if(p->clock == NULL)
		printf("  process (comb):\n");
	else
		printf("  process (clock=%s):\n", p->clock->name);

	a = p->head;
	while(a != NULL) {
		verilog_dump_assignment(a);
		a = a->next;
	}
}

void verilog_dump_module(struct verilog_module *m)
{
	struct verilog_signal *s;
	struct verilog_process *p;
	
	printf("Module %s:\n", m->name);
	s = m->shead;
	while(s != NULL) {
		verilog_dump_signal(s);
		s = s->next;
	}
	p = m->phead;
	while(p != NULL) {
		verilog_dump_process(p);
		p = p->next;
	}
}
