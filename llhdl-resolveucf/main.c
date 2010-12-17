#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <netlist/symbol.h>

static char *process_line(struct netlist_sym_store *sym, char *line)
{
	char *c;
	char *c2;
	struct netlist_sym *s;
	int r;
	char *replacement;
	
	while(isblank(*line))
		line++;
	if(strncasecmp(line, "NET ", 4) != 0)
		return strdup(line);

	c = line + 4;
	while(isblank(*c))
		c++;
	if(*c == '"')
		c++;
	c2 = c;
	while(!isblank(*c2) && (*c2 != '"'))
		c2++;
	if(*c2 == '"')
		c2++;
	if(*(c2+1) == 0) {
		fprintf(stderr, "Malformed line: %s\n", line);
		return NULL;
	}
	*c2 = 0;
	c2++;

	s = netlist_sym_lookup(sym, c, 'N');
	if(s == NULL) {
		fprintf(stderr, "Symbol not found: %s\n", c);
		return NULL;
	}
	r = asprintf(&replacement, "NET N%08x%s ", s->uid, c2);
	assert(r != -1);

	return replacement;
}

int main(int argc, char *argv[])
{
	FILE *fd_in, *fd_out;
	struct netlist_sym_store *sym;
	int r;
	char *line;
	size_t linesize;
	char *replacement;
	int err;

	if(argc != 4) {
		fprintf(stderr, "Usage: llhdl-resolveucf <input.ucf> <symbols.sym> <output.ucf>\n");
		return 1;
	}

	fd_in = fopen(argv[1], "r");
	if(fd_in == NULL) {
		perror("Error opening input file");
		exit(EXIT_FAILURE);
	}
	sym = netlist_sym_newstore();
	netlist_sym_from_file(sym, argv[2]);
	fd_out = fopen(argv[3], "w");
	if(fd_out == NULL) {
		perror("Error opening output file");
		exit(EXIT_FAILURE);
	}

	line = NULL;
	linesize = 0;
	err = 0;
	while(1) {
		r = getline(&line, &linesize, fd_in);
		if(r == -1) {
			assert(feof(fd_in));
			break;
		}
		replacement = process_line(sym, line);
		if(replacement != NULL) {
			r = fwrite(replacement, strlen(replacement), 1, fd_out);
			assert(r == 1);
			free(replacement);
		} else {
			err = 2;
			break;
		}
	}

	free(line);
	netlist_sym_freestore(sym);
	fclose(fd_in);
	r = fclose(fd_out);
	assert(r == 0);

	return err;
}
