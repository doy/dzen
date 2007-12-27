/*  
	gdbar - graphical percentage meter, to be used with dzen

	Copyright (c) 2007 by Robert Manea  <rob dot manea at gmail dot com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<X11/Xlib.h>

#define MAXLEN 1024

static void pbar (const char *, double, int, int, int, int, int, int, int);

enum style { norm, outlined, vertical };
char *bg, *fg;

static void
pbar(const char* label, double perc, int maxc, int height, int segw, int segh, int segb, int pnl, int mode) {
	int i, rp;
	int segs, segsa;
	double l;


	l = perc * ((mode ? (double)(maxc-2) : (double) maxc) / 100);
	if((int)(l + 0.5) >= (int)l)
		l += 0.5;

	if((int)(perc + 0.5) >= (int)perc)
		rp = (int)(perc + 0.5);
	else
		rp = (int)perc;

	if(mode == outlined)
		printf("%s%3d%% ^ib(1)^fg(%s)^ro(%dx%d)^p(%d)^fg(%s)^r(%dx%d)^p(%d)^ib(0)^fg()%s", 
				label ? label : "", rp, 
				bg, (int)maxc, height, -1*(maxc-1),
				fg, (int)l, height-2,
				maxc-(int)l-1, pnl ? "\n" : "");
	else if(mode == vertical) {
		segs  = height / (segh + segb);
		segsa = rp * segs / 100;

		printf("%s^ib(1)", label ? label : "");
		for(i=0; i < segs; i++) {
			if(i<segsa)
				printf("^fg(%s)^p(-%d)^r(%dx%d+%d-%d')", fg, segw, segw, segh, 0, (segh+segb)*(i+1));
			else
				printf("^fg(%s)^p(-%d)^r(%dx%d+%d-%d')", bg, segw, segw, segh, 0, (segh+segb)*(i+1));

		}
		printf("^ib(0)^fg()%s", pnl ? "\n" : "");
	}
	else
		printf("%s%3d%% ^fg(%s)^r(%dx%d)^fg(%s)^r(%dx%d)^fg()%s", 
				label ? label : "", rp, 
				fg, (int)l, height,
				bg, maxc-(int)l, height,
				pnl ? "\n" : "");

	fflush(stdout);
}

int
main(int argc, char *argv[])
{
	int i, nv;
	double val;
	char aval[MAXLEN], *endptr;

	/* defaults */
	int mode      = norm; 
	int barwidth  =   80;
	int barheight =   10;
	int segw      =    6;
	int segh      =    2;
	int segb      =    1;
	double minval =    0;
	double maxval =  100.0;
	int print_nl  =    1;
	const char *label = NULL;


	for(i=1; i < argc; i++) {
		if(!strncmp(argv[i], "-w", 3)) {
			if(++i < argc)
				barwidth = atoi(argv[i]);
		}
		else if(!strncmp(argv[i], "-h", 3)) {
			if(++i < argc)
				barheight = atoi(argv[i]);
		}
		else if(!strncmp(argv[i], "-sw", 4)) {
			if(++i < argc)
				segw = atoi(argv[i]);
		}
		else if(!strncmp(argv[i], "-sh", 4)) {
			if(++i < argc)
				segh = atoi(argv[i]);
		}
		else if(!strncmp(argv[i], "-ss", 4)) {
			if(++i < argc)
				segb = atoi(argv[i]);
		}
		else if(!strncmp(argv[i], "-s", 3)) {
			if(++i < argc) {
				switch(argv[i][0]) {
					case 'o':
						mode = outlined;
						break;
					case 'v':
						mode = vertical;
						break;
					default:
						mode = norm;
						break;
				}
			}
		}

		else if(!strncmp(argv[i], "-fg", 4)) {
			if(++i < argc)
				fg = argv[i];
			else {
				fg = malloc(6);
				strcpy(fg, "white");
			}
		}
		else if(!strncmp(argv[i], "-bg", 4)) {
			if(++i < argc)
				bg = argv[i];
			else {
				bg = malloc(9);
				strcpy(bg, "darkgrey");
			}
		}
		else if(!strncmp(argv[i], "-max", 5)) {
			if(++i < argc) {
				maxval = strtod(argv[i], &endptr);
				if(*endptr) {
					fprintf(stderr, "dbar: '%s' incorrect number format", argv[i]);
					return EXIT_FAILURE;
				}
			}
		}
		else if(!strncmp(argv[i], "-min", 5)) {
			if(++i < argc) {
				minval = strtod(argv[i], &endptr);
				if(*endptr) {
					fprintf(stderr, "dbar: '%s' incorrect number format", argv[i]);
					return EXIT_FAILURE;
				}
			}
		}
		else if(!strncmp(argv[i], "-l", 3)) {
			if(++i < argc)
				label = argv[i];
		}
		else if(!strncmp(argv[i], "-nonl", 6)) {
			print_nl = 0;
		}
		else {
			fprintf(stderr, "usage: dbar [-s <o|v>] [-w <pixel>] [-h <pixel>] [-sw <pixel>] [-ss <pixel>] [-sw <pixel>] [-fg <color>] [-bg <color>] [-min <minvalue>] [-max <maxvalue>] [-l <string>] [-nonl] \n");
			return EXIT_FAILURE;
		}
	}

	if(!fg) {
		fg = malloc(6);
		strcpy(fg, "white");
	}
	if(!bg) {
		bg = malloc(9);
		strcpy(bg, "darkgrey");
	}

	while(fgets(aval, MAXLEN, stdin)) {
		nv = sscanf(aval, "%lf %lf %lf", &val, &minval, &maxval);
		if(nv == 2) {
			maxval = minval;
			minval = 0;
		}

		pbar(label, (100*(val-minval))/(maxval-minval), 
				barwidth, barheight, segw, segh, segb,
				print_nl, mode);
	}

	return EXIT_SUCCESS;
}

