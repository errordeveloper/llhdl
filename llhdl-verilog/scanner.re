#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "parser.h"
#include "scanner.h"

#define BSIZE		8192

#define YYCTYPE		unsigned char
#define YYCURSOR	cursor
#define YYLIMIT		s->lim
#define YYMARKER	s->ptr
#define YYFILL(n)	{cursor = fill(s, cursor);}

#define RET(i)		{s->cur = cursor; return i;}

static unsigned char *fill(struct scanner *s, unsigned char *cursor)
{
	unsigned int cnt;
	unsigned char *buf;
	
	if(!s->eof) {
		cnt = s->tok - s->bot;
		if(cnt) {
			memcpy(s->bot, s->tok, s->lim - s->tok);
			s->tok = s->bot;
			s->ptr -= cnt;
			cursor -= cnt;
			s->pos -= cnt;
			s->lim -= cnt;
		}
		if((s->top - s->lim) < BSIZE) {
			buf = alloc_size((s->lim - s->bot) + BSIZE);
			memcpy(buf, s->tok, s->lim - s->tok);
			s->tok = buf;
			s->ptr = &buf[s->ptr - s->bot];
			cursor = &buf[cursor - s->bot];
			s->pos = &buf[s->pos - s->bot];
			s->lim = &buf[s->lim - s->bot];
			s->top = &s->lim[BSIZE];
			free(s->bot);
			s->bot = buf;
		}
		if((cnt = fread(s->lim, 1, BSIZE, s->fd)) != BSIZE) {
			s->eof = &s->lim[cnt];
			*(s->eof)++ = '\n';
		}
		s->lim += cnt;
	}
	return cursor;
}

struct scanner *scanner_new(FILE *fd)
{
	struct scanner *s;
	
	s = alloc_type(struct scanner);
	
	s->fd = fd;
	s->bot = NULL;
	s->tok = NULL;
	s->ptr = NULL;
	s->cur = NULL;
	s->pos = NULL;
	s->lim = NULL;
	s->top = NULL;
	s->eof = NULL;
	s->line = 1;
	
	return s;
}

void scanner_free(struct scanner *s)
{
	free(s->bot);
	free(s);
}

int scanner_scan(struct scanner *s)
{
	unsigned char *cursor;

	cursor = s->cur;
std:
	s->tok = cursor;
/*!re2c
	any	= [\000-\377];
	B	= [01];
	O	= [0-7];
	D	= [0-9];
	H	= [a-fA-F0-9];
	L	= [a-zA-Z_];
*/

/*!re2c
	"/*"			{ goto comment; }
	"//"			{ goto slcomment; }

	"module"		{ RET(TOK_MODULE); }
	"endmodule"		{ RET(TOK_ENDMODULE); }
	"wire"			{ RET(TOK_REGWIRE); }
	"reg"			{ RET(TOK_REGWIRE); }
	"signed"		{ RET(TOK_SIGNED); }
	"input"			{ RET(TOK_INPUT); }
	"output"		{ RET(TOK_OUTPUT); }
	"assign"		{ RET(TOK_ASSIGN); }
	"if"			{ RET(TOK_IF); }
	"else"			{ RET(TOK_ELSE); }
	"begin"			{ RET(TOK_BEGIN); }
	"end"			{ RET(TOK_END); }
	"always"		{ RET(TOK_ALWAYS); }
	"@"			{ RET(TOK_AT); }
	"posedge"		{ RET(TOK_POSEDGE); }
	"*"			{ RET(TOK_STAR); }

	L (L|D)*		{ RET(TOK_ID); }

	(D* "'b" B+) |
	(D* "'o" O+) |
	(D* "'d" D+) |
	(D* "'h" H+)		{ RET(TOK_VCON); }
	
	D+			{ RET(TOK_PINT); }

	"="			{ RET(TOK_BASSIGN); }
	"<="			{ RET(TOK_NBASSIGN); }

	"=="			{ RET(TOK_EQL); }
	"!="			{ RET(TOK_NEQ); }
	";"			{ RET(TOK_SEMICOLON); }
	","			{ RET(TOK_COMMA); }
	":"			{ RET(TOK_COLON); }
	"("			{ RET(TOK_LPAREN); }
	")"			{ RET(TOK_RPAREN); }
	"["			{ RET(TOK_LBRACKET); }
	"]"			{ RET(TOK_RBRACKET); }
	"&"			{ RET(TOK_AND); }
	"~"			{ RET(TOK_TILDE); }
	"^"			{ RET(TOK_XOR); }
	"|"			{ RET(TOK_OR); }
	"?"			{ RET(TOK_QUESTION); }

	[ \t\v\f]+		{ goto std; }

	"\n" {
		if(cursor == s->eof)
			RET(TOK_EOF);
		s->pos = cursor;
		s->line++;
		goto std;
	}

	any {
		printf("unexpected character: %c (line %d)\n", *s->tok, s->line);
		exit(EXIT_FAILURE);
	}
*/

slcomment:
/*!re2c
	"\n" {
		if(cursor == s->eof)
			RET(TOK_EOF);
		s->tok = s->pos = cursor;
		s->line++;
		goto std;
	}
        any 			{ goto slcomment; }
*/

comment:
/*!re2c
	"*/"			{ goto std; }
	"\n" {
		if(cursor == s->eof)
			RET(TOK_EOF);
		s->tok = s->pos = cursor;
		s->line++;
		goto comment;
	}
        any 			{ goto comment; }
*/
}

char *scanner_get_token(struct scanner *s)
{
	int len;
	char *ret;

	len = s->cur - s->tok;
	ret = alloc_size(len+1);
	memcpy(ret, s->tok, len);
	ret[len] = 0;
	return ret;
}
