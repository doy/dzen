
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
#define MAX_ICON_CACHE 32

typedef struct ICON_C {
	char name[ARGLEN];
	Pixmap p;
	
	int w, h;
} icon_c;

icon_c icons[MAX_ICON_CACHE];
int icon_cnt;
int otx;

/* command types for the in-text parser */
enum ctype {bg, fg, icon, rect, recto, circle, circleo, pos, abspos, titlewin, ibg, fn, sa};

int get_tokval(const char* line, char **retdata);
int get_token(const char*  line, int * t, char **tval);

static unsigned int
textnw(Fnt *font, const char *text, unsigned int len) {
	XRectangle r;

	if(font->set) {
		XmbTextExtents(font->set, text, len, NULL, &r);
		return r.width;
	}
	return XTextWidth(font->xfont, text, len);
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

	if(*(line+1) == ESC_CHAR)
		return 0;

	line++;
	/* ^bg(#rrggbb) background color, type: bg */

	if((off = 3) && ! strncmp(line, "bg(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = bg;
	}
	/* ^ib(bool) ignore background color, type: ibg */
	else if((off=3) && ! strncmp(line, "ib(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = ibg;
	}
	/* ^fg(#rrggbb) foreground color, type: fg */
	else if((off=3) && ! strncmp(line, "fg(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = fg;
	}
	/* ^tw() draw to title window, type: titlewin */
	else if((off=3) && ! strncmp(line, "tw(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = titlewin;
	}
	/* ^i(iconname) bitmap icon, type: icon */
	else if((off = 2) && ! strncmp(line, "i(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = icon;
	}
	/* ^r(widthxheight) filled rectangle, type: rect */
	else if((off = 2) && ! strncmp(line, "r(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = rect;
	}
	/* ^ro(widthxheight) outlined rectangle, type: recto */
	else if((off = 3) && ! strncmp(line, "ro(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = recto;
	}
	/* ^p(pixel) relative position, type: pos */
	else if((off = 2) && ! strncmp(line, "p(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = pos;
	}
	/* ^pa(pixel) absolute position, type: pos */
	else if((off = 3) && ! strncmp(line, "pa(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = abspos;
	}
	/*^c(radius) filled circle, type: circle */
	else if((off = 2) && ! strncmp(line, "c(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = circle;
	}
	/* ^co(radius) outlined circle, type: circleo */
	else if((off = 3) && ! strncmp(line, "co(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = circleo;
	}
	/* ^fn(fontname) change font, type: fn */
	else if((off = 3) && ! strncmp(line, "fn(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = fn;
	}
	/* ^sa(string) sensitive areas, type: sa */
	else if((off = 3) && ! strncmp(line, "sa(", off)) {
		next_pos = get_tokval(line+off, &tokval);
		*t = sa;
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
get_rect_vals(char *s, int *w, int *h, int *x, int *y) {
	int i, j;
	char buf[ARGLEN];

	*w=*h=*x=*y=0;

	for(i=0; s[i] && s[i] != 'x' && i<ARGLEN; i++) {
		buf[i] = s[i];
	}
	buf[i] = '\0';
	*w = atoi(buf);


	for(j=0, ++i; s[i] && s[i] != '+' && s[i] != '-' && i<ARGLEN; j++, i++)
		buf[j] = s[i];
	buf[j] = '\0';
	*h = atoi(buf);

	if(s[i]) {
		j=0;
		buf[j] = s[i]; ++j;
		for(++i; s[i] && s[i] != '+' && s[i] != '-' && j<ARGLEN; j++, i++) {
			buf[j] = s[i];
		}
		buf[j] = '\0';
		*x = atoi(buf);
		if(s[i])  {
			*y = atoi(s+i);
		}
	}
}

static int
get_circle_vals(char *s, int *d, int *a) {
	int i, ret;
	char buf[128];
	*d=*a=ret=0;

	for(i=0; s[i] && i<128; i++) {
		if(s[i] == '+' || s[i] == '-') {
			ret=1;
			break;
		} else 
			buf[i]=s[i];
	}

	buf[i+1]='\0';
	*d=atoi(buf);
	*a=atoi(s+i);

	return ret;
}

static int
get_pos_vals(char *s, int *d, int *a) {
	int i=0, ret=3, onlyx=1;
	char buf[128];
	*d=*a=0;

	for(i=0; s[i] && i<128; i++) {
		if(s[i] == ';') {
			onlyx=0;
			break;
		} else 
			buf[i]=s[i];
	}

	if(i) {
		buf[i]='\0';
		*d=atoi(buf);
	} else 
		ret=2;

	if(s[++i]) {
		*a=atoi(s+i);
	} else
		ret = 1;

	if(onlyx) ret=1;

	return ret;
}

static int
search_icon_cache(const char* name) {
	int i;

	for(i=0; i < MAX_ICON_CACHE; i++) 
		if(!strncmp(icons[i].name, name, ARGLEN))
			return i;

	return -1;
}

static void
cache_icon(const char* name, Pixmap pm, int w, int h) {
	int i;

	if(icon_cnt >= MAX_ICON_CACHE) 
		icon_cnt = 0;

	if(icons[icon_cnt].p)
		XFreePixmap(dzen.dpy, icons[icon_cnt].p);

	strncpy(icons[icon_cnt].name, name, ARGLEN);
	icons[icon_cnt].w = w;
	icons[icon_cnt].h = h;
	icons[icon_cnt].p = pm;
	icon_cnt++;
}

char *
parse_line(const char *line, int lnr, int align, int reverse, int nodraw) {
	/* bitmaps */
	unsigned int bm_w, bm_h; 
	int bm_xh, bm_yh;
	/* rectangles, cirlcles*/
	int rectw, recth, rectx, recty;
	/* positioning */
	int n_posx, n_posy, set_posy=0;
	int px=0, py=0, xorig;
	int i, next_pos=0, j=0, h=0, tw=0, ow;
	/* fonts */
	int font_was_set=0;

	/* temp buffers */
	char lbuf[MAX_LINE_LEN], *rbuf = NULL;

	/* parser state */
	int t=-1, nobg=0;
	char *tval=NULL;

	/* X stuff */
	long lastfg = dzen.norm[ColFG], lastbg = dzen.norm[ColBG];
	Fnt *cur_fnt = NULL;
	XGCValues gcv;
	Drawable pm=0, bm;
#ifdef DZEN_XPM
	int free_xpm_attrib = 0;
	Pixmap xpm_pm;
	XpmAttributes xpma;
	XpmColorSymbol xpms;
#endif

	/* icon cache */
	int ip;

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
		py = (dzen.line_height - h) / 2;
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
		cur_fnt = &dzen.font;

		if( lnr != -1 && (lnr + dzen.slave_win.first_line_vis >= dzen.slave_win.tcnt)) {
			XCopyArea(dzen.dpy, pm, dzen.slave_win.drawable[lnr], dzen.gc,
					0, 0, px, dzen.line_height, xorig, 0);
			XFreePixmap(dzen.dpy, pm);
			return NULL;
		}
	}


	for(i=0; (unsigned)i < strlen(line); i++) {
		if(*(line+i) == ESC_CHAR) {
			lbuf[j] = '\0';

			if(nodraw) {
				strcat(rbuf, lbuf);
			}
			else {
				if(t != -1 && tval) {
					switch(t) {
						case icon:
							if( (ip=search_icon_cache(tval)) != -1) {
								XCopyArea(dzen.dpy, icons[ip].p, pm, dzen.tgc, 
										0, 0, icons[ip].w, icons[ip].h, px, set_posy ? py :
										(dzen.line_height >= (signed)icons[ip].h ? 
										 (dzen.line_height - icons[ip].h)/2 : 0));
								px += icons[ip].w;
							} else {
								if(XReadBitmapFile(dzen.dpy, pm, tval, &bm_w, 
											&bm_h, &bm, &bm_xh, &bm_yh) == BitmapSuccess 
										&& (h/2 + px + (signed)bm_w < dzen.w)) {
									setcolor(&pm, px, bm_w, lastfg, lastbg, reverse, nobg);

									XCopyPlane(dzen.dpy, bm, pm, dzen.tgc, 
											0, 0, bm_w, bm_h, px, set_posy ? py : 
											(dzen.line_height >= (signed)bm_h ? (dzen.line_height - bm_h)/2 : 0), 1);
									XFreePixmap(dzen.dpy, bm);
									px += bm_w;
								}
#ifdef DZEN_XPM
								else if(XpmReadFileToPixmap(dzen.dpy, dzen.title_win.win, tval, &xpm_pm, NULL, &xpma) == XpmSuccess) {
									setcolor(&pm, px, xpma.width, lastfg, lastbg, reverse, nobg);
									cache_icon(tval, xpm_pm, xpma.width, xpma.height);

									XCopyArea(dzen.dpy, xpm_pm, pm, dzen.tgc, 
											0, 0, xpma.width, xpma.height, px, set_posy ? py :
											(dzen.line_height >= (signed)xpma.height ? (dzen.line_height - xpma.height)/2 : 0));
									px += xpma.width;

									//XFreePixmap(dzen.dpy, xpm_pm);
									free_xpm_attrib = 1;
								}
							}
#endif
							break;

						
						case rect:
							get_rect_vals(tval, &rectw, &recth, &rectx, &recty);
							recth = recth > dzen.line_height ? dzen.line_height : recth;
							if(set_posy)
								py += recty;
							recty =	recty == 0 ? (dzen.line_height - recth)/2 : 
								(dzen.line_height - recth)/2 + recty;
							px += rectx;
							setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
							//printf("R1: setpy=%d px=%d py=%d rectw=%d recth=%d\n", set_posy, px,
							//		set_posy ? py : ((int)recty<0 ? dzen.line_height + recty : recty),
							//		rectw, recth);
							
							XFillRectangle(dzen.dpy, pm, dzen.tgc, px, 
									set_posy ? py : 
									((int)recty < 0 ? dzen.line_height + recty : recty), 
									rectw, recth);

							px += rectw;
							break;
						
						case recto:
							get_rect_vals(tval, &rectw, &recth, &rectx, &recty);
							if (!rectw) break;

							recth = recth > dzen.line_height ? dzen.line_height-2 : recth-1;
							if(set_posy)
								py += recty;
							recty =	recty == 0 ? (dzen.line_height - recth)/2 : 
								(dzen.line_height - recth)/2 + recty;
							px = (rectx == 0) ? px : rectx+px;
							/* prevent from stairs effect when rounding recty */
							if (!((dzen.line_height - recth) % 2)) recty--;
							setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
							XDrawRectangle(dzen.dpy, pm, dzen.tgc, px, 
									set_posy ? py : 
									((int)recty<0 ? dzen.line_height + recty : recty), rectw-1, recth);
							px += rectw;
							break;

						case circle:
							rectx = get_circle_vals(tval, &rectw, &recth);
							setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
							XFillArc(dzen.dpy, pm, dzen.tgc, px, set_posy ? py :(dzen.line_height - rectw)/2, 
									rectw, rectw, 90*64, rectx?recth*64:64*360);
							px += rectw;
							break;

						case circleo:
							rectx = get_circle_vals(tval, &rectw, &recth);
							setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
							XDrawArc(dzen.dpy, pm, dzen.tgc, px, set_posy ? py : (dzen.line_height - rectw)/2, 
									rectw, rectw, 90*64, rectx?recth*64:64*360);
							px += rectw;
							break;

						case pos:
							if(tval[0]) {
								int r=0;
								if( (r=get_pos_vals(tval, &n_posx, &n_posy)) == 1 && !set_posy)
									set_posy=0;
								else
									set_posy=1;

								if(r != 2)
									px = px+n_posx<0? 0 : px + n_posx; 
								if(r != 1)
									py += n_posy;
							} else {
								set_posy = 0;
								py = (dzen.line_height - dzen.font.height) / 2;
							}
							break;

						case abspos:
							if(tval[0]) {
								int r=0;
								if( (r=get_pos_vals(tval, &n_posx, &n_posy)) == 1 && !set_posy)
									set_posy=0;
								else
									set_posy=1;

								n_posx = n_posx < 0 ? n_posx*-1 : n_posx;
								if(r != 2)
									px = n_posx;
								if(r != 1)
									py = n_posy;
							} else {
								set_posy = 0;
								py = (dzen.line_height - dzen.font.height) / 2;
							}
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
						case fn:
							if(tval[0]) {
								if(!strncmp(tval, "dfnt", 4)) {
									cur_fnt = &(dzen.fnpl[atoi(tval+4)]);

									if(!cur_fnt->set) {
										gcv.font = cur_fnt->xfont->fid;
										XChangeGC(dzen.dpy, dzen.tgc, GCFont, &gcv);
									}
								}
								else
									setfont(tval);
							}
							else {
								cur_fnt = &dzen.font;
								if(!cur_fnt->set){ 
									gcv.font = cur_fnt->xfont->fid;
									XChangeGC(dzen.dpy, dzen.tgc, GCFont, &gcv);
								}
							}
							py = set_posy ? py : (dzen.line_height - cur_fnt->height) / 2;
							font_was_set = 1;
							break;
					}
					free(tval);
				}

				/* check if text is longer than window's width */
				ow = j; tw = textnw(cur_fnt, lbuf, strlen(lbuf));
				while( ((tw + px) > (dzen.w - h)) && j>=0) {
					lbuf[--j] = '\0';
					tw = textnw(cur_fnt, lbuf, strlen(lbuf));
				}
				if(j < ow) {
					if(j > 1)
						lbuf[j - 1] = '.';
					if(j > 2)
						lbuf[j - 2] = '.';
					if(j > 3)
						lbuf[j - 3] = '.';
				}


				if(!nobg)
					setcolor(&pm, px, tw, lastfg, lastbg, reverse, nobg);
				if(cur_fnt->set) 
					XmbDrawString(dzen.dpy, pm, cur_fnt->set,
							dzen.tgc, px, py + cur_fnt->ascent, lbuf, strlen(lbuf));
				else 
					XDrawString(dzen.dpy, pm, dzen.tgc, px, py+dzen.font.ascent, lbuf, strlen(lbuf)); 
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
					if( (ip=search_icon_cache(tval)) != -1) {
						XCopyArea(dzen.dpy, icons[ip].p, pm, dzen.tgc, 
								0, 0, icons[ip].w, icons[ip].h, px, set_posy ? py :
								(dzen.line_height >= (signed)icons[ip].h ? 
								 (dzen.line_height - icons[ip].h)/2 : 0));
						px += icons[ip].w;
					} else {
						if(XReadBitmapFile(dzen.dpy, pm, tval, &bm_w, 
									&bm_h, &bm, &bm_xh, &bm_yh) == BitmapSuccess 
								&& (h/2 + px + (signed)bm_w < dzen.w)) {
							setcolor(&pm, px, bm_w, lastfg, lastbg, reverse, nobg);
							XCopyPlane(dzen.dpy, bm, pm, dzen.tgc, 
									0, 0, bm_w, bm_h, px, set_posy ? py : 
									(dzen.line_height >= (signed)bm_h ? (dzen.line_height - bm_h)/2 : 0), 1);
							XFreePixmap(dzen.dpy, bm);
							px += bm_w;
						}
#ifdef DZEN_XPM
						else if(XpmReadFileToPixmap(dzen.dpy, dzen.title_win.win, tval, &xpm_pm, NULL, &xpma) == XpmSuccess) {
							setcolor(&pm, px, xpma.width, lastfg, lastbg, reverse, nobg);
							cache_icon(tval, xpm_pm, xpma.width, xpma.height);

							XCopyArea(dzen.dpy, xpm_pm, pm, dzen.tgc, 
									0, 0, xpma.width, xpma.height, px, set_posy ? py :
									(dzen.line_height >= (signed)xpma.height ? (dzen.line_height - xpma.height)/2 : 0));
							px += xpma.width;

							//XFreePixmap(dzen.dpy, xpm_pm);
							free_xpm_attrib = 1;
						}
					}
#endif
					break;

				case rect:
					get_rect_vals(tval, &rectw, &recth, &rectx, &recty);
					recth = recth > dzen.line_height ? dzen.line_height : recth;
					if(set_posy)
						py += recty;
					recty =	recty == 0 ? (dzen.line_height - recth)/2 : 
						(dzen.line_height - recth)/2 + recty;
					px += rectx;

					setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
					XFillRectangle(dzen.dpy, pm, dzen.tgc, px, 
							set_posy ? py : 
							((int)recty < 0 ? dzen.line_height + recty : recty), 
							rectw, recth);

					px += rectw;
					break;

				case recto:
					get_rect_vals(tval, &rectw, &recth, &rectx, &recty);
					if (!rectw) break;

					recth = recth > dzen.line_height ? dzen.line_height-2 : recth-1;
					if(set_posy)
						py += recty;
					recty =	recty == 0 ? (dzen.line_height - recth)/2 :
								(dzen.line_height - recth)/2 + recty;
					px = (rectx == 0) ? px : rectx+px;
					/* prevent from stairs effect when rounding recty */
					if (!((dzen.line_height - recth) % 2)) recty--;
					setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
					XDrawRectangle(dzen.dpy, pm, dzen.tgc, px, 
							set_posy ? py : 
							((int)recty<0 ? dzen.line_height + recty : recty), rectw-1, recth);
					px += rectw;
					break;

				case circle:
					rectx = get_circle_vals(tval, &rectw, &recth);
					setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
					XFillArc(dzen.dpy, pm, dzen.tgc, px, set_posy ? py :(dzen.line_height - rectw)/2, 
							rectw, rectw, 90*64, rectx?recth*64:64*360);
					px += rectw;
					break;

				case circleo:
					rectx = get_circle_vals(tval, &rectw, &recth);
					setcolor(&pm, px, rectw, lastfg, lastbg, reverse, nobg);
					XDrawArc(dzen.dpy, pm, dzen.tgc, px, set_posy ? py : (dzen.line_height - rectw)/2, 
							rectw, rectw, 90*64, rectx?recth*64:64*360);
					px += rectw;
					break;

				case pos:
					if(tval[0]) {
						int r=0;
						if( (r=get_pos_vals(tval, &n_posx, &n_posy)) == 1 && !set_posy)
							set_posy=0;
						else
							set_posy=1;

						if(r != 2)
							px = px+n_posx<0? 0 : px + n_posx; 
						if(r != 1)
							py += n_posy;
					} else {
						set_posy = 0;
						py = (dzen.line_height - dzen.font.height) / 2;
					}
					break;

				case abspos:
					if(tval[0]) {
						int r=0;
						if( (r=get_pos_vals(tval, &n_posx, &n_posy)) == 1 && !set_posy)
							set_posy=0;
						else
							set_posy=1;

						n_posx = n_posx < 0 ? n_posx*-1 : n_posx;
						if(r != 2)
							px = n_posx;
						if(r != 1)
							py = n_posy;
					} else {
						set_posy = 0;
						py = (dzen.line_height - dzen.font.height) / 2;
					}
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

				case fn:
					if(tval[0]) {
						if(!strncmp(tval, "dfnt", 4)) {
							cur_fnt = &(dzen.fnpl[atoi(tval+4)]);

							if(!cur_fnt->set) {
								gcv.font = cur_fnt->xfont->fid;
								XChangeGC(dzen.dpy, dzen.tgc, GCFont, &gcv);
							}
						}
						else
							setfont(tval);
					}
					else {
						cur_fnt = &dzen.font;
						if(!cur_fnt->set){ 
							gcv.font = cur_fnt->xfont->fid;
							XChangeGC(dzen.dpy, dzen.tgc, GCFont, &gcv);
						}
					}
					py = set_posy ? py : (dzen.line_height - cur_fnt->height) / 2;
					font_was_set = 1;
					break;
			}
			free(tval);
		}

		/* check if text is longer than window's width */
		ow = j; tw = textnw(cur_fnt, lbuf, strlen(lbuf));
		while( ((tw + px) > (dzen.w - h)) && j>=0) {
			lbuf[--j] = '\0';
			tw = textnw(cur_fnt, lbuf, strlen(lbuf));
		}
		if(j < ow) {
			if(j > 1)
				lbuf[j - 1] = '.';
			if(j > 2)
				lbuf[j - 2] = '.';
			if(j > 3)
				lbuf[j - 3] = '.';
		}


		if(!nobg)
			setcolor(&pm, px, tw, lastfg, lastbg, reverse, nobg);
		if(cur_fnt->set) 
			XmbDrawString(dzen.dpy, pm, cur_fnt->set,
					dzen.tgc, px, py + cur_fnt->ascent, lbuf, strlen(lbuf));
		else 
			XDrawString(dzen.dpy, pm, dzen.tgc, px, py+dzen.font.ascent, lbuf, strlen(lbuf)); 
		px += tw;

		/* expand/shrink dynamically */
		if(dzen.title_win.expand && lnr == -1){
			i = px;
			switch(dzen.title_win.expand) {
				case left:
					/* grow left end */
					otx = dzen.title_win.x_right_corner - i > dzen.title_win.x ?
						dzen.title_win.x_right_corner - i : dzen.title_win.x; 
					XMoveResizeWindow(dzen.dpy, dzen.title_win.win, otx, dzen.title_win.y, px, dzen.line_height);
					break;
				case right:
					XResizeWindow(dzen.dpy, dzen.title_win.win, px, dzen.line_height);
					break;
			}

		} else {
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

		/* reset font to default */
		if(font_was_set)
			setfont(dzen.fnt ? dzen.fnt : FONT);

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
	int i;

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
	if(text[0] == '^' && text[1] == 'c' && text[2] == 's') {
		free_buffer();
		for(i=0; i < dzen.slave_win.max_lines; i++)
			XFillRectangle(dzen.dpy, dzen.slave_win.drawable[i], dzen.rgc, 0, 0, dzen.slave_win.width, dzen.line_height);
		x_draw_body();
		return;
	}
	if(dzen.slave_win.tcnt < dzen.slave_win.tsize) {
		dzen.slave_win.tbuf[dzen.slave_win.tcnt] = estrdup(text);
		dzen.slave_win.tcnt++;
	}
}
