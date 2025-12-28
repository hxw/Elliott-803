# BSDmakefile

TAPES += Elliott-Algol60-A104
TAPES += Elliott-Programs
TAPES += Elliott-Programs/X5
TAPES += H-Code-Compilers

SAMPLES += H-Code-Samples
SAMPLES += Algol60-Samples

VERSION ?= zero

PREFIX ?= /usr/local
DATADIR ?= ${PREFIX}/share/elliott-803
SHARE_DIR = ${DESTDIR}${DATADIR}

.for TD in ${TAPES}
TP += ${DATADIR}/${TD}
.endfor

# trailing ':' to search current directory last
DEFAULT_TAPE_DIR ?= ${TP:S/ /:/gW}:

.PHONY: all
all:
	${MAKE} -C emulator DEFAULT_TAPE_DIR="${DEFAULT_TAPE_DIR}" VERSION="${VERSION}" all

.PHONY: test
test: all
	${MAKE} -C emulator test

.PHONY: install
install: all
	${MAKE} -C emulator DESTDIR="${DESTDIR:tA}" PREFIX="${PREFIX}" DEFAULT_TAPE_DIR="${DEFAULT_TAPE_DIR}" install
.for TD in ${TAPES} ${SAMPLES}
	install -p -d -m 755 '${SHARE_DIR}/${TD}'
	for f in '${TD}'/* ; \
	do \
	  printf 'f: %s  d: %s\n' "$${f}" '${SHARE_DIR}/${TD}' ; \
	  if [ ! -d "$${f}" ] ; \
	  then \
	    install -m 644 "$${f}" '${SHARE_DIR}/${TD}' ; \
	  fi ; \
	done
.endfor

.PHONY: h-code-docs
h-code-docs:
	${MAKE} -C H-Code-Docs DESTDIR="${DESTDIR:tA}" PREFIX="${PREFIX}"

.PHONY: clean
clean:
	${MAKE} -C emulator clean
	${MAKE} -C H-Code-Docs complete-clean
