/*
 * (C)opyright MMVII Robert Manea <rob dot manea at gmail dot com>
 * See LICENSE file for license details.
 *
 */

#include "dzen.h"
#include "action.h"

#include <ctype.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

Dzen dzen = {0};
static int last_cnt = 0;
typedef void sigfunc(int);


static void 
clean_up(void) {
	int i;

	free_event_list();
	if(dzen.font.set)
		XFreeFontSet(dzen.dpy, dzen.font.set);
	else
		XFreeFont(dzen.dpy, dzen.font.xfont);

	XFreePixmap(dzen.dpy, dzen.title_win.drawable);
	if(dzen.slave_win.max_lines) {
		for(i=0; i < dzen.slave_win.max_lines; i++) {
			XFreePixmap(dzen.dpy, dzen.slave_win.drawable[i]);
			XDestroyWindow(dzen.dpy, dzen.slave_win.line[i]);
		}
		free(dzen.slave_win.line);
		XDestroyWindow(dzen.dpy, dzen.slave_win.win);
	}
	XFreeGC(dzen.dpy, dzen.gc);
	XFreeGC(dzen.dpy, dzen.rgc);
	XFreeGC(dzen.dpy, dzen.tgc);
	XDestroyWindow(dzen.dpy, dzen.title_win.win);
	XCloseDisplay(dzen.dpy);
}

static void
catch_sigusr1(int s) {
	(void)s;
	do_action(sigusr1);
}

static void
catch_sigusr2(int s) {
	(void)s;
	do_action(sigusr2);
}

static void
catch_sigterm(int s) {
	(void)s;
	do_action(onexit);
}

static void
catch_alrm(int s) {
	(void)s;
	do_action(onexit);
	clean_up();
	exit(0);
}

static sigfunc *
setup_signal(int signr, sigfunc *shandler) {
	struct sigaction nh, oh;

	nh.sa_handler = shandler;
	sigemptyset(&nh.sa_mask);
	nh.sa_flags = 0;

	if(sigaction(signr, &nh, &oh) < 0)
		return SIG_ERR;

	return NULL;
}

char *rem=NULL;
static int
chomp(char *inbuf, char *outbuf, int start, int len) {
	int i=0;
	int off=start;

	if(rem) {
		strncpy(outbuf, rem, strlen(rem));
		i += strlen(rem);
		free(rem);
		rem = NULL;
	}
	while(off < len) {
		if(i > MAX_LINE_LEN) {
			outbuf[i] = '\0';
			return ++off;
		}
		if(inbuf[off] != '\n') {
			 outbuf[i++] = inbuf[off++];
		} 
		else if(inbuf[off] == '\n') {
			outbuf[i] = '\0';
			return ++off;
		}
	}

	outbuf[i] = '\0';
	rem = estrdup(outbuf);
	return 0;
}

void
free_buffer(void) {
	int i;
	for(i=0; i<dzen.slave_win.tsize; i++) {
		free(dzen.slave_win.tbuf[i]);
		dzen.slave_win.tbuf[i] = NULL;
	}
	dzen.slave_win.tcnt = 
		dzen.slave_win.last_line_vis = 
		last_cnt = 0;
}

static int 
read_stdin(void) {
	char buf[MAX_LINE_LEN], 
		 retbuf[MAX_LINE_LEN];
	ssize_t n, n_off=0;

	if(!(n = read(STDIN_FILENO, buf, sizeof buf))) {
		if(!dzen.ispersistent) {
			dzen.running = False;
			return -1;
		} 
		else 
			return -2;
	} 
	else {
		while((n_off = chomp(buf, retbuf, n_off, n))) {
			if(!dzen.slave_win.ishmenu 
					&& dzen.tsupdate 
					&& dzen.slave_win.max_lines 
					&& ((dzen.cur_line == 0) || !(dzen.cur_line % (dzen.slave_win.max_lines+1))))
				drawheader(retbuf);
			else if(!dzen.slave_win.ishmenu 
					&& !dzen.tsupdate 
					&& ((dzen.cur_line == 0) || !dzen.slave_win.max_lines))
				drawheader(retbuf);
			else 
				drawbody(retbuf);
			dzen.cur_line++;
		}
	}
	return 0;
}

