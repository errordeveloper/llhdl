#ifndef __SCANNER_H
#define __SCANNER_H

/* Must be 0 to tell the parser to start */
#define TOK_EOF 0

struct scanner {
	FILE *fd;
	unsigned char *bot, *tok, *ptr, *cur, *pos, *lim, *top, *eof;
	unsigned int line;
};

struct scanner *scanner_new(FILE *fd);
void scanner_free(struct scanner *s);

/* get to the next token and return its type */
int scanner_scan(struct scanner *s);

/* get a newly allocated string with the current token */
char *scanner_get_token(struct scanner *s);

#endif /* __SCANNER_H */

