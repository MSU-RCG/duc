
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <stdint.h>

#include "cmd.h"
#include "duc.h"
#include "ducrc.h"


static int opt_bytes = 0;
static char *opt_database = NULL;
static int opt_force = 0;          
static int opt_check_hard_links = 0;
static int opt_hide_file_names = 0;     
static int opt_max_depth = 0;       
static int opt_one_file_system = 0;
static int opt_progress = 0;
static int opt_uncompressed = 0;   
static duc_index_req *req;


static int index_init(duc *duc, int argc, char **argv)
{
	req = duc_index_req_new(duc);

	return 0;
}


void progress_cb(struct duc_index_report *rep, void *ptr)
{
	static int n = 0;
	char meter[] = "--------";
	meter[7 - abs(n-7)] = '#';
	n = (n+1) % 14;

	char *siz = duc_human_size(rep->size_actual, opt_bytes);
	char *fs = duc_human_size(rep->file_count, opt_bytes);
	char *ds = duc_human_size(rep->dir_count, opt_bytes);

	printf("\e[K[%s] Indexed %sb in %s files and %s directories\r", meter, siz, fs, ds);
	fflush(stdout);

	free(siz);
	free(fs);
	free(ds);
}


static int index_main(duc *duc, int argc, char **argv)
{
	duc_index_flags index_flags = 0;
	int open_flags = DUC_OPEN_RW | DUC_OPEN_COMPRESS;
	
	if(opt_force) open_flags |= DUC_OPEN_FORCE;
	if(opt_max_depth) duc_index_req_set_maxdepth(req, opt_max_depth);
	if(opt_one_file_system) index_flags |= DUC_INDEX_XDEV;
	if(opt_hide_file_names) index_flags |= DUC_INDEX_HIDE_FILE_NAMES;
	if(opt_check_hard_links) index_flags |= DUC_INDEX_CHECK_HARD_LINKS;
	if(opt_uncompressed) open_flags &= ~DUC_OPEN_COMPRESS;
	if(opt_progress) duc_index_req_set_progress_cb(req, progress_cb, NULL);

	if(argc < 1) {
		duc_log(duc, DUC_LOG_WRN, "Required index PATH missing.");
		return -2;
	}
	
	int r = duc_open(duc, opt_database, open_flags);
	if(r != DUC_OK) {
		duc_log(duc, DUC_LOG_WRN, "%s", duc_strerror(duc));
		return -1;
	}

	/* Index all paths passed on the cmdline */

	int i;
	for(i=0; i<argc; i++) {

		struct duc_index_report *report;
		report = duc_index(req, argv[i], index_flags);
		if(report == NULL) {
			duc_log(duc, DUC_LOG_WRN, "%s", duc_strerror(duc));
			continue;
		}

		char *siz_apparent = duc_human_size(report->size_apparent, opt_bytes);
		char *siz_actual = duc_human_size(report->size_actual, opt_bytes);

		if(r == DUC_OK) {
			char *s = duc_human_duration(report->time_start, report->time_stop);
			duc_log(duc, DUC_LOG_INF, 
					"Indexed %zu files and %zu directories, (%sB apparent, %sB actual) in %s", 
					report->file_count, 
					report->dir_count,
					siz_apparent,
					siz_actual,
					s);
			free(s);
		} else {
			duc_log(duc, DUC_LOG_WRN, "An error occured while indexing: %s", duc_strerror(duc));
		}
		free(siz_apparent);
		free(siz_actual);

		duc_index_report_free(report);
	}

	duc_close(duc);

	return 0;
}


static void fn_exclude(const char *val)
{
	duc_index_req_add_exclude(req, val);
}


static struct ducrc_option options[] = {
	{ &opt_bytes,           "bytes",           'b', DUCRC_TYPE_BOOL,   "show file size in exact number of bytes" },
	{ &opt_database,        "database",        'd', DUCRC_TYPE_STRING, "use database file ARG" },
	{ &fn_exclude,          "exclude",         'e', DUCRC_TYPE_FUNC,   "exclude files matching ARG"  },
	{ &opt_check_hard_links,"check-hard-links",'H', DUCRC_TYPE_BOOL,   "count hard links only once" },
	{ &opt_force,           "force",           'f', DUCRC_TYPE_BOOL,   "force writing in case of corrupted db" },
	{ &opt_hide_file_names, "hide-file-names",  0 , DUCRC_TYPE_BOOL,   "hide file names in index (privacy)" },
	{ &opt_max_depth,       "max-depth",       'm', DUCRC_TYPE_INT,    "limit directory names to given depth" },
	{ &opt_one_file_system, "one-file-system", 'x', DUCRC_TYPE_BOOL,   "don't cross filesystem boundaries" },
	{ &opt_progress,        "progress",        'p', DUCRC_TYPE_BOOL,   "show progress during indexing" },
	{ &opt_uncompressed,    "uncompressed",     0 , DUCRC_TYPE_BOOL,   "do not use compression for database" },
	{ NULL }
};


struct cmd cmd_index = {
	.name = "index",
	.description = "Index filesystem",
	.usage = "[options] PATH ...",
	.init = index_init,
	.main = index_main,
	.options = options,
};

/*
 * End
 */

