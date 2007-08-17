
/* 
* (C)opyright MMVII Robert Manea <rob dot manea at gmail dot com>
* See LICENSE file for license details.
*
*/

#include "dzen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DZEN_XPM
#include <X11/xpm.h>
#endif

#define ARGLEN 256

/* command types for the in-text parser */
enum ctype {bg, fg, icon, rect, recto, circle, circleo, pos, abspos, titlewin, ibg};

int get_tokval(const char* line, char **retdata);
int get_token(const char*  line, int * t, char **tval);

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
	if(!reverse) {
		XSetForeground(dzen.dpy, dzen.gc, dzen.norm[ColBG]);
		XFillRectangle(dzen.dpy, dzen.slave_win.drawable[line], dzen.gc, 0, 0, dzen.w, dzen.h);
		XSetForeground(dzen.dpy, dzen.gc, dzen.norm[ColFG]);
	}
	else {
		XSetForeground(dzen.dpy, dzen.rgc, dzen.norm[ColFG]);
		XFillRectangle(dzen.dpy, dzen.slave_win.drawable[line], dzen.rgc, 0, 0, dzen.w, dzen.h);
		XSetForeground(dzen.dpy, dzen.rgc, dzen.norm[ColBG]);
	}

	parse_line(text, line, align, reverse, 0);
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
	char tokval[ARGLEN];

	for(i=0; i < ARGLEN && (*(line+i) != ')'); i++)
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
	/* ^bg(#rrggbb) background color, type: bg */
	if( (*line == 'b') && (*(line+1) == 'g') && (*(line+2) == '(')) {
		off = 3;
		next_pos = get_tokval(line+off, &tokval);
		*t = bg;
	}
	/* ^ib(bool) ignore background color, type: ibg */
	else if((*line == 'i') && (*(line+1) == 'b') && (*(line+2) == '(')) {
		off=3;
		next_pos = get_tokval(line+off, &tokval);
		*t = ibg;
	}
	/* ^fg(#rrggbb) foreground color, type: fg */
	else if((*line == 'f') && (*(line+1) == 'g') && (*(line+2) == '(')) {
		off=3;
		next_pos = get_tokval(line+off, &tokval);
		*t = fg;
	}
	/* ^tw() draw to title window, type: titlewin */
	else if((*line == 't') && (*(line+1) == 'w') && (*(line+2) == '(')) {
		off=3;
		next_pos = get_tokval(line+off, &tokval);
		*t = titlewin;
	}
	/* ^i(iconname) bitmap icon, type: icon */
	else if((*line == 'i') && (*(line+1) == '(')) {
		off = 2;
		next_pos = get_tokval(line+off, &tokval);
		*t = icon;
	}
	/* ^r(widthxheight) filled rectangle, type: rect */
	else if((*line == 'r') && (*(line+1) == '(')) {
		off = 2;
		next_pos = get_tokval(line+off, &tokval);
		*t = rect;
	}
	/* ^ro(widthxheight) outlined rectangle, type: recto */
	else if((*line == 'r') && (*(line+1) == 'o') && (*(line+2) == '(')) {
		off = 3;
		next_pos = get_tokval(line+off, &tokval);
		*t = recto;
	}
	/* ^p(pixel) relative position, type: pos */
	else if((*line == 'p') && (*(line+1) == '(')) {
		off = 2;
		next_pos = get_tokval(line+off, &tokval);
		*t = pos;
	}
	/* ^pa(pixel) absolute position, type: pos */
	else if((*line == 'p') && (*(line+1) == 'a') && (*(line+2) == '(')) {
		off = 3;
		next_pos = get_tokval(line+off, &tokval);
		*t = abspos;
	}
	/*^c(radius) filled circle, type: circle */
	else if((*line == 'c') && (*(line+1) == '(')) {
		off = 2;
		next_pos = get_tokval(line+off, &tokval);
		*t = circle;
	}
	/* ^co(radius) outlined circle, type: circleo */
	else if((*line == 'c') && (*(line+1) == 'o') && (*(line+2) == '(')) {
		off = 3;
		next_pos = get_tokval(line+off, &tokval);
		*t = circleo;
	}

	*tval = tokval;
	return next_pos+off;
}

