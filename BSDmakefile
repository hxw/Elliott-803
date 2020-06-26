# BSDmakefile

TAPES += Elliott-Algol60-A104
TAPES += Elliott-programs
TAPES += h-code-compilers
TAPES += hello

PREFIX ?= /usr/local

.for d in ${TAPES}
p += ${PREFIX}/${d}
.endfor

DEFAULT_TAPE_DIR ?= ${p:S/ /:/gW}

SHARE_DIR = ${DESTDIR}${PREFIX}/share/Elliott-803

.PHONY: all
all:
	${MAKE} -C emulator DEFAULT_TAPE_DIR="${DEFAULT_TAPE_DIR}" all

.PHONY: test
test:
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
