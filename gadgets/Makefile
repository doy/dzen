# dzen2 gadgets
# (c) 2007 Robert Manea

include config.mk

SRC = dbar.c textwidth.c
OBJ = ${SRC:.c=.o}

all: options dbar textwidth

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
	@${LD} -o $@ dbar.o ${LDFLAGS}
	@strip $@

textwidth: ${OBJ}
	@echo LD $@
	@${LD} -o $@ textwidth.o ${LDFLAGS} -L${X11LIB} -lX11
	@strip $@

clean:
	@echo cleaning
	@rm -f ${OBJ} dbar
	@rm -f ${OBJ} textwidth

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f dbar ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/dbar
	@cp -f textwidth ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/textwidth

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/dbar
	@rm -f ${DESTDIR}${PREFIX}/bin/textwidth

.PHONY: all options clean install uninstall
