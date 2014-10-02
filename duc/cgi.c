#include "config.h"

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <libgen.h>

#include "cmd.h"
#include "duc.h"
#include "duc-graph.h"

/* John */

/*
 * Simple parser for CGI parameter line. Does not escape CGI strings.
 */

struct param {
	char *key;
	char *val;
	struct param *next;
};

/* Right out of K&R */
struct tnode {
    char *key;
    char *file;
    struct tnode *left;
    struct tnode *right;
};

struct param *param_list = NULL;

/* Helpers to buid tree of PATH names and the full path to said PATH
   so that we don't expose internal filesystem details to external
   users, such as where the DB(s) are actually stored.  All pulled
   from K&R "The C Programming Language" because it's still the best.
   */

struct tnode *talloc() {
    return((struct tnode *) malloc(sizeof(struct tnode)));
}

struct tnode *tree( struct tnode *r, char *k, char *w) {
    int cond;

    if (r == NULL) {  /* Empty tree... */
	r = talloc();
	r->key = k;
	r->file = w;
	r->left = r->right = NULL;
    }

    else {
	cond = strcmp(k, r->key);
	if (cond == 0) {
	    fprintf(stderr,"Error!  We found a duplicate key: %s\n",k);
	    exit(1);
	} 
	else if (cond < 0) {
	    r->left = tree(r->left, k, w);
	}
	else {
	    r->right = tree(r->right, k, w);
	}	
    }
    return(r);
}

void treeprint(struct tnode *p) {
    if (p != NULL) {
	treeprint(p->left);
	printf(" %14s -> %s\n",p->key, p->file);
	treeprint(p->right);
    }
}

struct tnode * treefindfile(struct tnode *p, char *k) {
    int cond;
    if (p == NULL) {
	return(NULL);
    }

    if ((cond = strcmp(k, p->key)) == 0) {
	return(p);
    } 
    else if (cond < 0) {
	return(treefindfile(p->left,k));
    }
    else {
	return(treefindfile(p->right,k));
    }
}


/* silly little helper */
static void print_html_header(const char *title) {
	printf(
		"Content-Type: text/html\n"
		"\n"
		"<!DOCTYPE html>\n"
		"<HEAD>\n"
		"<STYLE>\n"
		"body { font-family: 'arial', 'sans-serif'; font-size: 11px; }\n"
		"table, thead, tbody, tr, td, th { font-size: inherit; font-family: inherit; }\n"
		"#list { 100%%; }\n"
		"#list td { padding-left: 5px; }\n"
		"</STYLE>\n"
		"<TITLE>%s</TITLE>\n"
		"</HEAD>\n",
		title
	);
}	

static int cgi_parse(void)
{
	char *qs = getenv("QUERY_STRING");
	if(qs == NULL) return -1;

	char *p = qs;

	for(;;) {

		char *pe = strchr(p, '=');
		if(!pe) break;
		char *pn = strchr(pe, '&');
		if(!pn) pn = pe + strlen(pe);

		char *key = p;
		int keylen = pe-p;
		char *val = pe+1;
		int vallen = pn-pe-1;

		struct param *param = malloc(sizeof(struct param));
		assert(param);

		param->key = malloc(keylen+1);
		assert(param->key);
		strncpy(param->key, key, keylen);

		param->val = malloc(vallen+1);
		assert(param->val);
		strncpy(param->val, val, vallen);
		
		param->next = param_list;
		param_list = param;

		if(*pn == 0) break;
		p = pn+1;
	}

	return 0;
}


static char *cgi_get(const char *key)
{
	struct param *param = param_list;

	while(param) {
		if(strcmp(param->key, key) == 0) {
			return param->val;
		}
		param = param->next;
	}

	return NULL;
}

