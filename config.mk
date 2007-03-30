# dzen version
VERSION = 0.2.0

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# includes and libs
INCS = -I. -I/usr/include -I${X11INC}
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 -lpthread

# flags
#CFLAGS = -Os ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}
CFLAGS = -g -Wall -Os ${INCS} -DVERSION=\"${VERSION}\" -DPOSIX_SOURCE
LDFLAGS = -g ${LIBS}

# Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}
#CFLAGS += -xtarget=ultra

# compiler and linker
CC = cc
LD = ${CC}
