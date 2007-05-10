/* 
 * (C)opyright MMVII Robert Manea <rob dot manea  at gmail dot com>
 * See LICENSE file for license details.
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
//#include <X11/keysym.h>
#include <pthread.h>

#define FONT	    "-*-fixed-*-*-*-*-*-*-*-*-*-*-*-*"
#define BGCOLOR		"#ab0b0b"
#define FGCOLOR		"#efefef"

//#define BUF_SIZE    4096
#define BUF_SIZE    1024

/* gui data structures */
enum { ColFG, ColBG, ColLast };

typedef struct DZEN Dzen;
typedef struct Fnt Fnt;
typedef struct TW TWIN;
typedef struct SW SWIN;

struct Fnt {
    XFontStruct *xfont;
    XFontSet set;
    int ascent;
    int descent;
    int height;
};

/* title window */
struct TW {
    int x, y, width, height;
    int screen;
    char *fnt;
    char *bg;
    char *fg;

    Window win;
    Drawable drawable;
    Bool autohide;
    Bool ishidden;
};

/* slave window */
struct SW {
    int x, y, width, height;
    int screen;
    char *fnt;
    char *bg;
    char *fg;

    Window win;
    Window *line;
    Drawable drawable;

    char *tbuf[BUF_SIZE];
    int tcnt;
    int max_lines;
    int first_line_vis;
    int last_line_vis;
    int sel_line;

    Bool ismenu;
    Bool issticky;
    Bool ispersistent;
    Bool ismapped;
};

/* TODO: Remove unused variables */
struct DZEN {
    int x, y, w, h;
    Bool running;
    unsigned long norm[ColLast];

    TWIN title_win;
    SWIN slave_win;

    /* to be removed */
    char *fnt;
    char *bg;
    char *fg;
    int mw, mh;
    /*---------------*/

    Display *dpy;
    int screen;
    unsigned int depth;

    Visual *visual;
    GC gc, rgc;
    Fnt font;

    /* position */
    int hy, hw;
    int cur_line;

    pthread_t read_thread;
    pthread_mutex_t mt;

    int ret_val;
};

extern Dzen dzen;

/* draw.c */
extern void drawtext(const char *text,
        int reverse,
        int line);	                        
extern unsigned long getcolor(const char *colstr);	/* returns color of colstr */
extern void setfont(const char *fontstr);		    /* sets global font */
extern unsigned int textw(const char *text);		/* returns width of text in px */

/* util.c */
extern void *emalloc(unsigned int size);		/* allocates memory, exits on error */
extern void eprint(const char *errstr, ...);	/* prints errstr and exits with 1 */
extern char *estrdup(const char *str);			/* duplicates str, exits on allocation error */
extern void spawn(const char *arg);             /* execute arg */
