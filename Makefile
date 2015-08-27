# shr - Shared memory with n-fold bufferings zero-copy data streaming
# See LICENSE file for copyright and license details.

PREFIX = /usr
LIB = /lib
LIBDIR = ${PREFIX}${LIB}
INCLUDE = /include
INCLUDEDIR = ${PREFIX}${INCLUDE}
DATA = /share
DATADIR = ${PREFIX}${DATA}
LICENSEDIR = ${DATADIR}/licenses
MANDIR = ${DATADIR}/man

PKGNAME = shr


MAN3 = shr_create shr_remove shr_remove_by_key shr_open shr_reverse_dup shr_close shr_chown shr_chmod  \
       shr_stat shr_key_to_str shr_str_to_key shr_read shr_read_try shr_read_timed shr_read_done       \
       shr_write shr_write_try shr_write_timed shr_write_done
MAN7 = libshr


FLAGS = -std=c99 -Wall -Wextra -pedantic -O2

LIB_MAJOR = 1
LIB_MINOR = 0
LIB_VERSION = ${LIB_MAJOR}.${LIB_MINOR}
VERSION = 1.0


all: shr doc
doc: man
man: man3 man7

shr: bin/libshr.so.${LIB_VERSION} bin/libshr.so.${LIB_MAJOR} bin/libshr.so bin/libshr.a
man3: $(foreach M,${MAN3},bin/${M}.3)
man7: $(foreach M,${MAN7},bin/${M}.7)

bin/%.3: doc/%.3
	@echo SED $@
	@mkdir -p bin
	@sed 's/%VERSION%/${VERSION}/g' < $< > $@

bin/%.7: doc/%.7
	@echo SED $@
	@mkdir -p bin
	@sed 's/%VERSION%/${VERSION}/g' < $< > $@

bin/libshr.a: obj/shr-fpic.o
	@echo AR $@
	@mkdir -p bin
	@ar rcs $@ $^

bin/libshr.so.${LIB_VERSION}: obj/shr-fpic.o
	@echo LD -o $@
	@mkdir -p bin
	@${CC} ${FLAGS} -shared -Wl,-soname,libshr.so.${LIB_MAJOR} -o $@ $^ ${LDFLAGS}

bin/libshr.so.${LIB_MAJOR}:
	@echo LN -s $@
	@mkdir -p bin
	@ln -sf libshr.so.${LIB_VERSION} $@

bin/libshr.so:
	@echo LN -s $@
	@mkdir -p bin
	@ln -sf libshr.so.${LIB_VERSION} $@

obj/%-nofpic.o: src/%.c src/*.h
	@echo CC -c $@
	@mkdir -p obj
	@${CC} ${FLAGS} -c -o $@ ${CPPFLAGS} ${CFLAGS} $<

obj/%-fpic.o: src/%.c src/*.h
	@echo CC -c $@
	@mkdir -p obj
	@${CC} ${FLAGS} -fPIC -c -o $@ ${CPPFLAGS} ${CFLAGS} $<

install: install-so install-a install-h install-license install-doc
install-doc: install-man
install-man: install-man3 install-man7

install-so: bin/libshr.so.${LIB_VERSION}
	@echo INSTALL libshr.so
	@install -dm755 -- "${DESTDIR}${LIBDIR}"
	@install -m755 $^ -- "${DESTDIR}${LIBDIR}"
	@ln -sf -- "libshr.so.${LIB_VERSION}" "${DESTDIR}${LIBDIR}/libshr.so.${LIB_MAJOR}"
	@ln -sf -- "libshr.so.${LIB_VERSION}" "${DESTDIR}${LIBDIR}/libshr.so"

install-a: bin/libshr.a
	@echo INSTALL libshr.a
	@install -dm755 -- "${DESTDIR}${LIBDIR}"
	@install -m644 $^ -- "${DESTDIR}${LIBDIR}"

install-h:
	@echo INSTALL shr.h
	@install -dm755 -- "${DESTDIR}${INCLUDEDIR}"
	@install -m644 src/shr.h -- "${DESTDIR}${INCLUDEDIR}"

install-license:
	@echo INSTALL LICENSE
	@install -dm755 -- "${DESTDIR}${LICENSEDIR}/${PKGNAME}"
	@install -m644 LICENSE -- "${DESTDIR}${LICENSEDIR}/${PKGNAME}"

install-man3: $(foreach M,${MAN3},bin/${M}.3)
	@echo INSTALL $(foreach M,${MAN3},${M}.3)
	@install -dm755 -- "${DESTDIR}${MANDIR}/man3"
	@install -m644 $^ -- "${DESTDIR}${MANDIR}/man3"

install-man7: $(foreach M,${MAN7},bin/${M}.7)
	@echo INSTALL $(foreach M,${MAN7},${M}.7)
	@install -dm755 -- "${DESTDIR}${MANDIR}/man7"
	@install -m644 $^ -- "${DESTDIR}${MANDIR}/man7"

uninstall:
	-rm -- "${DESTDIR}${BINDIR}/shr"
	-rm -- "${DESTDIR}${LIBDIR}/libshr.so.${LIB_VERSION}"
	-rm -- "${DESTDIR}${LIBDIR}/libshr.so.${LIB_MAJOR}"
	-rm -- "${DESTDIR}${LIBDIR}/libshr.so"
	-rm -- "${DESTDIR}${LIBDIR}/libshr.a"
	-rm -- "${DESTDIR}${INCLUDEDIR}/shr.h"
	-rm -- "${DESTDIR}${LICENSEDIR}/${PKGNAME}/LICENSE"
	-rmdir -- "${DESTDIR}${LICENSEDIR}/${PKGNAME}"
	-rm -- $(foreach M,${MAN3},"${DESTDIR}${MANDIR}/man3/${M}.3")
	-rm -- $(foreach M,${MAN7},"${DESTDIR}${MANDIR}/man7/${M}.7")

clean:
	@echo cleaning
	@-rm -rf obj bin

.PHONY: all doc man shr man3 man7 install install-doc install-man install-a  \
        install-h install-license install-man3 install-man7 uninstall clean