static void
x_hilight_line(int line) {
	drawtext(dzen.slave_win.tbuf[line + dzen.slave_win.first_line_vis], 1, line, dzen.slave_win.alignment);
	XCopyArea(dzen.dpy, dzen.slave_win.drawable[line], dzen.slave_win.line[line], dzen.gc,
			0, 0, dzen.slave_win.width, dzen.line_height, 0, 0);
}

static void
x_unhilight_line(int line) {
	drawtext(dzen.slave_win.tbuf[line + dzen.slave_win.first_line_vis], 0, line, dzen.slave_win.alignment);
	XCopyArea(dzen.dpy, dzen.slave_win.drawable[line], dzen.slave_win.line[line], dzen.rgc,
			0, 0, dzen.slave_win.width, dzen.line_height, 0, 0);
}

void 
x_draw_body(void) {
	int i;
	dzen.x = 0;
	dzen.y = 0;
	dzen.w = dzen.slave_win.width;
	dzen.h = dzen.line_height;

	if(!dzen.slave_win.last_line_vis) {
		if(dzen.slave_win.tcnt < dzen.slave_win.max_lines) {
			dzen.slave_win.first_line_vis = 0;
			dzen.slave_win.last_line_vis  = dzen.slave_win.tcnt;
		}
		else {
			dzen.slave_win.first_line_vis = dzen.slave_win.tcnt - dzen.slave_win.max_lines;
			dzen.slave_win.last_line_vis  = dzen.slave_win.tcnt;
		}
	}

	for(i=0; i < dzen.slave_win.max_lines; i++) {
		if(i < dzen.slave_win.last_line_vis) 
			drawtext(dzen.slave_win.tbuf[i + dzen.slave_win.first_line_vis], 
					0, i, dzen.slave_win.alignment);
	}
	for(i=0; i < dzen.slave_win.max_lines; i++)
		XCopyArea(dzen.dpy, dzen.slave_win.drawable[i], dzen.slave_win.line[i], dzen.gc,
				0, 0, dzen.slave_win.width, dzen.line_height, 0, 0);
}

static void
x_check_geometry(XRectangle si) {
	dzen.title_win.x += si.x;
	dzen.title_win.y += si.y;

	if(dzen.title_win.x > si.x + si.width)
		dzen.title_win.x = si.x;
	if (dzen.title_win.x < si.x)
		dzen.title_win.x = si.x;

	if(!dzen.title_win.width)
		dzen.title_win.width = si.width;

	if((dzen.title_win.x + dzen.title_win.width) > (si.x + si.width))
		dzen.title_win.width = si.width - (dzen.title_win.x - si.x);
	if(!dzen.slave_win.width) {
		dzen.slave_win.x = si.x;
		dzen.slave_win.width = si.width;
	}
	if( dzen.title_win.width == dzen.slave_win.width) {
		dzen.slave_win.x = dzen.title_win.x;
	}
	if(dzen.slave_win.width != si.width) {
		dzen.slave_win.x = dzen.title_win.x + (dzen.title_win.width - dzen.slave_win.width)/2;
		if(dzen.slave_win.x < si.x)
			dzen.slave_win.x = si.x;
		if(dzen.slave_win.x + dzen.slave_win.width > si.x + si.width)
			dzen.slave_win.x = si.x + (si.width - dzen.slave_win.width);
	}
	if(!dzen.line_height)
		dzen.line_height = dzen.font.height + 2;

	if (dzen.title_win.y + dzen.line_height > si.y + si.height)
		dzen.title_win.y = 0;
}

