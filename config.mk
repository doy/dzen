# dzen version
VERSION = 0.5.0

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib
INCS = -I. -I/usr/include -I${X11INC}


# No Xinerama, comment the following two lines for Xinerama
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11
CFLAGS = -Os ${INCS} -DVERSION=\"${VERSION}\"

# With Xinerama, uncomment the following two lines for Xinerama
#LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 -lXinerama
#CFLAGS = -Os ${INCS} -DVERSION=\"${VERSION}\" -DDZEN_XINERAMA

LDFLAGS = ${LIBS}

# Solaris, uncomment for Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}
#CFLAGS += -xtarget=ultra

# Debugging
#CFLAGS = -g -Wall -O0 ${INCS} -DVERSION=\"${VERSION}\" -DPOSIX_SOURCE
#LDFLAGS = -g ${LIBS}

# compiler and linker
CC = cc
LD = ${CC}
