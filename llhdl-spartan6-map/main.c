#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <banner/banner.h>

#include <tilm/tilm.h>

#include "flow.h"

struct flow_settings flow_settings = {
	.part = "xc6slx45-fgg484-2",

	.io_buffers = 1,
	.dsp = 1,
	.carry_chains = 1,
	.srl16 = 1,
	.dedicated_muxes = 1,
	.prune = 1,
	
	.lut_mapper = TILM_DEFAULT,
	.lut_max_inputs = 6
};

static const char *parts[] = {
	"xc6slx45-fgg484-2",
	"xc6slx45-fgg484-3",
	NULL
};

struct option_desc {
	const char *handle;
	const char *description;
	int *sw;
};

struct option_desc options[] = {
	{
		.handle = "io-buffers",
		.description = "Insert I/O buffers",
		.sw = &flow_settings.io_buffers
	},
	{
		.handle = "dsp",
		.description = "Use dedicated DSP blocks",
		.sw = &flow_settings.dsp
	},
	{
		.handle = "carry-chains",
		.description = "Use carry chains",
		.sw = &flow_settings.carry_chains
	},
	{
		.handle = "srl16",
		.description = "Use the shift register mode of LUTs",
		.sw = &flow_settings.srl16
	},
	{
		.handle = "dedicated-muxes",
		.description = "Use dedicated multiplexers (MUXF7, MUXF8)",
		.sw = &flow_settings.dedicated_muxes
	},
	{
		.handle = "prune",
		.description = "Prune final netlist",
		.sw = &flow_settings.prune
	},
};

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

static void list_parts()
{
	int i;
	
	i = 0;
	while(parts[i] != NULL) {
		printf("      %s%s\n", parts[i], strcmp(parts[i], flow_settings.part) == 0 ? " (default)" : "");
		i++;
	}
}

static void list_lutmappers()
{
	int i;
	
	for(i=0;i<TILM_COUNT;i++)
		printf("      %s: %s mapper%s\n",
			tilm_mappers[i].handle,
			tilm_mappers[i].description,
			i == flow_settings.lut_mapper ? " (default)" : "");
}

static void list_options()
{
	int i;
	
	for(i=0;i<sizeof(options)/sizeof(options[0]);i++)
		printf("    -f[no-]%s: %s (default: %s)\n",
			options[i].handle,
			options[i].description,
			*(options[i].sw) ? "enabled" : "disabled");
}

static void help()
{
	banner("\nXilinx Spartan-6 mapper");
	printf("Usage: llhdl-spartan6-map [parameters] <input.lhd>\n\n");
	printf("The input design in LLHDL interchange format (input.lhd) is mandatory.\n");
	printf("Parameters are:\n");
	printf("  -h Display this help text and exit.\n");
	printf("  -p <part>: Select part. Supported values are:\n");
	list_parts();
	printf("  -f <[no-]option>: Enable or disable options. Supported options are:\n");
	list_options();
	printf("  -l <algo>: Select LUT mapping algorithm. Supported values are:\n");
	list_lutmappers();
	printf("  -i <n>: Use at most that many LUT inputs (3-6, default: %d)\n", flow_settings.lut_max_inputs);
	printf("Output file(s) selection (can be combined):\n");
	printf("  -o <netlist.anl>: Write a netlist in Antares format.\n");
	printf("  -e <netlist.edf>: Write a netlist in EDIF format.\n");
	printf("  -d <netlist.dot>: Write a DOT (Graphviz) representation of the netlist.\n");
	printf("  -s <symbols.sym>: Write a symbols file.\n");
}

static void handle_option(char *opt)
{
	int val;
	int i;
	
	val = 1;
	if(strncmp(opt, "no-", 3) == 0) {
		val = 0;
		opt += 3;
	}
	for(i=0;i<sizeof(options)/sizeof(options[0]);i++)
		if(strcmp(opt, options[i].handle) == 0) {
			*(options[i].sw) = val;
			return;
		}
	fprintf(stderr, "Invalid option: '%s'.\n", opt);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int opt;
	
	while((opt = getopt(argc, argv, "hp:f:l:i:o:e:d:s:")) != -1) {
		switch(opt) {
			case 'h':
				help();
				exit(EXIT_SUCCESS);
				break;
			case 'p':
				flow_settings.part = validate_part(optarg);
				if(flow_settings.part == NULL) {
					fprintf(stderr, "Unknown part: %s. Use -h to list supported parts.\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
			case 'f':
				handle_option(optarg);
				break;
			case 'l':
				flow_settings.lut_mapper = tilm_get_mapper_by_handle(optarg);
				if(flow_settings.lut_mapper < 0) {
					fprintf(stderr, "Unknown LUT mapping algorithm: %s. Use -h to list supported algorithms.\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
			case 'i':
				flow_settings.lut_max_inputs = atoi(optarg);
				if((flow_settings.lut_max_inputs < 3) || (flow_settings.lut_max_inputs > 6)) {
					fprintf(stderr, "Invalid number of maximum LUT inputs.\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'o':
				flow_settings.output_anl = stralloc(optarg);
				break;
			case 'e':
				flow_settings.output_edf = stralloc(optarg);
				break;
			case 'd':
				flow_settings.output_dot = stralloc(optarg);
				break;
			case 's':
				flow_settings.output_sym = stralloc(optarg);
				break;
			default:
				fprintf(stderr, "Invalid option passed. Use -h for help.\n");
				exit(EXIT_FAILURE);
				break;
		}
	}
	
	if((argc - optind) != 1) {
		fprintf(stderr, "llhdl-spartan6-map: missing input file. Use -h for help.\n");
		exit(EXIT_FAILURE);
	}
	flow_settings.input_lhd = argv[optind];

	run_flow(&flow_settings);
	
	free(flow_settings.output_anl);
	free(flow_settings.output_edf);
	free(flow_settings.output_dot);
	free(flow_settings.output_sym);

	return 0;
}

