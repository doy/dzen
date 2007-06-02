# dzen version
VERSION = 0.3.0

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib
INCS = -I. -I/usr/include -I${X11INC}

# Uncomment this line to use Xinerama.
# USE_XINERAMA=y

# (ifdef/else/endif might not be portable; if it doesn't work for you,
# contact the author and we'll work on a better solution.)
ifdef USE_XINERAMA
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11 -lXinerama
CFLAGS = -Os ${INCS} -DVERSION=\"${VERSION}\" -DDZEN_XINERAMA
else
LIBS = -L/usr/lib -lc -L${X11LIB} -lX11
CFLAGS = -Os ${INCS} -DVERSION=\"${VERSION}\"
endif

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
