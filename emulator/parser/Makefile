# BSDmakefile

CFLAGS ?= -g -I. -Wall -Werror -std=c17

LIB = libparser.a

SRCS = parser.c

TESTS = parser_test.c

.PHONY: all
all: test

TEST_PROGRAMS = ${TESTS:S/.c$//}

.PHONY: test
test: ${LIB} ${TEST_PROGRAMS}
.for p in ${TEST_PROGRAMS}
	./${p}
.endfor

.for p in ${TEST_PROGRAMS}
${p}: ${p}.o ${LIB}
	${CC} ${CFLAGS} -o ${.TARGET} ${.ALLSRC}
.endfor


.PHONY: clean
clean:
	rm -f *.o
	rm -f .depend
	rm -f ${LIB}
	rm -f ${TEST_PROGRAMS}

OBJS = ${SRCS:S/.c$/.o/}

${LIB}: ${OBJS}
	ar -r ${LIB} ${OBJS}

depend:
	rm -f .depend
	env MKDEP_CPP_OPTS=-MM mkdep ${CFLAGS} ${SRCS} ${TESTS}

.sinclude ".depend"
