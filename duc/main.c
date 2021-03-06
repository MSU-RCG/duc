
#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "duc.h"
#include "db.h"
#include "cmd.h"
#include "ducrc.h"


struct cmd cmd_help;
struct cmd cmd_info;
struct cmd cmd_index;
struct cmd cmd_ls;
struct cmd cmd_gui;
struct cmd cmd_graph;
struct cmd cmd_xml;
struct cmd cmd_cgi;
struct cmd cmd_ui;


struct cmd *cmd_list[] = {
	&cmd_cgi,
	&cmd_graph,
	&cmd_gui,
	&cmd_help,
	&cmd_index,
	&cmd_info,
	&cmd_ls,
	&cmd_ui,
	&cmd_xml,
};

#define SUBCOMMAND_COUNT (sizeof(cmd_list) / sizeof(cmd_list[0]))

static struct cmd *find_cmd_by_name(const char *name);
static void help_cmd(struct cmd *cmd);


static int opt_debug = 0;
static int opt_verbose = 0;
static int opt_quiet = 0;
static int opt_help = 0;


static struct ducrc_option global_options[] = {
	{ &opt_debug,    "debug",      0, DUCRC_TYPE_BOOL,   "increase verbosity to debug level" },
	{ &opt_help,     "help",     'h', DUCRC_TYPE_BOOL,   "show help" },
	{ &opt_quiet,    "quiet",    'q', DUCRC_TYPE_BOOL,   "quiet mode, do not print any warning" },
	{ &opt_verbose,  "verbose",  'v', DUCRC_TYPE_BOOL,   "increase verbosity" },
	{ NULL }
};


int main(int argc, char **argv)
{
	int r;

	/* Open duc context */
	
	duc *duc = duc_new();
	if(duc == NULL) {
		duc_log(duc, DUC_LOG_WRN, "Error creating duc context");
		return -1;
	}

	/* Find subcommand */

	struct cmd *cmd = NULL;

	if(argc >= 2) {
		cmd = find_cmd_by_name(argv[1]);
	}

	if(cmd == NULL) {
		if(getenv("QUERY_STRING")) {
			cmd = &cmd_cgi;
		} else {
			cmd = &cmd_help;
		}
	}


	/* Register options */

	struct ducrc *ducrc = ducrc_new(cmd->name);
	ducrc_add_options(ducrc, global_options);
	ducrc_add_options(ducrc, cmd->options);

	/* Call init function */

	if(cmd->init) {
		int r = cmd->init(duc, argc, argv);
		if(r != 0) exit(r);
	}

	/* Read configuration files from /etc/ducrc, ~/.ducrc and .ducrc and
	 * finally from the command line. Newer options will override older
	 * options */

	ducrc_read(ducrc, "/etc/ducrc");
	char *home = getenv("HOME");
	if(home) {
		char tmp[PATH_MAX];
		snprintf(tmp, sizeof(tmp), "%s/.ducrc", home);
		ducrc_read(ducrc, tmp);
	}
	ducrc_read(ducrc, "./.ducrc");
	r = ducrc_getopt(ducrc, &argc, &argv);

	if(r == -1) {
		fprintf(stderr, "Try 'duc --help' for more information.\n");
		exit(1);
	}

	/* Help requested ? */

	if(opt_help) {
		help_cmd(cmd);
		return(EXIT_SUCCESS);
	}


	/* Set log level */

	duc_log_level log_level = DUC_LOG_WRN;
	if(opt_quiet) log_level = DUC_LOG_FTL;
	if(opt_verbose) log_level = DUC_LOG_INF;
	if(opt_debug) log_level = DUC_LOG_DMP;
	duc_set_log_level(duc, log_level);


	/* Handle command */

	r = cmd->main(duc, argc, argv);
	if(r == -2) help_cmd(cmd);

	
	/* Cleanup */

	duc_del(duc);
	ducrc_free(ducrc);
	return (r == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


static struct cmd *find_cmd_by_name(const char *name)
{
	int i;

	for(i=0; i<SUBCOMMAND_COUNT; i++) {
		struct cmd *cmd = cmd_list[i];
		if(strcmp(cmd->name, name) == 0) {
			return cmd;
		}
	}
	return NULL;
}


static void show_options(struct ducrc_option *o)
{
	while(o && o->longopt) {
		char s[4] = "";
		char l[20] = "";

		if(o->shortopt) snprintf(s, sizeof(s), "-%c,", o->shortopt); 

		if(o->type != DUCRC_TYPE_BOOL) {
			snprintf(l, sizeof(l), "%s=VAL", o->longopt);
		} else {
			snprintf(l, sizeof(l), "%s", o->longopt);
		}

		printf("  %-4.4s --%-20.20s", s, l);
		if(o->description) printf("%s", o->description); 
		printf("\n");

		o++;
	}
}


static void help_cmd(struct cmd *cmd)
{
	if(cmd->usage) {
		printf("usage: duc %s %s\n", cmd->name, cmd->usage);
		printf("\n");
	}
	
	printf("Options for the command '%s':\n", cmd->name);
	show_options(cmd->options);

	printf("\n");
	printf("Global options:\n");
	show_options(global_options);
}


int opt_all = 0;


static int help_main(duc *duc, int argc, char **argv)
{
	struct cmd *cmd = NULL;

	if(argc > 0) cmd = find_cmd_by_name(argv[0]);

	if(cmd) {
		help_cmd(cmd);
	} else {
		printf("usage: duc <cmd> [options] [args]\n"
			"\n"
			"Available subcommands:\n"
			"\n"
		);

		int i;
		for(i=0; i<SUBCOMMAND_COUNT; i++) {
			struct cmd *c = cmd_list[i];

			if(opt_all) {
				printf("duc %s %s: %s\n", c->name, c->usage, c->description);
				printf("\n");
				show_options(c->options);
				printf("\n");
			} else {
				printf("  %-10.10s: %s\n", c->name, c->description);
			}
		}

		if(!opt_all) {
			printf("\nUse 'duc help --all' for a complete list of all options for all subcommands.\n");
		}
	}

	return 0;
}


static struct ducrc_option help_options[] = {
	{ &opt_all,     "all",     'a', DUCRC_TYPE_BOOL,   "show complete help for all commands" },
	{ NULL }
};

struct cmd cmd_help = {
	.name = "help",
	.description = "Show help",
	.usage = "[options]",
	.main = help_main,
	.options = help_options,
};

/*
 * End
 */