static void
qsi_no_xinerama(Display *dpy, XRectangle *rect) {
	rect->x = 0;
	rect->y = 0;
	rect->width  = DisplayWidth( dpy, DefaultScreen(dpy));
	rect->height = DisplayHeight(dpy, DefaultScreen(dpy));
}

#ifdef DZEN_XINERAMA
static void
queryscreeninfo(Display *dpy, XRectangle *rect, int screen) {
	XineramaScreenInfo *xsi = NULL;
	int nscreens = 1;

	if(XineramaIsActive(dpy))
		xsi = XineramaQueryScreens(dpy, &nscreens);

	if(xsi == NULL || screen > nscreens || screen <= 0) {
		qsi_no_xinerama(dpy, rect);
	} 
	else {
		rect->x      = xsi[screen-1].x_org;
		rect->y      = xsi[screen-1].y_org;
		rect->width  = xsi[screen-1].width;
		rect->height = xsi[screen-1].height;
	}
}
#endif

static void 
set_net_wm_strut_partial_for(Display *dpy, Window w) {
	unsigned long strut[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	XWindowAttributes wa;

	XGetWindowAttributes(dpy, w, &wa);

	if(wa.y == 0) {
		strut[2] = wa.height;
		strut[8] = wa.x;
		strut[9] = wa.x + wa.width - 1;
	} 
	else if((wa.y + wa.height) == DisplayHeight(dpy, DefaultScreen(dpy))) {
		strut[3] = wa.height;
		strut[10] = wa.x;
		strut[11] = wa.x + wa.width - 1;
	}

	if(strut[2] != 0 || strut[3] != 0) {
		XChangeProperty(
			dpy, 
			w, 
			XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False),
			XInternAtom(dpy, "CARDINAL", False),
			32, 
			PropModeReplace,
			(unsigned char *)&strut,
			12
		);
	}
}

static void
x_create_gcs(void) {
	/* normal GC */
	dzen.gc  = XCreateGC(dzen.dpy, RootWindow(dzen.dpy, dzen.screen), 0, 0);
	XSetForeground(dzen.dpy, dzen.gc, dzen.norm[ColFG]);
	XSetBackground(dzen.dpy, dzen.gc, dzen.norm[ColBG]);
	/* reverse color GC */
	dzen.rgc = XCreateGC(dzen.dpy, RootWindow(dzen.dpy, dzen.screen), 0, 0);
	XSetForeground(dzen.dpy, dzen.rgc, dzen.norm[ColBG]);
	XSetBackground(dzen.dpy, dzen.rgc, dzen.norm[ColFG]);
	/* temporary GC */
	dzen.tgc = XCreateGC(dzen.dpy, RootWindow(dzen.dpy, dzen.screen), 0, 0);
}