static void do_index(duc *duc, duc_graph *graph, duc_dir *dir)
{
    print_html_header("Index");

	char *path = cgi_get("path");
	char *script = getenv("SCRIPT_NAME");
	if(!script) return;

	char *qs = getenv("QUERY_STRING");
	int x = 0, y = 0;
	if(qs) {
		char *p1 = strchr(qs, '?');
		if(p1) {
			char *p2 = strchr(p1, ',');
			if(p2) {
				x = atoi(p1+1);
				y = atoi(p2+1);
			}
		}
	}

	if(x || y) {
		//char newpath[PATH_MAX];
		//duc_graph_xy_to_path(graph, dir, x, y, newpath, sizeof newpath);
	}

	struct duc_index_report *report;
	int i = 0;

	printf("<center>");

	printf("<table id=list>");
	printf("<tr>");
	printf("<th>Path</th>");
	printf("<th>Size</th>");
	printf("<th>Files</th>");
	printf("<th>Directories</th>");
	printf("<th>Date</th>");
	printf("<th>Time</th>");
	printf("</tr>");

	while( (report = duc_get_report(duc, i)) != NULL) {

		char ts_date[32];
		char ts_time[32];
		struct tm *tm = localtime(&report->time_start.tv_sec);
		strftime(ts_date, sizeof ts_date, "%Y-%m-%d",tm);
		strftime(ts_time, sizeof ts_time, "%H:%M:%S",tm);

		char url[PATH_MAX];
		snprintf(url, sizeof url, "%s?cmd=index&path=%s", script, report->path);

		char siz[32];
		duc_humanize(report->size_total, siz, sizeof siz);

		printf("<tr>");
		printf("<td><a href='%s'>%s</a></td>", url, report->path);
		printf("<td>%s</td>", siz);
		printf("<td>%zu</td>", report->file_count);
		printf("<td>%zu</td>", report->dir_count);
		printf("<td>%s</td>", ts_date);
		printf("<td>%s</td>", ts_time);
		printf("</tr>\n");

		duc_index_report_free(report);
		i++;
	}
	printf("</table>");

	if(path) {
		printf("<a href='%s?cmd=index&path=%s&'>", script, path);
		printf("<img src='%s?cmd=image&path=%s' ismap='ismap'>\n", script, path);
		printf("</a><br>");
		printf("<b>%s</b><br>", path);
	}
	fflush(stdout);
}


void do_image(duc *duc, duc_graph *graph, duc_dir *dir)
{
	printf("Content-Type: image/png\n");
	printf("\n");

	if(dir) {
		duc_graph_draw_file(graph, dir, DUC_GRAPH_FORMAT_PNG, stdout);
	}
}




static int cgi_main(int argc, char **argv)
{
	int r;
	
	r = cgi_parse();
	if(r != 0) {
	    fprintf(stderr, 
		    "The 'cgi' subcommand is used for integrating Duc into a web server.\n"
		    "Please refer to the documentation for instructions how to install and configure.\n"
		);
		return(-1);
	}

	char *path_db = NULL;
	char *db_dir = NULL;

	struct option longopts[] = {
		{ "database",       required_argument, NULL, 'd' },
		{ "dbdir",          required_argument, NULL, 'D' },
		{ NULL }
	};

	int c;
	while( ( c = getopt_long(argc, argv, "d:D:", longopts, NULL)) != EOF) {

	    switch(c) {
	    case 'd':
		path_db = realpath(optarg, NULL);
		break;
	    case 'D':
		db_dir = realpath(optarg, NULL);
		break;
	    default:
		return -2;
	    }
	}
	

	char *cmd = cgi_get("cmd");
	char *path = cgi_get("path");
	if(cmd == NULL) cmd = "index";
	
	duc *duc = duc_new();
	if(duc == NULL) {
	    print_html_header("Error creating duc context\n");
	    printf("<BODY>Sorry, we had a problem with the CGI script.\n</BODY></HTML>\n");
	    return -1;
        }

	if (db_dir) {
	    glob_t bunch_of_dbs;
	    char **db_file;
	    size_t n = duc_find_dbs(db_dir, &bunch_of_dbs);
	    int i = 0;

	    print_html_header("DUC db_dir list");
	    
	    printf("<BODY>\n<H1>DUC db_dir list: %s</H1>\n<UL>\n",path);
	    printf("<br>Found %zu (%zu) DBs to look at.<br>\n", n, bunch_of_dbs.gl_pathc);
	    for (db_file = bunch_of_dbs.gl_pathv; i < n; db_file++, i++) {
                printf("  <LI> <A HREF= %s\n", *db_file);
	    }
	    printf("</UL>\n</BODY>\n</HTML>\n");
	    exit(1);
	}

	path_db = duc_pick_db_path(path_db);
        r = duc_open(duc, path_db, DUC_OPEN_RO);
        if(r != DUC_OK) {
	    print_html_header("Content-Type: text/plain\n\n");
	    printf("<BODY>%s\n</BODY></HTML>", duc_strerror(duc));
		return -1;
        }

	duc_dir *dir = NULL;

	if(path) {
		dir = duc_opendir(duc, path);
		if(dir == NULL) {
			fprintf(stderr, "%s\n", duc_strerror(duc));
			return 0;
		}
	}

	duc_graph *graph = duc_graph_new(duc);
	duc_graph_set_size(graph, 600);
	duc_graph_set_max_level(graph, 4);

	if(strcmp(cmd, "index") == 0) do_index(duc, graph, dir);
	if(strcmp(cmd, "image") == 0) do_image(duc, graph, dir);

	if(dir) duc_closedir(dir);
	duc_close(duc);
	duc_del(duc);

	return 0;
}


struct cmd cmd_cgi = {
	.name = "cgi",
	.description = "CGI interface",
	.usage = "[options] [PATH]",
	.help = "\
  -d, --database=ARG      use database file ARG [~/.duc.db]\n\
  -D, --datadir=ARG       use directory of database file(s) ARG\n",
	.main = cgi_main,
		
};


/*
 * End
 */

