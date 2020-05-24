# BSDmakefile

TAPES += Elliott-Algol60-A104
TAPES += Elliott-programs
TAPES += h-code-compilers
TAPES += hello

PREFIX ?= /usr/local

SHARE_DIR = ${DESTDIR}${PREFIX}/share/Elliott803

.PHONY: all
all:
	${MAKE} -C emulator

.PHONY: test
test:
	${MAKE} -C emulator test

.PHONY: install
install: all
	${MAKE} -C emulator DESTDIR="${DESTDIR:tA}" PREFIX="${PREFIX}" install
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
