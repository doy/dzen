# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

INCS = -I. -I/usr/include

# No Xinerama, comment the following two lines for Xinerama
LIBS = -L/usr/lib 
CFLAGS = -Os ${INCS} 
LDFLAGS = ${LIBS}

# compiler and linker
CC = gcc
LD = ${CC}
