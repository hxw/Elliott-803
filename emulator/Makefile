# BSDmakefile

PREFIX ?= /usr/local
VERSION ?= zero

MAN1_DIR = ${DESTDIR}${PREFIX}/man/man1
BIN_DIR = ${DESTDIR}${PREFIX}/bin

PROG = emu803
MAN1 = man1/emu803.1

CFLAGS = -g -Wall -Werror -pedantic -std=c17 -Wstrict-prototypes

.ifdef STRICT
CFLAGS += -Weverything -Wno-padded -Wno-switch-enum -Wno-unused-parameter
CFLAGS += -Wno-sign-conversion -Wno-vla -Wno-shift-sign-overflow -Wno-format-nonliteral
CFLAGS += -Wno-declaration-after-statement
CFLAGS += -Wno-used-but-marked-unused
.endif

CFLAGS += -I.
CFLAGS += -D VERSION_STRING=\"${VERSION}\"
CFLAGS += -D PROGRAM_STRING=\"${PROG}\"
.ifdef DEFAULT_TAPE_DIR
CFLAGS += -D DEFAULT_TAPE_DIR=\"${DEFAULT_TAPE_DIR}\"
.endif

LIBS = -lcursesw -lthr -lrt -Lcpu -l803 -Lio5 -lio5 -Lparser -lparser


SRCS = main.c emulator.c commands.c pathsearch.c
TESTS =

.PHONY: all
all: libs
	cc ${CFLAGS} -o ${PROG} ${SRCS} ${LIBS}

.PHONY: test
test:

.PHONY: install
install:
	install -d -m 755 "${BIN_DIR}"
	install -C -m 755 "${PROG}" "${BIN_DIR}/${PROG}"
	install -d -m 755 "${MAN1_DIR}"
	install -C -m 644 "${MAN1}" "${MAN1_DIR}"


.PHONY: libs
libs:
	${MAKE} CFLAGS='${CFLAGS}' -C parser depend
	${MAKE} CFLAGS='${CFLAGS}' -C parser
	${MAKE} CFLAGS='${CFLAGS}' -C io5 depend
	${MAKE} CFLAGS='${CFLAGS}' -C io5
	${MAKE} CFLAGS='${CFLAGS}' -C cpu depend
	${MAKE} CFLAGS='${CFLAGS}' -C cpu


.PHONY: clean
clean:
	make -C parser clean
	make -C io5 clean
	make -C cpu clean
	rm -f *.o
	rm -f .depend
	rm -f "${PROG}"

.PHONY: depend
depend:
	rm -f .depend
	env MKDEP_CPP_OPTS=-MM mkdep ${CFLAGS} ${SRCS} ${TESTS}

.sinclude ".depend"
