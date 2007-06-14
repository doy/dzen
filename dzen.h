/* 
 * (C)opyright MMVII Robert Manea <rob dot manea at gmail dot com>
 * See LICENSE file for license details.
 *
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef DZEN_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#define FONT		"-*-fixed-*-*-*-*-*-*-*-*-*-*-*-*"
#define BGCOLOR		"#ab0b0b"
#define FGCOLOR		"#efefef"

#define ALIGNCENTER 0
#define ALIGNLEFT   1
#define ALIGNRIGHT  2

#define MIN_BUF_SIZE   512
#define MAX_LINE_LEN   512

/* gui data structures */
enum { ColFG, ColBG, ColLast };

typedef struct DZEN Dzen;
typedef struct Fnt Fnt;
typedef struct TW TWIN;
typedef struct SW SWIN;
typedef struct _Sline Sline;

struct Fnt {
	XFontStruct *xfont;
	XFontSet set;
	int ascent;
	int descent;
	int height;
};

struct _Sline {
	unsigned long fg;
	unsigned long bg;
	char * text;
};

/* title window */
struct TW {
	int x, y, width, height;

	Window win;
	Drawable drawable;
	char alignment;
	Bool ishidden;
};

/* slave window */
struct SW {
	int x, y, width, height;

	Window win;
	Window *line;
	Drawable *drawable;

	/* input buffer */
	//char **tbuf;
	Sline *tbuf;
	int tsize;
	int tcnt;
	/* line fg colors */
	unsigned long *tcol;

	int max_lines;
	int first_line_vis;
	int last_line_vis;
	int sel_line;

	char alignment;
	Bool ismenu;
	Bool ishmenu;
	Bool issticky;
	Bool ismapped;
};

struct DZEN {
	int x, y, w, h;
	Bool running;
	unsigned long norm[ColLast];

	TWIN title_win;
	SWIN slave_win;

	char *fnt;
	char *bg;
	char *fg;
	int line_height;

	Display *dpy;
	int screen;
	unsigned int depth;

	Visual *visual;
	GC gc, rgc;
	Fnt font;

	Bool ispersistent;
	Bool tsupdate;
	Bool colorize;
	unsigned long timeout;
	long cur_line;
	int ret_val;

	/* should always be 0 if DZEN_XINERAMA not defined */
	int xinescreen;
};

extern Dzen dzen;

void free_buffer(void);
void x_draw_body(void);

/* draw.c */
extern void drawtext(const char *text,
		int reverse,
		int line,
		int aligne);	                        
extern long getcolor(const char *colstr);	/* returns color of colstr */
extern void setfont(const char *fontstr);		    /* sets global font */
extern unsigned int textw(const char *text);		/* returns width of text in px */
extern void drawheader(char *text);
extern void drawbody(char *text);

/* util.c */
extern void *emalloc(unsigned int size);		/* allocates memory, exits on error */
extern void eprint(const char *errstr, ...);	/* prints errstr and exits with 1 */
extern char *estrdup(const char *str);			/* duplicates str, exits on allocation error */
extern void spawn(const char *arg);             /* execute arg */
