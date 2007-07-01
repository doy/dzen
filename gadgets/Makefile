# dzen2 gadgets
# (c) 2007 Robert Manea

include config.mk

SRC = dbar.c
OBJ = ${SRC:.c=.o}

all: options dbar

options:
	@echo dzen2 gadgets build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@echo "LD       = ${LD}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

dbar: ${OBJ}
	@echo LD $@
	@${LD} -o $@ ${OBJ} ${LDFLAGS}
	@strip $@

clean:
	@echo cleaning
	@rm -f ${OBJ} dbar

.PHONY: all options clean
