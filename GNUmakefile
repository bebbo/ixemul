.PHONY: all clean install

all: Makefile
	@make -f Makefile

Makefile: Makefile.in db/Makefile.in general/Makefile.in glue/Makefile.in ixnet/Makefile.in library/Makefile.in libsrc/Makefile.in man/Makefile.in net/Makefile.in stack/Makefile.in static/Makefile.in stdio/Makefile.in stdlib/Makefile.in string/Makefile.in utils/Makefile.in
	@echo "Running configure script.."
	@chmod u+x configure
	@./configure --enable-ppc --disable-m68k --disable-cat --with-cpu=powerpc.604e --prefix=/gg
	@make all

clean:
ifeq ($(wildcard Makefile),Makefile)
	@make -f Makefile clean
else
	@echo "This is no real makefile.. just a fallback. Build ixemul yourself"
endif

install: all
	ppc-morphos-strip --remove-section=.comment -o sys:morphos/libs/ixemul.library library/powerpc/604e/morphos/ixemul.library
	ppc-morphos-strip --remove-section=.comment -o sys:morphos/libs/ixnet.library ixnet/powerpc/morphos/ixnet.library
	mkdir -p gg:ppc-morphos/lib/libb32
	cp libsrc/crt0.o libsrc/scrt0.o libsrc/lcrt0.o gg:ppc-morphos/lib
	cp libsrc/libc.a gg:ppc-morphos/lib
	cp libsrc/libb32c.a gg:ppc-morphos/lib/libb32/libc.a
	ppc-morphos-strip --remove-section=.comment -o sys:morphos/L/ixpipe-handler utils/ixpipe-handler
	cp utils/ixpipe sys:morphos/Devs/DOSDrivers/IXPIPE

install-iso: all
	mkdir -p $(ISOPATH)MorphOS/Libs
	ppc-morphos-strip --remove-section=.comment -o $(ISOPATH)MorphOS/Libs/ixemul.library library/powerpc/604e/morphos/ixemul.library
	ppc-morphos-strip --remove-section=.comment -o $(ISOPATH)MorphOS/Libs/ixnet.library ixnet/powerpc/morphos/ixnet.library
	ppc-morphos-strip --remove-section=.comment -o $(ISOPATH)MorphOS/L/ixpipe-handler utils/ixpipe-handler
	cp utils/ixpipe $(ISOPATH)MorphOS/Devs/DOSDrivers/IXPIPE

