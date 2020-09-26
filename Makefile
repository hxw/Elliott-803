# BSDmakefile

TAPES += Elliott-Algol60-A104
TAPES += Elliott-Programs
TAPES += Elliott-Programs/X5
TAPES += H-Code-Compilers
TAPES += hello
TAPES += Algol60-Samples

VERSION ?= zero

PREFIX ?= /usr/local
DATADIR ?= ${PREFIX}/share/Elliott-803
SHARE_DIR = ${DESTDIR}${DATADIR}

.for d in ${TAPES}
p += ${DATADIR}/${d}
.endfor

DEFAULT_TAPE_DIR ?= ${p:S/ /:/gW}

.PHONY: all
all:
	${MAKE} -C emulator DEFAULT_TAPE_DIR="${DEFAULT_TAPE_DIR}" VERSION="${VERSION}" all

.PHONY: test
test: all
	${MAKE} -C emulator test

.PHONY: install
install: all
	${MAKE} -C emulator DESTDIR="${DESTDIR:tA}" PREFIX="${PREFIX}" DEFAULT_TAPE_DIR="${DEFAULT_TAPE_DIR}" install
.for d in ${TAPES}
	install -p -d -m 755 "${SHARE_DIR}/${d}"
	for f in "${d}"/* ; \
	do \
	  install -m 644 "$${f}" "${SHARE_DIR}/${d}" ; \
	done
.endfor

.PHONY: clean
clean:
	make -C emulator clean
