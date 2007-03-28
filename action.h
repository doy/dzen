/*  
 *  (C)opyright MMVII Robert Manea <rob dot manea at gmail dot com>
 *  See LICENSE file for license details.
 *
 */

/* Event, Action data structures */
typedef struct AS As;
typedef struct EV Ev;

enum ev_id {
    /* internal events, should not be used by the user */
    exposetitle, exposeslave,
    /* mouse buttons */
    button1, button2, button3, button4, button5,
    /* entering/leaving windows */
    entertitle, leavetitle, enterslave, leaveslave, 
    /* external signals */
    sigusr1, sigusr2
};

struct event_lookup {
    char *name;
    int id;
};

struct action_lookup {
    char *name;
    int (*handler)(char **);
};

struct AS {
    char *options[256];
    int (*handler)(char **);
};

struct EV {
    int isset;
    As *action[256];
};

extern Ev ev_table[256];

/* utility functions */
void do_action(int);
int get_ev_id(char *);
void * get_action_handler(char *);
void fill_ev_table(char *);

/* action handlers */
int a_exposetitle(char **);
int a_exposeslave(char **);
int a_print(char **);
int a_exit(char **);
int a_exec(char **);
int a_collapse(char **);
int a_uncollapse(char **);
int a_stick(char **);
int a_unstick(char **);
int a_togglestick(char **);
int a_scrollup(char **);
int a_scrolldown(char **);
int a_hide(char **);
int a_unhide(char **);
int a_menuprint(char **);
int a_menuexec(char **);

