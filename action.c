/* 
 * (C)opyright MMVII Robert Manea <rob dot manea  at gmail dot com>
 * See LICENSE file for license details.
 */

#include "dzen.h"
#include "action.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct event_lookup ev_lookup_table[] = {
    { "exposet",        exposetitle},
    { "exposes",        exposeslave},
    { "onstart",        onstart},
    { "onexit",         onexit},
    { "button1",        button1},
    { "button2",        button2},
    { "button3",        button3},
    { "button4",        button4},
    { "button5",        button5},
    { "entertitle",     entertitle},
    { "leavetitle",     leavetitle},
    { "enterslave",     enterslave},
    { "leaveslave",     leaveslave},
    { "sigusr1",        sigusr1},
    { "sigusr2",        sigusr2},
    { 0, 0 }
};

struct action_lookup  ac_lookup_table[] = {
    { "exposetitle",    a_exposetitle},
    { "exposeslave",    a_exposeslave},
    { "print",          a_print },
    { "exec",           a_exec},
    { "exit",           a_exit},
    { "collapse",       a_collapse},
    { "uncollapse",     a_uncollapse},
    { "stick",          a_stick},
    { "unstick",        a_unstick},
    { "togglestick",    a_togglestick},
    { "hide",           a_hide},
    { "unhide",         a_unhide},
    { "scrollup",       a_scrollup},
    { "scrolldown",     a_scrolldown},
    { "menuprint",      a_menuprint},
    { "menuexec",       a_menuexec},
    { "raise",          a_raise},
    { "lower",          a_lower},
    { 0, 0 }
};

Ev ev_table[MAXEVENTS] = {{0}, {0}};

/* utilities */
void
do_action(int event) {
    int i;

    if(ev_table[event].isset)
        for(i=0; ev_table[event].action[i]->handler; i++)
            ev_table[event].action[i]->handler(ev_table[event].action[i]->options);
}

int
get_ev_id(char *evname) {
    int i;

    for(i=0; ev_lookup_table[i].name; i++) {
        if(strcmp(ev_lookup_table[i].name, evname) == 0)
            return ev_lookup_table[i].id;
    }
    return -1;
}

void *
get_action_handler(char *acname) {
    int i;

    for(i=0; ac_lookup_table[i].name; i++) {
        if(strcmp(ac_lookup_table[i].name, acname) == 0)
            return ac_lookup_table[i].handler;
    }
    return (void *)NULL;
}

void
free_ev_table(void) {
    int i, j;

    for(i=0; i<MAXEVENTS; i++) {
        if(ev_table[i].isset)
            for(j=0; ev_table[i].action[j]->handler; j++)
                free(ev_table[i].action[j]);
    }
}

void
fill_ev_table(char *input)
{
    char *str1, *str2, *str3, *str4,
         *token, *subtoken, *kommatoken, *dptoken;
    char *saveptr1, *saveptr2, *saveptr3, *saveptr4;
    int j, i=0, k=0;
    int eid=0;
    void *ah=0;


    for (j = 1, str1 = input; ; j++, str1 = NULL) {
        token = strtok_r(str1, ";", &saveptr1);
        if (token == NULL)
            break;

        for (str2 = token; ; str2 = NULL) {
            subtoken = strtok_r(str2, "=", &saveptr2);
            if (subtoken == NULL)
                break;
            if( (str2 == token) && ((eid = get_ev_id(subtoken)) != -1))
                ;
            else if(eid == -1)
                break;

            for (str3 = subtoken; ; str3 = NULL) {
                kommatoken = strtok_r(str3, ",", &saveptr3);
                if (kommatoken == NULL)
                    break;

                for (str4 = kommatoken; ; str4 = NULL) {
                    dptoken = strtok_r(str4, ":", &saveptr4);
                    if (dptoken == NULL) {
                        break;
                    }
                    if(str4 == kommatoken && str4 != token && eid != -1) {
                            ev_table[eid].isset = 1;
                            ev_table[eid].action[i] = malloc(sizeof(As));
                            if((ah = (void *)get_action_handler(dptoken)))
                                ev_table[eid].action[i]->handler= get_action_handler(dptoken);
                            i++;
                    }
                    else if(str4 != token && eid != -1 && ah) {
                        ev_table[eid].action[i-1]->options[k] = strdup(dptoken);
                        k++;
                    }
                    else if(!ah)
                        break;
                }
                k=0;
            }
            ev_table[eid].action[i] = malloc(sizeof(As));
            ev_table[eid].action[i]->handler = NULL;
            i=0; 
        }
    }
}


/* actions */

/* used internally */
int
a_exposetitle(char * opt[]) {
    XCopyArea(dzen.dpy, dzen.title_win.drawable, dzen.title_win.win, 
            dzen.gc, 0, 0, dzen.title_win.width, dzen.line_height, 0, 0);
    return 0;
}

