
#include "config.h"

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cmd.h"
#include "duc.h"

#ifdef HAVE_LIBNCURSES

#include <ncurses.h>


static char type_char[] = {
        [DT_BLK]     = ' ',
        [DT_CHR]     = ' ',
        [DT_DIR]     = '/',
        [DT_FIFO]    = '|',
        [DT_LNK]     = '>',
        [DT_REG]     = ' ',
        [DT_SOCK]    = '@',
        [DT_UNKNOWN] = ' ',
};

static int opt_classify = 1;
static int opt_apparent = 0;
static int opt_bytes = 0;
static int opt_graph = 1;
static char *opt_database = NULL;


static int cols = 80;
static int rows = 25;


static duc_dir *do_dir(duc_dir *dir, int depth)
{
	int top = 0;
	int cur = 0;

	for(;;) {
	
		duc_size_type size_type = opt_apparent ? DUC_SIZE_TYPE_APPARENT : DUC_SIZE_TYPE_ACTUAL;

		/* Iterate all dirents to find largest size */

		duc_dir_seek(dir, 0);

		off_t size_max = 1;
		struct duc_dirent *e;
		while( (e = duc_dir_read(dir, size_type)) != NULL) {
			off_t size = opt_apparent ? e->size_apparent : e->size_actual;
			if(size > size_max) size_max = size;
		}

		int count = duc_dir_get_count(dir);
		int pgsize = rows - 2;
		
		/* Check boundaries */

		if(cur < 0) cur = 0;
		if(cur > count - 1) cur = count - 1;
		if(cur < top) top = cur;
		if(cur > top + pgsize - 1) top = cur - pgsize + 1;
		if(top < 0) top = 0;


		/* Draw header */

		char *path = duc_dir_get_path(dir);
		attrset(A_REVERSE);
		mvhline(0, 0, ' ', cols);
		mvprintw(0, 1, " %s ", path);
		attrset(0);
		free(path);


		/* Draw footer */
	
		off_t size = duc_dir_get_size(dir, size_type);
		char *siz = duc_human_size(size, opt_bytes);
		char *cnt = duc_human_size(count, opt_bytes);
		attrset(A_REVERSE);
		mvhline(rows-1, 0, ' ', cols);
		mvprintw(rows-1, 0, " Total %sB in %s files/directories", siz, cnt);
		attrset(0);
		free(siz);
		free(cnt);


		/* Draw dirents */
	
		duc_dir_seek(dir, top);

		int i;
		int y = 1;

		for(i=top; i<top + pgsize; i++) {
			
			struct duc_dirent *e = duc_dir_read(dir, size_type);

			attrset(cur == i ? A_REVERSE : 0);
			mvhline(y, 0, ' ', cols);

			if(e) {

				off_t size = opt_apparent ? e->size_apparent : e->size_actual;
		
				size_t max_size_len = opt_bytes ? 12 : 7;

				char class = ' ';
				if(opt_classify && e->type < sizeof(type_char)) {
					class = type_char[e->type];
				}
				
				char *siz = duc_human_size(size, opt_bytes);
				printw("%*s", max_size_len, siz);
				free(siz);

				if(cur != i) attron(A_BOLD);
				printw(" %s%c", e->name, class);
				attroff(A_BOLD);

				int w = cols - 30;
				if(w > cols / 2) w = cols / 2;
				if(opt_graph && w > 2) {
					int j;
					off_t g = w * size / size_max;
					mvprintw(y, cols - w - 4, " [");
					for(j=0; j<w; j++) printw("%s", j < g ? "=" : " ");
					printw("] ");
				}

			} else {

				attrset(A_DIM);
				mvprintw(y, 0, "~");
			}

			y++;
		}

		duc_dir *dir2 = NULL;

		/* Handle key */

		int c = getch();

		switch(c) {
			case KEY_UP: cur--; break;
			case KEY_DOWN: cur++; break;
			case KEY_PPAGE: cur -= pgsize; break;
			case KEY_NPAGE: cur += pgsize; break;
			case KEY_RESIZE: getmaxyx(stdscr, rows, cols); break;
			case 'a': opt_apparent ^= 1; break;
			case 'b': opt_bytes ^= 1; break;
			case 'c': opt_classify ^= 1; break;
			case 'g': opt_graph ^= 1; break;

			case 27:
			case 'q': 
				  exit(0); break;

			case KEY_BACKSPACE:
			case KEY_LEFT:
				  if(depth > 0) {
					  return NULL;
				  } else {
					  dir2 = duc_dir_openat(dir, "..");
					  if(dir2) {
						  do_dir(dir2, 0);
						  duc_dir_close(dir2);
					  }
				  }
				  break;

			case KEY_RIGHT:
			case '\r':
			case '\n': 
				  duc_dir_seek(dir, cur);
				  struct duc_dirent *e = duc_dir_read(dir, size_type);
				  if(e->type == DT_DIR) {
					dir2 = duc_dir_openent(dir, e);
					if(dir2) {
						do_dir(dir2, depth + 1);
						duc_dir_close(dir2);
					}
				  }
				  break;
		}

	}

	return NULL;
}


static void bye(void)
{
	endwin();
}


static int ui_main(duc *duc, int argc, char **argv)
{
	char *path = ".";
	if(argc > 0) path = argv[0];


	int r = duc_open(duc, opt_database, DUC_OPEN_RO);
	if(r != DUC_OK) {
		return -1;
	}

	duc_dir *dir = duc_dir_open(duc, path);
	if(dir == NULL) {
		return -1;
	}

	initscr();
	atexit(bye);
	cbreak();
	noecho();
	nonl();
	keypad(stdscr, 1);
	//halfdelay(1);
	curs_set(0);
	start_color();
	use_default_colors();
	getmaxyx(stdscr, rows, cols);

	do_dir(dir, 0);

	duc_dir_close(dir);
	duc_close(duc);

	return 0;
}

static struct ducrc_option options[] = {
	{ &opt_apparent,  "apparent",  'a', DUCRC_TYPE_BOOL,   "show apparent instead of actual file size" },
	{ &opt_bytes,     "bytes",     'b', DUCRC_TYPE_BOOL,   "show file size in exact number of bytes" },
	{ &opt_database,  "database",  'd', DUCRC_TYPE_STRING, "select database file to use [~/.duc.db]" },
	{ NULL }
};


#else

int ui_main(int argc, char *argv[])
{
	duc_log(NULL, DUC_LOG_WRN, "'duc ui' is not supported on this platform.");
	return -1;
}

static struct ducrc_option options[] = {
	{ NULL }
};

#endif

struct cmd cmd_ui = {
	.name = "ui",
	.description = "interactive ncurses user interface",
	.usage = "[options] [PATH]",
	.main = ui_main,
	.options = options,
};


/*
 * End
 */

