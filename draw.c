/* 
 * (C)opyright MMVII Robert Manea <rob dot manea at gmail dot com>
 * See LICENSE file for license details.
 *
 */

#include "dzen.h"
#include <stdio.h>
#include <string.h>

/* static */
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
	int x, y, w, h;
	static char buf[1024];
	unsigned int len, olen;
	XGCValues gcv;
	GC mgc;
	XRectangle r = { dzen.x, dzen.y, dzen.w, dzen.h};


	if(line != -1) {
		if(reverse) {
			if(dzen.slave_win.tbuf[line].text) {
				XSetForeground(dzen.dpy, dzen.rgc, dzen.norm[ColBG]);
				XSetBackground(dzen.dpy, dzen.rgc, dzen.slave_win.tbuf[line].fg);
			} else {
				XSetForeground(dzen.dpy, dzen.rgc, dzen.norm[ColBG]);
				XSetBackground(dzen.dpy, dzen.rgc, dzen.norm[ColFG]);
			}
		} else {
			if(dzen.slave_win.tbuf[line].text) {
				XSetForeground(dzen.dpy, dzen.gc, dzen.slave_win.tbuf[line].fg);
				XSetBackground(dzen.dpy, dzen.gc, dzen.norm[ColBG]);
			} else {
				XSetForeground(dzen.dpy, dzen.gc, dzen.norm[ColFG]);
				XSetBackground(dzen.dpy, dzen.gc, dzen.norm[ColBG]);
			}
		}
	}

	mgc = reverse ? dzen.gc : dzen.rgc;
	if(line == -1)  /* title window */
		XFillRectangles(dzen.dpy, dzen.title_win.drawable, mgc, &r, 1);
	else            /* slave window */
		XFillRectangles(dzen.dpy, dzen.slave_win.drawable[line], mgc, &r, 1);
	

	if(!text)
		return;

	w = 0;
	olen = len = strlen(text);
	if(len >= sizeof buf)
		len = sizeof buf - 1;
	memcpy(buf, text, len);
	buf[len] = 0;
	h = dzen.font.ascent + dzen.font.descent;
	/* shorten text if necessary */
	while(len && (w = textnw(buf, len)) > dzen.w - h)
		buf[--len] = 0;
	if(len < olen) {
		if(len > 1)
			buf[len - 1] = '.';
		if(len > 2)
			buf[len - 2] = '.';
		if(len > 3)
			buf[len - 3] = '.';
	}

	if(line != -1) {
		if(align == ALIGNLEFT)
			x = h/2;
		else if(align == ALIGNCENTER)
			x = (dzen.slave_win.width - textw(buf)+h)/2;
		else
			x = dzen.slave_win.width - textw(buf);
	} else {
		if(!align)
			x = (dzen.w - textw(buf)+h)/2;
		else if(align == ALIGNLEFT)
			x = h/2;
		else
			x = dzen.title_win.width - textw(buf);
	}
	y = dzen.font.ascent + (dzen.line_height - h) / 2;

	mgc = reverse ? dzen.rgc : dzen.gc;
	if(dzen.font.set) {
		if(line == -1) 
	 		XmbDrawString(dzen.dpy, dzen.title_win.drawable, dzen.font.set,
					mgc, x, y, buf, len);
		else 
			XmbDrawString(dzen.dpy, dzen.slave_win.drawable[line], dzen.font.set,
					mgc, x, y, buf, len);
	}
	else {
		gcv.font = dzen.font.xfont->fid;
		XChangeGC(dzen.dpy, mgc, GCForeground | GCFont, &gcv);

		if(line != -1)
			XDrawString(dzen.dpy, dzen.slave_win.drawable[line],
					mgc, x, y, buf, len);
		else
			XDrawString(dzen.dpy, dzen.title_win.drawable, 
					mgc, x, y, buf, len);
	}
}

unsigned long
getcolor(const char *colstr) {
	Colormap cmap = DefaultColormap(dzen.dpy, dzen.screen);
	XColor color;

	if(!XAllocNamedColor(dzen.dpy, cmap, colstr, &color, &color))
		eprint("dzen: error, cannot allocate color '%s'\n", colstr);
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

char *
setlinecolor(char * text, unsigned long * col) {
	char fgcolor[8];
	unsigned long newfg;

	if(text[0] == '^' && text[1] == '#') {
		strncpy(fgcolor, text+1, 7);
		fgcolor[7] = '\0';
		newfg = getcolor(fgcolor);
		*col = newfg;
		return text+8;
	}

	return NULL;
}

void
drawheader(char * text) {
	char *ctext;
	unsigned long col;
	dzen.x = 0;
	dzen.y = 0;
	dzen.w = dzen.title_win.width;
	dzen.h = dzen.line_height;

	if(text) {
		if( (ctext = setlinecolor(text, &col)) ) {
			XSetForeground(dzen.dpy, dzen.gc, col);
			drawtext(ctext, 0, -1, dzen.title_win.alignment);
			XSetForeground(dzen.dpy, dzen.gc, dzen.norm[ColFG]);
		}
		else
			drawtext(text, 0, -1, dzen.title_win.alignment);
	}

	XCopyArea(dzen.dpy, dzen.title_win.drawable, dzen.title_win.win, 
			dzen.gc, 0, 0, dzen.title_win.width, dzen.line_height, 0, 0);
}

void
drawbody(char * text) {
	char *ctext;
	unsigned long col;

	if(dzen.slave_win.tcnt == dzen.slave_win.tsize) 
		free_buffer();
	if(dzen.slave_win.tcnt < dzen.slave_win.tsize) {
		if( (ctext = setlinecolor(text, &col)) ) {
			dzen.slave_win.tbuf[dzen.slave_win.tcnt].fg = col;
			dzen.slave_win.tbuf[dzen.slave_win.tcnt].text = estrdup(ctext);
		} else {
			dzen.slave_win.tbuf[dzen.slave_win.tcnt].fg = dzen.norm[ColFG];
			dzen.slave_win.tbuf[dzen.slave_win.tcnt].text = estrdup(text);
		}
		dzen.slave_win.tcnt++;
	}
}
