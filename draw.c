
/* 
 * (C)opyright MMVII Robert Manea <rob dot manea at gmail dot com>
 * See LICENSE file for license details.
 *
 */

#include "dzen.h"
#include <stdio.h>
#include <string.h>

void parse_line(const char *, int, int, int);
int get_tokval(const char*, char**);
int get_token(const char *, int *, char **);

enum ctype {bg, fg, icon};

static unsigned int
textnw(const char *text, unsigned int len) {
	XRectangle r;

	if(dzen.font.set) {
		XmbTextExtents(dzen.font.set, text, len, NULL, &r);
		return r.width;
	}
	return XTextWidth(dzen.font.xfont, text, len);
}


void
drawtext(const char *text, int reverse, int line, int align) {
	XRectangle r = { dzen.x, dzen.y, dzen.w, dzen.h};


	if(!reverse) {
		XSetForeground(dzen.dpy, dzen.gc, dzen.norm[ColBG]);
		XFillRectangles(dzen.dpy, dzen.slave_win.drawable[line], dzen.gc, &r, 1);
		XSetForeground(dzen.dpy, dzen.gc, dzen.norm[ColFG]);
	}
	else {
		XSetForeground(dzen.dpy, dzen.rgc, dzen.norm[ColFG]);
		XFillRectangles(dzen.dpy, dzen.slave_win.drawable[line], dzen.rgc, &r, 1);
		XSetForeground(dzen.dpy, dzen.rgc, dzen.norm[ColBG]);
	}

	if(dzen.font.set) 
		parse_line(text, line, align, reverse);
	else 
		parse_line(text, line, align, reverse);
}

long
getcolor(const char *colstr) {
	Colormap cmap = DefaultColormap(dzen.dpy, dzen.screen);
	XColor color;

	if(!XAllocNamedColor(dzen.dpy, cmap, colstr, &color, &color))
		return -1;

	return color.pixel;
}

void
setfont(const char *fontstr) {
	char *def, **missing;
	int i, n;

	missing = NULL;
	if(dzen.font.set)
		XFreeFontSet(dzen.dpy, dzen.font.set);
	dzen.font.set = XCreateFontSet(dzen.dpy, fontstr, &missing, &n, &def);
	if(missing)
		XFreeStringList(missing);
	if(dzen.font.set) {
		XFontSetExtents *font_extents;
		XFontStruct **xfonts;
		char **font_names;
		dzen.font.ascent = dzen.font.descent = 0;
		font_extents = XExtentsOfFontSet(dzen.font.set);
		n = XFontsOfFontSet(dzen.font.set, &xfonts, &font_names);
		for(i = 0, dzen.font.ascent = 0, dzen.font.descent = 0; i < n; i++) {
			if(dzen.font.ascent < (*xfonts)->ascent)
				dzen.font.ascent = (*xfonts)->ascent;
			if(dzen.font.descent < (*xfonts)->descent)
				dzen.font.descent = (*xfonts)->descent;
			xfonts++;
		}
	}
	else {
		if(dzen.font.xfont)
			XFreeFont(dzen.dpy, dzen.font.xfont);
		dzen.font.xfont = NULL;
		if(!(dzen.font.xfont = XLoadQueryFont(dzen.dpy, fontstr)))
			eprint("dzen: error, cannot load font: '%s'\n", fontstr);
		dzen.font.ascent = dzen.font.xfont->ascent;
		dzen.font.descent = dzen.font.xfont->descent;
	}
	dzen.font.height = dzen.font.ascent + dzen.font.descent;
}

unsigned int
textw(const char *text) {
	return textnw(text, strlen(text)) + dzen.font.height;
}

int
get_tokval(const char* line, char **retdata) {
	int i;
	char tokval[128];

	for(i=0; i < 128 && (*(line+i) != ')'); i++)
		tokval[i] = *(line+i);

	tokval[i] = '\0';
	*retdata = strdup(tokval);

	return i+1;
}

