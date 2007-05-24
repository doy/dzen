# dzen version
VERSION = 0.2.3

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

INCS = -I. -I/usr/include -I${X11INC}
# No Xinerama:
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 -lpthread
# With Xinerama:
#LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 -lpthread -lXinerama

# No Xinerama:
CFLAGS = -Os ${INCS} -DVERSION=\"${VERSION}\"
# With Xinerama:
#CFLAGS = -Os ${INCS} -DVERSION=\"${VERSION}\" -DDZEN_XINERAMA
LDFLAGS = ${LIBS}

# Debugging
#CFLAGS = -g -Wall -O0 ${INCS} -DVERSION=\"${VERSION}\" -DPOSIX_SOURCE
#LDFLAGS = -g ${LIBS}

# Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}
#CFLAGS += -xtarget=ultra

# compiler and linker
CC = cc
LD = ${CC}
