%include {
	#include <assert.h>
	#include <string.h>
	#include <stdlib.h>
	#include "verilog.h"
}

%start_symbol module
%extra_argument {struct verilog_module *outm}

%token_type {void *}
%token_destructor { free($$); }

%type constant {struct verilog_constant *}
%destructor constant { verilog_free_constant($$); }

constant(C) ::= TOK_VCON(V). {
	C = verilog_new_constant_str(V);
}

%type newsignal {struct verilog_signal *}
%destructor newsignal { verilog_free_signal(outm, $$); }

newsignal(S) ::= TOK_REGWIRE TOK_ID(I). {
	S = verilog_new_update_signal(outm, VERILOG_SIGNAL_REGWIRE, I, 1, 0);
}

newsignal(S) ::= TOK_OUTPUT TOK_ID(I). {
	S = verilog_new_update_signal(outm, VERILOG_SIGNAL_OUTPUT, I, 1, 0);
}

newsignal(S) ::= TOK_INPUT TOK_ID(I). {
	S = verilog_new_update_signal(outm, VERILOG_SIGNAL_INPUT, I, 1, 0);
}

newsignal(S) ::= TOK_OUTPUT TOK_REGWIRE TOK_ID(I). {
	S = verilog_new_update_signal(outm, VERILOG_SIGNAL_OUTPUT, I, 1, 0);
}

newsignal(S) ::= TOK_INPUT TOK_REGWIRE TOK_ID(I). {
	S = verilog_new_update_signal(outm, VERILOG_SIGNAL_INPUT, I, 1, 0);
}

%type signal {struct verilog_signal *}

signal(S) ::= TOK_ID(I). {
	S = verilog_find_signal(outm, I);
	if(S == NULL) {
		fprintf(stderr, "Signal not found: %s\n", I);
		exit(EXIT_FAILURE);
	}
}

%type node {struct verilog_node *}
%destructor node { verilog_free_node($$); }

node(N) ::= constant(C). {
	N = verilog_new_constant_node(C);
}

node(N) ::= signal(S). {
	N = verilog_new_signal_node(S);
}

%left TOK_QUESTION TOK_COLON.
%left TOK_TILDE.
%left TOK_OR TOK_AND TOK_XOR.
%left TOK_EQL TOK_NEQ.

node(N) ::= node(A) TOK_EQL node(B). {
	N = verilog_new_op_node(VERILOG_NODE_EQL);
	N->branches[0] = A;
	N->branches[1] = B;
}

node(N) ::= node(A) TOK_NEQ node(B). {
	N = verilog_new_op_node(VERILOG_NODE_NEQ);
	N->branches[0] = A;
	N->branches[1] = B;
}

node(N) ::= node(A) TOK_OR node(B). {
	N = verilog_new_op_node(VERILOG_NODE_OR);
	N->branches[0] = A;
	N->branches[1] = B;
}

node(N) ::= node(A) TOK_AND node(B). {
	N = verilog_new_op_node(VERILOG_NODE_AND);
	N->branches[0] = A;
	N->branches[1] = B;
}

node(N) ::= TOK_TILDE node(A). {
	N = verilog_new_op_node(VERILOG_NODE_TILDE);
	N->branches[0] = A;
}

node(N) ::= node(A) TOK_XOR node(B). {
	N = verilog_new_op_node(VERILOG_NODE_XOR);
	N->branches[0] = A;
	N->branches[1] = B;
}

node(N) ::= node(A) TOK_QUESTION node(B) TOK_COLON node(C). {
	N = verilog_new_op_node(VERILOG_NODE_ALT);
	N->branches[0] = A;
	N->branches[1] = B;
	N->branches[2] = C;
}

node(N) ::= TOK_LPAREN node(A) TOK_RPAREN. {
	N = A;
}

%type singleassignment {struct verilog_statement *}
%destructor singleassignment { verilog_free_statement($$); }

singleassignment(A) ::= signal(D) TOK_BASSIGN node(S). {
	A = verilog_new_assignment(D, 1, S);
}

singleassignment(A) ::= signal(D) TOK_NBASSIGN node(S). {
	A = verilog_new_assignment(D, 0, S);
}

%type statement {struct verilog_statement *}
%destructor statement { verilog_free_statement_list($$); }

%type singlecondition {struct verilog_statement *}
%destructor singlecondition { verilog_free_statement($$); }

singlecondition(C) ::= TOK_IF TOK_LPAREN node(E) TOK_RPAREN statement(S). {
	C = verilog_new_condition(E, NULL, S);
}

/*singlecondition(C) ::= TOK_IF TOK_LPAREN node(E) TOK_RPAREN statement(P) TOK_ELSE statement(N). {
	C = verilog_new_condition(E, N, P);
}*/

%type singlestatement {struct verilog_statement *}
%destructor singlestatement { verilog_free_statement($$); }

singlestatement(S) ::= singleassignment(A) TOK_SEMICOLON. { S = A; }
singlestatement(S) ::= singlecondition(C). { S = C; }

%type statementlist {struct verilog_statement *}
%destructor statementlist { verilog_free_statement_list($$); }

statementlist(L) ::= statementlist(B) singlestatement(S). {
	if(B != NULL) {
		B->next = S;
		L = B;
	} else
		L = S;
}
statementlist(L) ::= TOK_SEMICOLON. { L = NULL; }
statementlist(L) ::= . { L = NULL; }

statement(S) ::= singlestatement(X). { S = X; }
statement(S) ::= TOK_BEGIN statementlist(L) TOK_END. { S = L; }

%type process {struct verilog_process *}
%destructor process { verilog_free_process(outm, $$); }

process(P) ::= TOK_ASSIGN singleassignment(A) TOK_SEMICOLON. {
	P = verilog_new_process(outm, NULL, A);
}

process(P) ::= TOK_ALWAYS TOK_AT TOK_LPAREN TOK_STAR TOK_RPAREN statement(S). {
	P = verilog_new_process(outm, NULL, S);
}

process(P) ::= TOK_ALWAYS TOK_AT TOK_LPAREN TOK_POSEDGE signal(C) TOK_RPAREN statement(S). {
	P = verilog_new_process(outm, C, S);
}

/* FIXME: this accepts wire/reg declarations in the module header */

/* FIXME: add destructors for io and body */

io(I) ::= io TOK_COMMA newsignal(S). { I = S; }
io(I) ::= newsignal(S). { I = S; }
io ::= .

body(B) ::= body newsignal(N). { B = N; }
body(B) ::= body process(P). { B = P; }
body ::= .

module ::= TOK_MODULE TOK_ID(N) TOK_LPAREN io TOK_RPAREN TOK_SEMICOLON body TOK_ENDMODULE. {
	verilog_set_module_name(outm, N);
}
