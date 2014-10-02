
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <stdint.h>

#include "duc.h"
#include "duc-graph.h"
#include "cmd.h"

static struct option longopts[] = {
	{ "database",       required_argument, NULL, 'd' },
	{ "levels",         required_argument, NULL, 'l' },
	{ "output",         required_argument, NULL, 'o' },
	{ "size",           required_argument, NULL, 's' },
	{ "verbose",        required_argument, NULL, 'v' },
	{ NULL }
};


static int graph_main(int argc, char **argv)
{
	int c;
	char *path_db = NULL;
	int size = 800;
	char *path_out = NULL;
	char *path_out_default = "duc.png";
	duc_log_level loglevel = DUC_LOG_WRN;
	int max_level = 4;
	enum duc_graph_file_format format = DUC_GRAPH_FORMAT_PNG;

	while( ( c = getopt_long(argc, argv, "d:f:l:o:s:qv", longopts, NULL)) != EOF) {

		switch(c) {
			case 'd':
				path_db = optarg;
				break;
			case 'f':
				if(strcasecmp(optarg, "svg") == 0) {
					format = DUC_GRAPH_FORMAT_SVG;
					path_out_default = "duc.svg";
				}
				if(strcasecmp(optarg, "pdf") == 0) {
					format = DUC_GRAPH_FORMAT_PDF;
					path_out_default = "duc.pdf";
				}
				break;
			case 'l':
				max_level = atoi(optarg);
				break;
			case 'o':
				path_out = optarg;
				break;
			case 'q':
				loglevel = DUC_LOG_FTL;
				break;
			case 's':
				size = atoi(optarg);
				break;
			case 'v':
				if(loglevel < DUC_LOG_DMP) loglevel ++;
				break;
			default:
				return -2;
		}
	}

	if(path_out == NULL) path_out = path_out_default;

	argc -= optind;
	argv += optind;
	
	char *path = ".";
	if(argc > 0) path = argv[0];

        /* Open duc context */

	duc *duc = duc_new();
	if(duc == NULL) {
                fprintf(stderr, "Error creating duc context\n");
                return -1;
        }
	
	duc_set_log_level(duc, loglevel);

        int r = duc_open(duc, path_db, DUC_OPEN_RO);
        if(r != DUC_OK) {
                fprintf(stderr, "%s\n", duc_strerror(duc));
                return -1;
        }

        duc_dir *dir = duc_dir_open(duc, path);
        if(dir == NULL) {
                fprintf(stderr, "%s\n", duc_strerror(duc));
                return -1;
        }

	duc_graph *graph = duc_graph_new(duc);
	duc_graph_set_size(graph, size);
	duc_graph_set_max_level(graph, max_level);

	FILE *f = fopen(path_out, "w");
	if(f == NULL) {
		return -1;
	}

	duc_graph_draw_file(graph, dir, format, f);

	duc_graph_free(graph);
	duc_dir_close(dir);
	duc_close(duc);

	return 0;
}


	
struct cmd cmd_graph = {
	.name = "graph",
	.description = "Draw graph",
	.usage = "[options] [PATH]",
	.help = 
		"  -d, --database=ARG      use database file ARG [~/.duc.db]\n"
		"  -f, --format=ARG        select output format <png|svg|pdf> [png]\n"
	        "  -l, --levels=ARG        draw up to ARG levels deep [4]\n"
		"  -o, --output=ARG        output file name [duc.png]\n"
	        "  -s, --size=ARG          image size [800]\n"
		"  -q, --quiet             quiet mode, do not print any warnings\n"
		"  -v, --verbose           verbose mode, can be passed two times for debugging\n",
	.main = graph_main
};

/*
 * End
 */

