#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <util.h>

#include <banner/banner.h>

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
		return stralloc(line);

	c = line + 4;
	while(isblank(*c))
		c++;
	if(*c == '"')
		c++;
	c2 = c;
	while(!isblank(*c2) && (*c2 != '"'))
		c2++;
	if(*c2 == '"') {
		*c2 = 0;
		c2++;
	}
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
	r = asprintf(&replacement, "NET N%08x %s", s->uid, c2);
	assert(r != -1);

	return replacement;
}

static void help()
{
	banner("UCF symbol resolver");
	printf("Usage: llhdl-resolveucf [parameters] <input.ucf>\n");
	printf("The input UCF (input.ucf) is mandatory.\n");
	printf("Parameters are:\n");
	printf("  -h Display this help text and exit.\n");
	printf("  -s <symbols.sym> Specify the symbol file.\n");
	printf("  -o <output.ucf> Set the name of the output file.\n");
}

static char *file_suffix(char *inname, char *suffix)
{
	char *c;
	int r;
	char *out;
	
	inname = stralloc(inname);
	c = strrchr(inname, '.');
	if(c != NULL)
		*c = 0;
	r = asprintf(&out, "%s%s", inname, suffix);
	if(r == -1) abort();
	free(inname);
	return out;
}

int main(int argc, char *argv[])
{
	int opt;
	char *inname, *symname, *outname;
	FILE *fd_in, *fd_out;
	struct netlist_sym_store *sym;
	int r;
	char *line;
	size_t linesize;
	char *replacement;
	int err;

	symname = NULL;
	outname = NULL;
	while((opt = getopt(argc, argv, "ho:")) != -1) {
		switch(opt) {
			case 'h':
				help();
				exit(EXIT_SUCCESS);
				break;
			case 's':
				free(symname);
				symname = stralloc(optarg);
				break;
			case 'o':
				free(outname);
				outname = stralloc(optarg);
				break;
			default:
				fprintf(stderr, "Invalid option passed. Use -h for help.\n");
				exit(EXIT_FAILURE);
				break;
		}
	}
	
	if((argc - optind) != 1) {
		fprintf(stderr, "llhdl-resolveucf: missing input file. Use -h for help.\n");
		exit(EXIT_FAILURE);
	}
	inname = argv[optind];
	if(symname == NULL)
		symname = file_suffix(inname, ".sym");
	if(outname == NULL)
		outname = file_suffix(inname, "-resolved.ucf");

	fd_in = fopen(inname, "r");
	if(fd_in == NULL) {
		perror("Error opening input file");
		exit(EXIT_FAILURE);
	}
	sym = netlist_sym_newstore();
	netlist_sym_from_file(sym, symname);
	fd_out = fopen(outname, "w");
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
	
	free(symname);
	free(outname);

	return err;
}