static void
setcolor(Drawable *pm, int x, int width, long tfg, long tbg, int reverse, int nobg) {

	if(nobg)
		return;

	XSetForeground(dzen.dpy, dzen.tgc, reverse ? tfg : tbg);
	XFillRectangle(dzen.dpy, *pm, dzen.tgc, x, 0, width, dzen.line_height);

	XSetForeground(dzen.dpy, dzen.tgc, reverse ? tbg : tfg);
	XSetBackground(dzen.dpy, dzen.tgc, reverse ? tfg : tbg);
}

static void
get_rect_vals(char *s, int *w, int *h) {
	int i;
	char buf[128];

	*w = 0; *h = 0;

	for(i=0; (s[i] != 'x') && i<128; i++) {
		buf[i] = s[i];
	}
	buf[++i] = '\0';
	*w = atoi(buf);
	*h = atoi(s+i);
}

char *
parse_line(const char *line, int lnr, int align, int reverse, int nodraw) {
	unsigned int bm_w, bm_h; 
	int bm_xh, bm_yh;
	int rectw, recth, n_pos, recty;
	int i, next_pos=0, j=0, px=0, py=0, xorig, h=0, tw, ow;
	char lbuf[MAX_LINE_LEN], *rbuf = NULL;
	int t=-1, nobg=0;
	char *tval=NULL;
	long lastfg = dzen.norm[ColFG], lastbg = dzen.norm[ColBG];
	XGCValues gcv;
	Drawable pm=0, bm;
#ifdef DZEN_XPM
	int free_xpm_attrib = 0;
	Pixmap xpm_pm;
	XpmAttributes xpma;
	XpmColorSymbol xpms;
#endif

	/* parse line and return the text without control commands */
	if(nodraw) {
		rbuf = emalloc(MAX_LINE_LEN);
		rbuf[0] = '\0';
		if( (lnr + dzen.slave_win.first_line_vis) >= dzen.slave_win.tcnt) 
			line = NULL;
		else
			line = dzen.slave_win.tbuf[dzen.slave_win.first_line_vis+lnr];

	}
	/* parse line and render text */
	else {
		h = dzen.font.height;
		py = dzen.font.ascent + (dzen.line_height - h) / 2;
		xorig = 0; 


		if(lnr != -1) {
			pm = XCreatePixmap(dzen.dpy, RootWindow(dzen.dpy, DefaultScreen(dzen.dpy)), dzen.slave_win.width, 
					dzen.line_height, DefaultDepth(dzen.dpy, dzen.screen));
		}
		else {
			pm = XCreatePixmap(dzen.dpy, RootWindow(dzen.dpy, DefaultScreen(dzen.dpy)), dzen.title_win.width, 
					dzen.line_height, DefaultDepth(dzen.dpy, dzen.screen));
		}

		if(!reverse) {
			XSetForeground(dzen.dpy, dzen.tgc, dzen.norm[ColBG]);
#ifdef DZEN_XPM
			xpms.pixel = dzen.norm[ColBG];
#endif
		}
		else {
			XSetForeground(dzen.dpy, dzen.tgc, dzen.norm[ColFG]);
#ifdef DZEN_XPM
			xpms.pixel = dzen.norm[ColFG];
#endif
		}
		XFillRectangle(dzen.dpy, pm, dzen.tgc, 0, 0, dzen.w, dzen.h);

		if(!reverse) {
			XSetForeground(dzen.dpy, dzen.tgc, dzen.norm[ColFG]);
		}
		else {
			XSetForeground(dzen.dpy, dzen.tgc, dzen.norm[ColBG]);
		}

#ifdef DZEN_XPM
		xpms.name = NULL;
		xpms.value = (char *)"none";

		xpma.colormap = DefaultColormap(dzen.dpy, dzen.screen);
		xpma.depth = DefaultDepth(dzen.dpy, dzen.screen);
		xpma.visual = DefaultVisual(dzen.dpy, dzen.screen);
		xpma.colorsymbols = &xpms;
		xpma.numsymbols = 1;
		xpma.valuemask = XpmColormap|XpmDepth|XpmVisual|XpmColorSymbols;
#endif

		if(!dzen.font.set){ 
			gcv.font = dzen.font.xfont->fid;
			XChangeGC(dzen.dpy, dzen.tgc, GCFont, &gcv);
		}

		if( lnr != -1 && (lnr + dzen.slave_win.first_line_vis >= dzen.slave_win.tcnt)) {
			XCopyArea(dzen.dpy, pm, dzen.slave_win.drawable[lnr], dzen.gc,
					0, 0, px, dzen.line_height, xorig, 0);
			XFreePixmap(dzen.dpy, pm);
			return NULL;
		}
	}

	for(i=0; (unsigned)i < strlen(line); i++) {
		if(*(line+i) == '^') {
			lbuf[j] = '\0';

			if(nodraw) {
				strcat(rbuf, lbuf);
			}
			else {
				if(t != -1 && tval) {
					switch(t) {
						case icon:
							if(XReadBitmapFile(dzen.dpy, pm, tval, &bm_w, 
										&bm_h, &bm, &bm_xh, &bm_yh) == BitmapSuccess 
									&& (h/2 + px + (signed)bm_w < dzen.w)) {
								setcolor(&pm, px, bm_w, lastfg, lastbg, reverse, nobg);
								XCopyPlane(dzen.dpy, bm, pm, dzen.tgc, 
										0, 0, bm_w, bm_h, px, 
										dzen.line_height >= (signed)bm_h ? (dzen.line_height - bm_h)/2 : 0, 1);
								XFreePixmap(dzen.dpy, bm);
								px += bm_w;
							}
#ifdef DZEN_XPM
							else if(XpmReadFileToPixmap(dzen.dpy, dzen.title_win.win, tval, &xpm_pm, NULL, &xpma) == XpmSuccess) {
								setcolor(&pm, px, xpma.width, lastfg, lastbg, reverse, nobg);
								XCopyArea(dzen.dpy, xpm_pm, pm, dzen.tgc, 
										0, 0, xpma.width, xpma.height, px, 
										dzen.line_height >= (signed)xpma.height ? (dzen.line_height - xpma.height)/2 : 0);
								px += xpma.width;

								XFreePixmap(dzen.dpy, xpm_pm);
								free_xpm_attrib = 1;
							}
#endif
							break;

						case rect:
							get_rect_vals(tval, &rectw, &recth);
							rectw = rectw+px > dzen.w ? dzen.w-px : rectw;
							recth = recth > dzen.line_height ? dzen.line_height : recth;
							recty = (dzen.line_height - recth)/2;
							setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
							XFillRectangle(dzen.dpy, pm, dzen.tgc, px, (int)recty, rectw, recth);

							px += rectw;
							break;

						case recto:
							get_rect_vals(tval, &rectw, &recth);
							rectw = rectw+px > dzen.w ? dzen.w-px : rectw;
							recth = recth > dzen.line_height ? dzen.line_height-2 : recth-1;
							recty = (dzen.line_height - recth)/2;
							setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
							XDrawRectangle(dzen.dpy, pm, dzen.tgc, px, (int)recty, rectw-1, recth);
							px += rectw;
							break;

						case circle:
							rectw = recth = atoi(tval);
							setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
							XFillArc(dzen.dpy, pm, dzen.tgc, px, (dzen.line_height - recth)/2, rectw, recth, 2880, 23040);
							px += rectw;
							break;

						case circleo:
							rectw = recth = atoi(tval);
							setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
							XDrawArc(dzen.dpy, pm, dzen.tgc, px, (dzen.line_height - recth)/2, rectw-1, recth-1, 2880, 23040);
							px += rectw;
							break;

						case pos:
							if((n_pos = atoi(tval)) < 0) {
								n_pos = px + n_pos < 0 ? 0 : n_pos;
								setcolor(&pm, n_pos, px, lastfg, lastbg, reverse, nobg);
							}
							else
								setcolor(&pm, px, n_pos, lastfg, lastbg, reverse, nobg);

							px += n_pos;
							break;

						case abspos:
							if((n_pos = atoi(tval)) < 0)
								n_pos *= -1;
							px = n_pos;
							break;

						case ibg:
							nobg = atoi(tval);
							break;

						case bg:
							lastbg = tval[0] ? (unsigned)getcolor(tval) : dzen.norm[ColBG];
							break;

						case fg:
							lastfg = tval[0] ? (unsigned)getcolor(tval) : dzen.norm[ColFG];
							XSetForeground(dzen.dpy, dzen.tgc, lastfg);
							break;
					}
					free(tval);
				}

				/* check if text is longer than window's width */
				ow = j; tw = textnw(lbuf, strlen(lbuf));
				while( ((tw + px) > (dzen.w - h)) && j>=0) {
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


				setcolor(&pm, px, tw, lastfg, lastbg, reverse, nobg);
				if(dzen.font.set) 
					XmbDrawString(dzen.dpy, pm, dzen.font.set,
							dzen.tgc, px, py, lbuf, strlen(lbuf));
				else 
					XDrawString(dzen.dpy, pm, dzen.tgc, px, py, lbuf, strlen(lbuf)); 
				px += tw;
			}

			j=0; t=-1; tval=NULL;
			next_pos = get_token(line+i, &t, &tval);
			i += next_pos;

			/* ^^ escapes */
			if(next_pos == 0) 
				lbuf[j++] = line[i++];
		} 
		else 
			lbuf[j++] = line[i];
	}

	lbuf[j] = '\0';
	if(nodraw) {
		strcat(rbuf, lbuf);
	}
	else {
		if(t != -1 && tval) {
			switch(t) {
				case icon:
					if(XReadBitmapFile(dzen.dpy, pm, tval, &bm_w, 
								&bm_h, &bm, &bm_xh, &bm_yh) == BitmapSuccess 
							&& (h/2 + px + (signed)bm_w < dzen.w)) {
						setcolor(&pm, px, bm_w, lastfg, lastbg, reverse, nobg);
						XCopyPlane(dzen.dpy, bm, pm, dzen.tgc, 
								0, 0, bm_w, bm_h, px, 
								dzen.line_height >= (signed)bm_h ? (dzen.line_height - bm_h)/2 : 0, 1);
						XFreePixmap(dzen.dpy, bm);
						px += bm_w;
					}
#ifdef DZEN_XPM
					else if(XpmReadFileToPixmap(dzen.dpy, dzen.title_win.win, tval, &xpm_pm, NULL, &xpma) == XpmSuccess) {
						setcolor(&pm, px, xpma.width, lastfg, lastbg, reverse, nobg);
						XCopyArea(dzen.dpy, xpm_pm, pm, dzen.tgc, 
								0, 0, xpma.width, xpma.height, px, 
								dzen.line_height >= (signed)xpma.height ? (dzen.line_height - xpma.height)/2 : 0);
						px += xpma.width;

						XFreePixmap(dzen.dpy, xpm_pm);
						free_xpm_attrib = 1;
					}
#endif
					break;

				case rect:
					get_rect_vals(tval, &rectw, &recth);
					rectw = rectw+px > dzen.w ? dzen.w-px : rectw;
					recth = recth > dzen.line_height ? dzen.line_height : recth;
					recty = (dzen.line_height - recth)/2;
					setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
					XFillRectangle(dzen.dpy, pm, dzen.tgc, px, (int)recty, rectw, recth);

					px += rectw;
					break;

				case recto:
					get_rect_vals(tval, &rectw, &recth);
					rectw = rectw+px > dzen.w ? dzen.w-px : rectw;
					recth = recth > dzen.line_height ? dzen.line_height-2 : recth-1;
					recty = (dzen.line_height - recth)/2;
					setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
					XDrawRectangle(dzen.dpy, pm, dzen.tgc, px, (int)recty, rectw-1, recth);
					px += rectw;
					break;

				case circle:
					rectw = recth = atoi(tval);
					setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
					XFillArc(dzen.dpy, pm, dzen.tgc, px, (dzen.line_height - recth)/2, rectw, recth, 2880, 23040);
					px += rectw;
					break;

				case circleo:
					rectw = recth = atoi(tval);
					setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
					XDrawArc(dzen.dpy, pm, dzen.tgc, px, (dzen.line_height - recth)/2, rectw-1, recth-1, 2880, 23040);
					px += rectw;
					break;

				case pos:
					if((n_pos = atoi(tval)) < 0) {
						n_pos = px + n_pos < 0 ? 0 :  px + n_pos;
						setcolor(&pm, n_pos, px, lastfg, lastbg, reverse, nobg);
					}
					else
						setcolor(&pm, px, n_pos, lastfg, lastbg, reverse, nobg);

					px += n_pos;
					break;

				case abspos:
					if((n_pos = atoi(tval)) < 0)
						n_pos *= -1;
					px = n_pos;
					break;

				case ibg:
					nobg = atoi(tval);
					break;

				case bg:
					lastbg = tval[0] ? (unsigned)getcolor(tval) : dzen.norm[ColBG];
					break;

				case fg:
					lastfg = tval[0] ? (unsigned)getcolor(tval) : dzen.norm[ColFG];
					XSetForeground(dzen.dpy, dzen.tgc, lastfg);
					break;
			}
			free(tval);
		}

		/* check if text is longer than window's width */
		ow = j; tw = textnw(lbuf, strlen(lbuf));
		while( ((tw + px) > (dzen.w - h)) && j>=0) {
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


		setcolor(&pm, px, tw, lastfg, lastbg, reverse, nobg);
		if(dzen.font.set) 
			XmbDrawString(dzen.dpy, pm, dzen.font.set,
					dzen.tgc, px, py, lbuf, strlen(lbuf));
		else 
			XDrawString(dzen.dpy, pm, dzen.tgc, px, py, lbuf, strlen(lbuf)); 
		px += tw;


		if(align == ALIGNLEFT)
			xorig = 0;
		if(align == ALIGNCENTER) {
			xorig = (lnr != -1) ?
				(dzen.slave_win.width - px)/2 :
				(dzen.title_win.width - px)/2;
		}
		else if(align == ALIGNRIGHT) {
			xorig = (lnr != -1) ?
				(dzen.slave_win.width - px) :
				(dzen.title_win.width - px); 
		}

		if(lnr != -1) {
			XCopyArea(dzen.dpy, pm, dzen.slave_win.drawable[lnr], dzen.gc,
					0, 0, dzen.w, dzen.line_height, xorig, 0);
		}
		else {
			XCopyArea(dzen.dpy, pm, dzen.title_win.drawable, dzen.gc,
					0, 0, dzen.w, dzen.line_height, xorig, 0);
		}
		XFreePixmap(dzen.dpy, pm);

#ifdef DZEN_XPM
		if(free_xpm_attrib) {
			XFreeColors(dzen.dpy, xpma.colormap, xpma.pixels, xpma.npixels, xpma.depth);
			XpmFreeAttributes(&xpma);
		}
#endif
	}

	return nodraw ? rbuf : NULL;
}

void
drawheader(const char * text) {
	if(text){ 
		dzen.w = dzen.title_win.width;
		dzen.h = dzen.line_height;

		XFillRectangle(dzen.dpy, dzen.title_win.drawable, dzen.rgc, 0, 0, dzen.w, dzen.h);
		parse_line(text, -1, dzen.title_win.alignment, 0, 0);
	}

	XCopyArea(dzen.dpy, dzen.title_win.drawable, dzen.title_win.win, 
			dzen.gc, 0, 0, dzen.title_win.width, dzen.line_height, 0, 0);
}

void
drawbody(char * text) {
	char *ec;

	/* draw to title window
	   this routine should be better integrated into
	   the actual parsing process
	   */
	if((ec = strstr(text, "^tw()")) && (*(ec-1) != '^')) {
		dzen.w = dzen.title_win.width;
		dzen.h = dzen.line_height;

		XFillRectangle(dzen.dpy, dzen.title_win.drawable, dzen.rgc, 0, 0, dzen.w, dzen.h);
		parse_line(ec+5, -1, dzen.title_win.alignment, 0, 0);
		XCopyArea(dzen.dpy, dzen.title_win.drawable, dzen.title_win.win, 
				dzen.gc, 0, 0, dzen.w, dzen.h, 0, 0);
		return;
	}

	if(dzen.slave_win.tcnt == dzen.slave_win.tsize) 
		free_buffer();
	if(dzen.slave_win.tcnt < dzen.slave_win.tsize) {
		dzen.slave_win.tbuf[dzen.slave_win.tcnt] = estrdup(text);
		dzen.slave_win.tcnt++;
	}
}
