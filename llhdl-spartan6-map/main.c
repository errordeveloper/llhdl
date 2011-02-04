#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <banner/banner.h>
#include <tilm/tilm.h>

static const char *parts[] = {
	"xc6slx45-fgg484-2",
	"xc6slx45-fgg484-3",
	NULL
};

const char default_part[] = "xc6slx45-fgg484-2";

static const char *validate_part(const char *part)
{
	int i;
	
	i = 0;
	while(parts[i] != NULL) {
		if(strcmp(parts[i], part) == 0)
			return parts[i];
		i++;
	}
	return NULL;
}

static void list_lutmappers()
{
	int i;
	
	for(i=0;i<TILM_COUNT;i++)
		printf("      %s: %s mapper%s\n", tilm_mappers[i].handle, tilm_mappers[i].description, i == TILM_DEFAULT ? " (default)" : "");
}

static void list_parts()
{
	int i;
	
	i = 0;
	while(parts[i] != NULL) {
		printf("      %s%s\n", parts[i], strcmp(parts[i], default_part) == 0 ? " (default)" : "");
		i++;
	}
}

static void help()
{
	banner("\nXilinx Spartan-6 mapper");
	printf("Usage: llhdl-spartan6-map [options] <input.lhd> <output.edf> <output.sym>\n\n");
	printf("Files are:\n");
	printf("  input.lhd: Input design in LLHDL exchange format.\n");
	printf("  output.edf: Output netlist in EDIF format.\n");
	printf("  output.sym: Output symbols to resolve signal names.\n");
	printf("All file names are mandatory.\n\n");
	printf("Options are:\n");
	printf("  -h Display this help text.\n");
	printf("  -p <part>: Select part. Supported values are:\n");
	list_parts();
	printf("  -l <algo>: Select LUT mapping algorithm. Supported values are:\n");
	list_lutmappers();
}

static const char *part = default_part;
static int lutmapper = TILM_DEFAULT;
static const char *input_lhd;
static const char *output_edf;
static const char *output_sym;

int main(int argc, char *argv[])
{
	int opt;
	
	assert(validate_part(default_part));
	while((opt = getopt(argc, argv, "hp:l:")) != -1) {
		switch(opt) {
			case 'h':
				help();
				exit(EXIT_SUCCESS);
				break;
			case 'p':
				part = validate_part(optarg);
				if(part == NULL) {
					fprintf(stderr, "Unknown part: %s. Use -h to list supported parts.\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
			case 'l':
				lutmapper = tilm_get_mapper_by_handle(optarg);
				if(lutmapper < 0) {
					fprintf(stderr, "Unknown LUT mapping algorithm: %s. Use -h to list supported algorithms.\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
			default:
				fprintf(stderr, "Invalid option passed. Use -h for help.\n");
				exit(EXIT_FAILURE);
				break;
		}
	}
	if((argc - optind) != 3) {
		fprintf(stderr, "Exactly 3 file names must be passed. Use -h for help.\n");
		exit(EXIT_FAILURE);
	}
	input_lhd = argv[optind];
	output_edf = argv[optind+1];
	output_sym = argv[optind+2];
	return 0;
}