static void
x_create_windows(void) {
	XSetWindowAttributes wa;
	Window root;
	int i;
	XRectangle si;

	dzen.dpy = XOpenDisplay(0);
	if(!dzen.dpy) 
		eprint("dzen: cannot open display\n");

	dzen.screen = DefaultScreen(dzen.dpy);
	root = RootWindow(dzen.dpy, dzen.screen);

	/* style */
	if((dzen.norm[ColBG] = getcolor(dzen.bg)) == ~0lu)
		eprint("dzen: error, cannot allocate color '%s'\n", dzen.bg);
	if((dzen.norm[ColFG] = getcolor(dzen.fg)) == ~0lu)
		eprint("dzen: error, cannot allocate color '%s'\n", dzen.fg);
	setfont(dzen.fnt);

	x_create_gcs();

	/* window attributes */
	wa.override_redirect = 1;
	wa.background_pixmap = ParentRelative;
	wa.event_mask = ExposureMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | KeyPressMask;

#ifdef DZEN_XINERAMA
	queryscreeninfo(dzen.dpy, &si, dzen.xinescreen);
#else
	qsi_no_xinerama(dzen.dpy, &si);
#endif
	x_check_geometry(si);

	/* title window */
	dzen.title_win.win = XCreateWindow(dzen.dpy, root, 
			dzen.title_win.x, dzen.title_win.y, dzen.title_win.width, dzen.line_height, 0,
			DefaultDepth(dzen.dpy, dzen.screen), CopyFromParent,
			DefaultVisual(dzen.dpy, dzen.screen),
			CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
	XStoreName(dzen.dpy, dzen.title_win.win, "dzen title");

	dzen.title_win.drawable = XCreatePixmap(dzen.dpy, root, dzen.title_win.width, 
			dzen.line_height, DefaultDepth(dzen.dpy, dzen.screen));
	XFillRectangle(dzen.dpy, dzen.title_win.drawable, dzen.rgc, 0, 0, dzen.title_win.width, dzen.line_height); 

	/* set some hints for windowmanagers*/
	set_net_wm_strut_partial_for(dzen.dpy, dzen.title_win.win);

	/* TODO: Smarter approach to window creation so we can reduce the
	 *       size of this function. 
	 */

	if(dzen.slave_win.max_lines) {
		dzen.slave_win.first_line_vis = 0;
		dzen.slave_win.last_line_vis  = 0;
		dzen.slave_win.line     = emalloc(sizeof(Window) * dzen.slave_win.max_lines);
		dzen.slave_win.drawable =  emalloc(sizeof(Drawable) * dzen.slave_win.max_lines);

		/* horizontal menu mode */
		if(dzen.slave_win.ishmenu) {
			/* calculate width of menuentrieѕ, this is a very simple
			 * approach but works well for general cases.
			 */
			int ew = dzen.slave_win.width / dzen.slave_win.max_lines;
			int r = dzen.slave_win.width - ew * dzen.slave_win.max_lines;
			dzen.slave_win.issticky = True;
			dzen.slave_win.y = dzen.title_win.y;

			dzen.slave_win.win = XCreateWindow(dzen.dpy, root, 
					dzen.slave_win.x, dzen.slave_win.y, dzen.slave_win.width, dzen.line_height, 0,
					DefaultDepth(dzen.dpy, dzen.screen), CopyFromParent,
					DefaultVisual(dzen.dpy, dzen.screen),
					CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
			XStoreName(dzen.dpy, dzen.slave_win.win, "dzen slave");

			for(i=0; i < dzen.slave_win.max_lines; i++) {
				dzen.slave_win.drawable[i] = XCreatePixmap(dzen.dpy, root, ew+r, 
						dzen.line_height, DefaultDepth(dzen.dpy, dzen.screen));
			XFillRectangle(dzen.dpy, dzen.slave_win.drawable[i], dzen.rgc, 0, 0, 
					ew+r, dzen.line_height); 
			}

			/* windows holding the lines */
			for(i=0; i < dzen.slave_win.max_lines; i++)
				dzen.slave_win.line[i] = XCreateWindow(dzen.dpy, dzen.slave_win.win, 
						i*ew, 0, (i == dzen.slave_win.max_lines-1) ? ew+r : ew, dzen.line_height, 0,
						DefaultDepth(dzen.dpy, dzen.screen), CopyFromParent,
						DefaultVisual(dzen.dpy, dzen.screen),
						CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);

			/* As we don't use the title window in this mode,
			 * we reuse it's width value 
			 */
			dzen.title_win.width = dzen.slave_win.width;
			dzen.slave_win.width = ew+r;
		}

		/* vertical slave window */
		else {
			dzen.slave_win.issticky = False;
			dzen.slave_win.y = dzen.title_win.y + dzen.line_height;

			if(dzen.title_win.y + dzen.line_height*dzen.slave_win.max_lines > si.y + si.height)
				dzen.slave_win.y = (dzen.title_win.y - dzen.line_height) - dzen.line_height*(dzen.slave_win.max_lines) + dzen.line_height;

			dzen.slave_win.win = XCreateWindow(dzen.dpy, root, 
					dzen.slave_win.x, dzen.slave_win.y, dzen.slave_win.width, dzen.slave_win.max_lines * dzen.line_height, 0,
					DefaultDepth(dzen.dpy, dzen.screen), CopyFromParent,
					DefaultVisual(dzen.dpy, dzen.screen),
					CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
			XStoreName(dzen.dpy, dzen.slave_win.win, "dzen slave");

			for(i=0; i < dzen.slave_win.max_lines; i++) {
				dzen.slave_win.drawable[i] = XCreatePixmap(dzen.dpy, root, dzen.slave_win.width, 
						dzen.line_height, DefaultDepth(dzen.dpy, dzen.screen));
				XFillRectangle(dzen.dpy, dzen.slave_win.drawable[i], dzen.rgc, 0, 0, 
						dzen.slave_win.width, dzen.line_height); 
			}

			/* windows holding the lines */
			for(i=0; i < dzen.slave_win.max_lines; i++) 
				dzen.slave_win.line[i] = XCreateWindow(dzen.dpy, dzen.slave_win.win, 
						0, i*dzen.line_height, dzen.slave_win.width, dzen.line_height, 0,
						DefaultDepth(dzen.dpy, dzen.screen), CopyFromParent,
						DefaultVisual(dzen.dpy, dzen.screen),
						CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
		}
	}

}

static void 
x_map_window(Window win) {
	XMapRaised(dzen.dpy, win);
	XSync(dzen.dpy, False);
}

static void
x_redraw(XEvent e) {
	int i;

	if(!dzen.slave_win.ishmenu
			&& e.xexpose.window == dzen.title_win.win) 
		drawheader(NULL);
	if(!dzen.tsupdate && e.xexpose.window == dzen.slave_win.win) {
		for(i=0; i < dzen.slave_win.max_lines; i++)
			XCopyArea(dzen.dpy, dzen.slave_win.drawable[i], dzen.slave_win.line[i], dzen.gc,
					0, 0, dzen.slave_win.width, dzen.line_height, 0, 0);
	}
	else {
		for(i=0; i < dzen.slave_win.max_lines; i++) 
			if(e.xcrossing.window == dzen.slave_win.line[i]) {
				XCopyArea(dzen.dpy, dzen.slave_win.drawable[i], dzen.slave_win.line[i], dzen.gc,
						0, 0, dzen.slave_win.width, dzen.line_height, 0, 0);
			}
	}
}

static void
handle_xev(void) {
	XEvent ev;
	int i;
	char buf[32];
	KeySym ksym;

	XNextEvent(dzen.dpy, &ev);
	switch(ev.type) {
		case Expose:
			if(ev.xexpose.count == 0) 
				x_redraw(ev);
			break;
		case EnterNotify:
			if(dzen.slave_win.ismenu) { 
				for(i=0; i < dzen.slave_win.max_lines; i++) 
					if(ev.xcrossing.window == dzen.slave_win.line[i])
						x_hilight_line(i);
			}
			if(!dzen.slave_win.ishmenu 
					&& ev.xcrossing.window == dzen.title_win.win)
				do_action(entertitle);
			if(ev.xcrossing.window == dzen.slave_win.win)
				do_action(enterslave);
			break;
		case LeaveNotify:
			if(dzen.slave_win.ismenu) {
				for(i=0; i < dzen.slave_win.max_lines; i++)
					if(ev.xcrossing.window == dzen.slave_win.line[i])
						x_unhilight_line(i);
			}
			if(!dzen.slave_win.ishmenu 
					&& ev.xcrossing.window == dzen.title_win.win)
				do_action(leavetitle);
			if(ev.xcrossing.window == dzen.slave_win.win) {
				do_action(leaveslave);
			}
			break;
		case ButtonRelease:
			if(dzen.slave_win.ismenu) {
				for(i=0; i < dzen.slave_win.max_lines; i++) 
					if(ev.xbutton.window == dzen.slave_win.line[i]) 
						dzen.slave_win.sel_line = i;
			}
			switch(ev.xbutton.button) {
				case Button1:
					do_action(button1);
					break;
				case Button2:
					do_action(button2);
					break;
				case Button3:
					do_action(button3);
					break;
				case Button4:
					do_action(button4);
					break;
				case Button5:
					do_action(button5);
					break;
			}
			break;
		case KeyPress:
			XLookupString(&ev.xkey, buf, sizeof buf, &ksym, 0);
			do_action(ksym+keymarker);
			break;
		
		/* TODO: XRandR rotation and size chnages */
#if 0

#ifdef DZEN_XRANDR 
		case RRScreenChangeNotify:
			handle_xrandr();
#endif

#endif
		
	}
}

static void
handle_newl(void) {
	XWindowAttributes wa;


	if(dzen.slave_win.max_lines && (dzen.slave_win.tcnt > last_cnt)) {
		do_action(onnewinput);

		if (XGetWindowAttributes(dzen.dpy, dzen.slave_win.win, &wa),
				wa.map_state != IsUnmapped
				/* autoscroll and redraw only if  we're 
				 * currently viewing the last line of input 
				 */
				&& (dzen.slave_win.last_line_vis == last_cnt)) {
			dzen.slave_win.first_line_vis = 0;
			dzen.slave_win.last_line_vis = 0;
			x_draw_body();
		} 
		/* needed for a_scrollhome */
		else if(wa.map_state != IsUnmapped 
				&& dzen.slave_win.last_line_vis == dzen.slave_win.max_lines)
			x_draw_body();
		/* forget state if window was unmapped */
		else if(wa.map_state == IsUnmapped || !dzen.slave_win.last_line_vis) {
			dzen.slave_win.first_line_vis = 0;
			dzen.slave_win.last_line_vis = 0;
			x_draw_body();
		}
		last_cnt = dzen.slave_win.tcnt;
	}
}

static void
event_loop(void) {
	int xfd, ret, dr=0;
	fd_set rmask;

	xfd = ConnectionNumber(dzen.dpy);
	while(dzen.running) {
		FD_ZERO(&rmask);
		FD_SET(xfd, &rmask);
		if(dr != -2)
			FD_SET(STDIN_FILENO, &rmask);

		while(XPending(dzen.dpy))
			handle_xev();

		ret = select(xfd+1, &rmask, NULL, NULL, NULL);
		if(ret) {
			if(dr != -2 && FD_ISSET(STDIN_FILENO, &rmask)) {
				if((dr = read_stdin()) == -1)
					return;
				handle_newl();
			}
			if(dr == -2 && dzen.timeout > 0) {
				/* set an alarm to kill us after the timeout */
				struct itimerval t;
				memset(&t, 0, sizeof t);
				t.it_value.tv_sec = dzen.timeout;
				t.it_value.tv_usec = 0;
				setitimer(ITIMER_REAL, &t, NULL);
			}
			if(FD_ISSET(xfd, &rmask))
				handle_xev();
		}
	}
	return; 
}


static void
set_alignment(void) {
	if(dzen.title_win.alignment) 
		switch(dzen.title_win.alignment) {
			case 'l': 
				dzen.title_win.alignment = ALIGNLEFT;
				break;
			case 'c':
				dzen.title_win.alignment = ALIGNCENTER;
				break;
			case 'r':
				dzen.title_win.alignment = ALIGNRIGHT;
				break;
			default:
				dzen.title_win.alignment = ALIGNCENTER;
		}
	if(dzen.slave_win.alignment)
		switch(dzen.slave_win.alignment) {
			case 'l': 
				dzen.slave_win.alignment = ALIGNLEFT;
				break;
			case 'c':
				dzen.slave_win.alignment = ALIGNCENTER;
				break;
			case 'r':
				dzen.slave_win.alignment = ALIGNRIGHT;
				break;
			default:
				dzen.slave_win.alignment = ALIGNLEFT;
		}
}

static void
init_input_buffer(void) {
	if(MIN_BUF_SIZE % dzen.slave_win.max_lines)
		dzen.slave_win.tsize = MIN_BUF_SIZE + (dzen.slave_win.max_lines - (MIN_BUF_SIZE % dzen.slave_win.max_lines));
	else
		dzen.slave_win.tsize = MIN_BUF_SIZE;

	dzen.slave_win.tbuf = emalloc(dzen.slave_win.tsize * sizeof(char *));
}

int
main(int argc, char *argv[]) {
	int i;
	char *action_string = NULL;
	char *endptr;

	/* default values */
	dzen.cur_line  = 0;
	dzen.ret_val   = 0;
	dzen.title_win.x = dzen.slave_win.x = 0;
	dzen.title_win.y = 0;
	dzen.title_win.width = dzen.slave_win.width = 0;
	dzen.title_win.alignment = ALIGNCENTER;
	dzen.slave_win.alignment = ALIGNLEFT;
	dzen.fnt = FONT;
	dzen.bg  = BGCOLOR;
	dzen.fg  = FGCOLOR;
	dzen.slave_win.max_lines  = 0;
	dzen.running = True;
	dzen.xinescreen = 0;
	dzen.tsupdate = 0;
	dzen.line_height = 0;

	/* cmdline args */
	for(i = 1; i < argc; i++)
		if(!strncmp(argv[i], "-l", 3)){
			if(++i < argc) {
				dzen.slave_win.max_lines = atoi(argv[i]);
				init_input_buffer();
			}
		}
		else if(!strncmp(argv[i], "-u", 3)){
			dzen.tsupdate = True;
		}
		else if(!strncmp(argv[i], "-p", 3)) {
			dzen.ispersistent = True;
			if (i+1 < argc) {
				dzen.timeout = strtoul(argv[i+1], &endptr, 10);
				if(*endptr)
					dzen.timeout = 0;
				else
					i++;
			}
		}
		else if(!strncmp(argv[i], "-ta", 4)) {
			if(++i < argc) dzen.title_win.alignment = argv[i][0];
		}
		else if(!strncmp(argv[i], "-sa", 4)) {
			if(++i < argc) dzen.slave_win.alignment = argv[i][0];
		}
		else if(!strncmp(argv[i], "-m", 3)) {
			dzen.slave_win.ismenu = True;
			if(i+1 < argc) 
				dzen.slave_win.ishmenu = (argv[i+1][0] == 'h') ? ++i, True : False;
		}
		else if(!strncmp(argv[i], "-fn", 4)) {
			if(++i < argc) dzen.fnt = argv[i];
		}
		else if(!strncmp(argv[i], "-e", 3)) {
			if(++i < argc) action_string = argv[i];
		}
		else if(!strncmp(argv[i], "-bg", 4)) {
			if(++i < argc) dzen.bg = argv[i];
		}
		else if(!strncmp(argv[i], "-fg", 4)) {
			if(++i < argc) dzen.fg = argv[i];
		}
		else if(!strncmp(argv[i], "-x", 3)) {
			if(++i < argc) dzen.title_win.x = dzen.slave_win.x = atoi(argv[i]);
		}
		else if(!strncmp(argv[i], "-y", 3)) {
			if(++i < argc) dzen.title_win.y = atoi(argv[i]);
		}
		else if(!strncmp(argv[i], "-w", 3)) {
			if(++i < argc) dzen.slave_win.width = atoi(argv[i]);
		}
		else if(!strncmp(argv[i], "-h", 3)) {
			if(++i < argc) dzen.line_height= atoi(argv[i]);
		}
		else if(!strncmp(argv[i], "-tw", 4)) {
			if(++i < argc) dzen.title_win.width = atoi(argv[i]);
		}
#ifdef DZEN_XINERAMA
		else if(!strncmp(argv[i], "-xs", 4)) {
			if(++i < argc) dzen.xinescreen = atoi(argv[i]);
		}
#endif
		else if(!strncmp(argv[i], "-v", 3)) 
			eprint("dzen-"VERSION", (C)opyright 2007 Robert Manea\n");
		else
			eprint("usage: dzen2 [-v] [-p [seconds]] [-m [v|h]] [-ta <l|c|r>] [-sa <l|c|r>]\n"
                   "             [-x <pixel>] [-y <pixel>] [-w <pixel>] [-tw <pixel>] [-u] \n"
				   "             [-e <string>] [-l <lines>]  [-fn <font>] [-bg <color>] [-fg <color>]\n"
#ifdef DZEN_XINERAMA
				   "             [-xs <screen>]\n"
#endif
				  );

	if(dzen.tsupdate && !dzen.slave_win.max_lines)
		dzen.tsupdate = False;

	if(!dzen.title_win.width)
		dzen.title_win.width = dzen.slave_win.width;

	if(!setlocale(LC_ALL, "") || !XSupportsLocale())
		puts("dzen: locale not available, expect problems with fonts.\n");

	if(action_string) 
		fill_ev_table(action_string);
	else {
		if(!dzen.slave_win.max_lines) {
			char edef[] = "button3=exit:13";
			fill_ev_table(edef);
		}
		else if(dzen.slave_win.ishmenu) {
			char edef[] = "enterslave=grabkeys;leaveslave=ungrabkeys;"
				"button4=scrollup;button5=scrolldown;"
				"key_Left=scrollup;key_Right=scrolldown;"
				"button1=menuexec;button3=exit:13;"
				"key_Escape=ungrabkeys,exit";
			fill_ev_table(edef);
		}
		else {
			char edef[]  = "entertitle=uncollapse,grabkeys;"
				"enterslave=grabkeys;leaveslave=collapse,ungrabkeys;"
				"button1=menuexec;button2=togglestick;button3=exit:13;"
				"button4=scrollup;button5=scrolldown;"
				"key_Up=scrollup;key_Down=scrolldown;"
				"key_Escape=ungrabkeys,exit";
			fill_ev_table(edef);
		}
	}

	if((find_event(onexit) != -1) 
			&& (setup_signal(SIGTERM, catch_sigterm) == SIG_ERR))
		fprintf(stderr, "dzen: error hooking SIGTERM\n");

	if((find_event(sigusr1) != -1) 
			&& (setup_signal(SIGUSR1, catch_sigusr1) == SIG_ERR))
		fprintf(stderr, "dzen: error hooking SIGUSR1\n");
	
	if((find_event(sigusr2) != -1) 
		&& (setup_signal(SIGUSR2, catch_sigusr2) == SIG_ERR))
		fprintf(stderr, "dzen: error hooking SIGUSR2\n");
	
	if(setup_signal(SIGALRM, catch_alrm) == SIG_ERR)
		fprintf(stderr, "dzen: error hooking SIGALARM\n");

	set_alignment();
	
	if(dzen.slave_win.ishmenu &&
			!dzen.slave_win.max_lines)
		dzen.slave_win.max_lines = 1;

	x_create_windows();
	
	if(!dzen.slave_win.ishmenu)
		x_map_window(dzen.title_win.win);
	else {
		XMapRaised(dzen.dpy, dzen.slave_win.win);
		for(i=0; i < dzen.slave_win.max_lines; i++)
			XMapWindow(dzen.dpy, dzen.slave_win.line[i]);
	}

	do_action(onstart);

	/* main loop */
	event_loop();

	do_action(onexit);
	clean_up();

	if(dzen.ret_val)
		return dzen.ret_val;

	return EXIT_SUCCESS;
}

