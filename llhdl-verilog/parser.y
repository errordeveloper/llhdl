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
%destructor constant { free($$); }

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
}

%type node {struct verilog_node *}
%destructor node { free($$); }

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

%type assignment {struct verilog_assignment *}
%destructor assignment { free($$); }

assignment(A) ::= signal(D) TOK_BASSIGN node(S). {
	A = verilog_new_assignment(D, 1, S);
}

%type process {struct verilog_process *}
%destructor process { verilog_free_process(outm, $$); }

process(P) ::= TOK_ASSIGN assignment(A) TOK_SEMICOLON. {
	P = verilog_new_process_assign(outm, A);
}

/* FIXME: this accepts wire/reg declarations in the module header */

io ::= io TOK_COMMA newsignal.
io ::= .

body ::= body newsignal.
body ::= body process.
body ::= .

module ::= TOK_MODULE TOK_LPAREN io TOK_RPAREN TOK_SEMICOLON body TOK_ENDMODULE TOK_EOF.