int
get_token(const char *line, int * t, char **tval) {
	int off=0, next_pos=0;
	char *tokval = NULL;

	if(*(line+1) == '^')
		return 0;
	
	line++;
	/* ^bg(#rrggbb) */
	if( (*line == 'b') && (*(line+1) == 'g') && (*(line+2) == '(')) {
		off = 3;
		next_pos = get_tokval(line+off, &tokval);
		*t = bg;
	}
	/* ^fg(#rrggbb) */
	if((*line == 'f') && (*(line+1) == 'g') && (*(line+2) == '(')) {
		off=3;
		next_pos = get_tokval(line+off, &tokval);
		*t = fg;
	}
	/* ^i(iconname) */
	if((*line == 'i') && (*(line+1) == '(')) {
		off = 2;
		next_pos = get_tokval(line+off, &tokval);
		*t = icon;
	}

	*tval = tokval;
	return next_pos+off;
}

void
parse_line(const char *line, int lnr, int align, int reverse) {
	int i, next_pos=0, j=0, px=0, py=0, xorig, h, tw, ow;
	char lbuf[MAX_LINE_LEN];
	int t=-1;
	char *tval=NULL;
	Drawable pm;
	XRectangle r = { dzen.x, dzen.y, dzen.w, dzen.h};


	h = dzen.font.ascent + dzen.font.descent;
	xorig = 0; py = dzen.font.ascent + (dzen.line_height - h) / 2;;

	if(lnr != -1) {
	pm = XCreatePixmap(dzen.dpy, RootWindow(dzen.dpy, DefaultScreen(dzen.dpy)), dzen.slave_win.width, 
			dzen.line_height, DefaultDepth(dzen.dpy, dzen.screen));
	}
	else {
	pm = XCreatePixmap(dzen.dpy, RootWindow(dzen.dpy, DefaultScreen(dzen.dpy)), dzen.title_win.width, 
			dzen.line_height, DefaultDepth(dzen.dpy, dzen.screen));
	}

	if(!reverse) {
		XSetBackground(dzen.dpy, dzen.tgc, dzen.norm[ColBG]);
		XSetForeground(dzen.dpy, dzen.tgc, dzen.norm[ColBG]);
	}
	else {
		XSetBackground(dzen.dpy, dzen.tgc, dzen.norm[ColFG]);
		XSetForeground(dzen.dpy, dzen.tgc, dzen.norm[ColFG]);
	}
	XFillRectangles(dzen.dpy, pm, dzen.tgc, &r, 1);

	if(!reverse) {
		XSetForeground(dzen.dpy, dzen.tgc, dzen.norm[ColFG]);
	}
	else {
		XSetForeground(dzen.dpy, dzen.tgc, dzen.norm[ColBG]);
	}

	if( (lnr + dzen.slave_win.first_line_vis) >= dzen.slave_win.tcnt) {
			XmbDrawImageString(dzen.dpy, pm, dzen.font.set,
					dzen.tgc, px, py, "", 0);
			XCopyArea(dzen.dpy, pm, dzen.slave_win.drawable[lnr], dzen.gc,
					0, 0, px, dzen.line_height, xorig, 0);
			XFreePixmap(dzen.dpy, pm);
			return;
	}


	for(i=0; i < strlen(line); i++) {
		if(*(line+i) == '^') {
			lbuf[j] = '\0';
			if(t != -1 && tval) {
				switch(t) {
					case fg:
						reverse ?
							XSetBackground(dzen.dpy, dzen.tgc, getcolor(tval)) :
							XSetForeground(dzen.dpy, dzen.tgc, getcolor(tval));
						break;
					case bg:
						reverse ? 
							XSetForeground(dzen.dpy, dzen.tgc, getcolor(tval)) :
							XSetBackground(dzen.dpy, dzen.tgc, getcolor(tval));
						break;
				}
			}
			ow = j;
			tw = textnw(lbuf, strlen(lbuf));
			while( (tw + px) > (dzen.w - h)) {
				lbuf[--j] = '\0';
				tw = textnw(lbuf, strlen(lbuf));
			}
			if(j < ow) {
				if(j > 1)
					lbuf[j - 1] = '.';
				if(j > 2)
					lbuf[j - 2] = '.';
				if(j > 3)
					lbuf[j - 3] = '.';
			}

				
			XmbDrawImageString(dzen.dpy, pm, dzen.font.set,
					dzen.tgc, px, py, lbuf, tw);
			px += tw;

			j=0; t=-1; tval=NULL;
			/* get values */
			next_pos = get_token(line+i, &t, &tval);
			i += next_pos;

			/* ^^ escapes */
			if(next_pos == 0) {
				lbuf[j++] = line[i];
				i++;
			} 
		} 
		else {
			lbuf[j++] = line[i];
		}
	}

	lbuf[j] = '\0';
	if(t != -1 && tval) {
		switch(t) {
			case fg:
				reverse ?
					XSetBackground(dzen.dpy, dzen.tgc, getcolor(tval)) :
					XSetForeground(dzen.dpy, dzen.tgc, getcolor(tval));
				break;
			case bg:
				reverse ? 
					XSetForeground(dzen.dpy, dzen.tgc, getcolor(tval)) :
					XSetBackground(dzen.dpy, dzen.tgc, getcolor(tval));
				break;
		}
	}
	ow = j;
	tw = textnw(lbuf, strlen(lbuf));
	while( (tw + px) > (dzen.w - h)) {
		lbuf[--j] = '\0';
		tw = textnw(lbuf, strlen(lbuf));
	}
	if(j < ow) {
		if(j > 1)
			lbuf[j - 1] = '.';
		if(j > 2)
			lbuf[j - 2] = '.';
		if(j > 3)
			lbuf[j - 3] = '.';
	}

	XmbDrawImageString(dzen.dpy, pm, dzen.font.set,
			dzen.tgc, px, py, lbuf, tw);
	px += tw;

	if(align == ALIGNLEFT)
		xorig = h/2;
	if(align == ALIGNCENTER) {
		xorig = (lnr != -1) ?
			(dzen.slave_win.width - px)/2 :
			(dzen.title_win.width - px)/2;
	}
	else if(align == ALIGNRIGHT) {
		xorig = (lnr != -1) ?
			(dzen.slave_win.width - px - h/2) :
			(dzen.title_win.width - px - h/2); 
	}

	if(lnr != -1) {
		XCopyArea(dzen.dpy, pm, dzen.slave_win.drawable[lnr], dzen.gc,
				0, 0, px, dzen.line_height, xorig, 0);
	}
	else {
		XCopyArea(dzen.dpy, pm, dzen.title_win.drawable, dzen.gc,
				0, 0, px, dzen.line_height, xorig, 0);
	}
	XFreePixmap(dzen.dpy, pm);
}

void
drawheader(const char * text) {
	dzen.x = 0;
	dzen.y = 0;
	dzen.w = dzen.title_win.width;
	dzen.h = dzen.line_height;
	XRectangle r = { dzen.x, dzen.y, dzen.w, dzen.h};

	if(text) {
		XSetForeground(dzen.dpy, dzen.gc, dzen.norm[ColBG]);
		XFillRectangles(dzen.dpy, dzen.title_win.drawable, dzen.gc, &r, 1);
		XSetForeground(dzen.dpy, dzen.gc, dzen.norm[ColFG]);
		parse_line(text, -1, dzen.title_win.alignment, 0);
	}

	XCopyArea(dzen.dpy, dzen.title_win.drawable, dzen.title_win.win, 
			dzen.gc, 0, 0, dzen.title_win.width, dzen.line_height, 0, 0);
}

void
drawbody(char * text) {
	if(dzen.slave_win.tcnt == dzen.slave_win.tsize) 
		free_buffer();
	if(dzen.slave_win.tcnt < dzen.slave_win.tsize) {
		dzen.slave_win.tbuf[dzen.slave_win.tcnt].text = estrdup(text);
		dzen.slave_win.tcnt++;
	}
}