/* used internally */
int
a_exposeslave(char * opt[]) {
    x_draw_body();
    return 0;
}

/* user selectable actions */
int
a_exit(char * opt[]) {
    if(opt[0]) 
        dzen.ret_val = atoi(opt[0]);
    dzen.running = False;
    return 0;
}

int
a_collapse(char * opt[]){
    int i;
    if(dzen.slave_win.max_lines && !dzen.slave_win.issticky) {
        for(i=0; i < dzen.slave_win.max_lines; i++)
            XUnmapWindow(dzen.dpy, dzen.slave_win.line[i]);
        XUnmapWindow(dzen.dpy, dzen.slave_win.win);
    }
    return 0;
}

int
a_uncollapse(char * opt[]){
    int i;
    if(dzen.slave_win.max_lines && !dzen.slave_win.issticky) {
        XMapRaised(dzen.dpy, dzen.slave_win.win);
        for(i=0; i < dzen.slave_win.max_lines; i++)
            XMapRaised(dzen.dpy, dzen.slave_win.line[i]);
        x_draw_body();
    }
    return 0;
}

int
a_togglecollapse(char * opt[]){
    return 0;
}

int
a_stick(char * opt[]) {
    if(dzen.slave_win.max_lines)
        dzen.slave_win.issticky = True;
    return 0;
}

int
a_unstick(char * opt[]) {
    if(dzen.slave_win.max_lines)
        dzen.slave_win.issticky = False;
    return 0;
}

int
a_togglestick(char * opt[]) {
    if(dzen.slave_win.max_lines)
        dzen.slave_win.issticky = dzen.slave_win.issticky ? False : True;
    return 0;
}

int
a_scrollup(char * opt[]) {
    if(dzen.slave_win.max_lines 
            && dzen.slave_win.first_line_vis 
            && dzen.slave_win.last_line_vis > dzen.slave_win.max_lines) {
        dzen.slave_win.first_line_vis--;
        dzen.slave_win.last_line_vis--;
        x_draw_body();
    }
    return 0;
}

int
a_scrolldown(char * opt[]) {
    if(dzen.slave_win.max_lines
            && dzen.slave_win.last_line_vis >= dzen.slave_win.max_lines 
            && dzen.slave_win.last_line_vis < dzen.slave_win.tcnt) {
        dzen.slave_win.first_line_vis++;
        dzen.slave_win.last_line_vis++;
        x_draw_body();
    }
    return 0;
}

int
a_hide(char * opt[]) {
    if(!dzen.title_win.ishidden) {
        XResizeWindow(dzen.dpy, dzen.title_win.win, dzen.title_win.width, 1);
        dzen.title_win.ishidden = True;
    }
    return 0;
}

int
a_unhide(char * opt[]) {
    if(dzen.title_win.ishidden) {
        XResizeWindow(dzen.dpy, dzen.title_win.win, dzen.title_win.width, dzen.line_height);
        dzen.title_win.ishidden = False;
    }
    return 0;
}

int
a_exec(char * opt[]) {
    int i;

    if(opt) 
        for(i=0; opt[i]; i++) 
            if(opt[i]) 
                spawn(opt[i]);
    return 0;
}

int
a_print(char * opt[]) {
    int i;

    if(opt) 
        for(i=0; opt[i]; i++)
            puts(opt[i]);
    return 0;
}

int
a_menuprint(char * opt[]) {
    if(dzen.slave_win.ismenu && dzen.slave_win.sel_line != -1 
            && (dzen.slave_win.sel_line + dzen.slave_win.first_line_vis) < dzen.slave_win.tcnt) {
        puts(dzen.slave_win.tbuf[dzen.slave_win.sel_line + dzen.slave_win.first_line_vis]);
        dzen.slave_win.sel_line = -1;
        fflush(stdout);
    }
    return 0;
}

int
a_menuexec(char * opt[]) {
    if(dzen.slave_win.ismenu && dzen.slave_win.sel_line != -1) {
        spawn(dzen.slave_win.tbuf[dzen.slave_win.sel_line + dzen.slave_win.first_line_vis]);
        dzen.slave_win.sel_line = -1;
    }
    return 0;
}

int 
a_raise(char * opt[]) {
    XRaiseWindow(dzen.dpy, dzen.title_win.win);

    if(dzen.slave_win.max_lines)
        XRaiseWindow(dzen.dpy, dzen.slave_win.win);
    return 0;
}

int 
a_lower(char * opt[]) {
    XLowerWindow(dzen.dpy, dzen.title_win.win);

    if(dzen.slave_win.max_lines)
        XLowerWindow(dzen.dpy, dzen.slave_win.win);
    return 0;
}
