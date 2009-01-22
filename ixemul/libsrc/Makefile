# Generated automatically from Makefile.in by configure.
#### Start of system configuration section. ####

srcdir =        .

VPATH :=         $(srcdir)

# Common prefix for machine-independent installed files.
prefix =        /usr/local

# Common prefix for machine-dependent installed files.
exec_prefix =   ${prefix}

bindir =        $(exec_prefix)/bin
libdir =        $(exec_prefix)/Sys/libs

INSTALL =       /usr/bin/install -c
INSTALL_DATA =  ${INSTALL} -m 644

CC =            gcc

CFLAGS =        -O2
LDFLAGS =       

RANLIB =        ranlib
AR =            ar
RM =            rm -f

OS =            amigaos

NATIVE_CC =     gcc

NATIVE_CFLAGS = -O2
NATIVE_LDFLAGS= 

#### End system configuration section ####

# For now we only build one CPU/FPU flavor of libc.a, which is the most generic version.

ifeq ($(CPU), powerpc) 
CPUDIR          = powerpc
FPUDIR          = ppc
FLAVOR_FLAGS    = -mmultiple -mcpu=604e
OTHER_CFLAGS    = -DNATIVE_MORPHOS
else
CPUDIR          = 68000
FPUDIR          = soft-float
FLAVOR_CFLAGS   =
OTHER_CFLAGS    =  -fomit-frame-pointer
endif

ALL_CFLAGS =    $(CFLAGS) $(FLAVOR_CFLAGS) $(OTHER_CFLAGS) $(INCS) $(DEFS)

DEFS =
INCS =          -I$(srcdir) -I$(srcdir)/../library -I$(srcdir)/../include -I. #-I/p
SHELL =         /bin/sh

ifeq ($(CPU), powerpc) 
NATIVE_FLAGS = $(NATIVE_CFLAGS) $(INC) -DNATIVE_MORPHOS -DCPU_$(FPU)
else
NATIVE_FLAGS = $(NATIVE_CFLAGS) $(INC) -DCPU_$(CPU) -DFPU_$(FPU)
endif

.c.o:
		$(CC) $(ALL_CFLAGS) -c $< -o $@

SRC =           v1.c

OBJ =           $(SRC:.c=.o)

# 'glue' must be the first item in this list!
LIBS =          glue static stack net db

ifeq ($(CPU), powerpc)
NOBASELIBS :=   $(foreach d, $(LIBS),../$(d)/morphos/lib$(d).a)
BASELIBS :=     $(foreach d, $(LIBS),../$(d)/morphos-rel/lib$(d).a)
LBASELIBS :=    $(foreach d, $(LIBS),../$(d)/morphos-rel32/lib$(d).a)
else
NOBASELIBS :=   $(foreach d, $(LIBS),../$(d)/no-baserel/lib$(d).a)
BASELIBS :=     $(foreach d, $(LIBS),../$(d)/baserel/lib$(d).a)
LBASELIBS :=    $(foreach d, $(LIBS),../$(d)/baserel32/lib$(d).a)
endif

ifeq ($(CPU), powerpc)

all:            libc.a libb32c.a crt0.o lcrt0.o scrt0.o crt0i.o lcrt0i.o scrt0i.o crt0ni.o

bcrt0.o:        crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -mbaserel -DSUPPORT_CTORS -DBASECRT0 $(INCS) -c $< -o $@

rcrt0.o:        crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -mbaserel -DSUPPORT_CTORS -DBASECRT0 -DRCRT0 $(INCS) -c $< -o $@

lcrt0.o:        crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -mbaserel32 -DSUPPORT_CTORS -DBASECRT0 -DLBASE $(INCS) -c $< -o $@

scrt0.o:        crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -mbaserel32 -DSUPPORT_CTORS -DBASECRT0 -DRCRT0 -DLBASE $(INCS) -c $< -o $@

crt0.o:         crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -DSUPPORT_CTORS $(INCS) -c -DCRT0 $< -o $@

crt0n.o:        crt0n.c
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -DSUPPORT_CTORS -c $< -o $@

bcrt0i.o:       crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -mbaserel -DSUPPORT_INIT -DBASECRT0 $(INCS) -c $< -o $@

rcrt0i.o:       crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -mbaserel -DSUPPORT_INIT -DBASECRT0 -DRCRT0 $(INCS) -c $< -o $@

lcrt0i.o:       crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -mbaserel32 -DSUPPORT_INIT -DBASECRT0 -DLBASE $(INCS) -c $< -o $@

scrt0i.o:       crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -mbaserel32 -DSUPPORT_INIT -DBASECRT0 -DRCRT0 -DLBASE $(INCS) -c $< -o $@

crt0i.o:        crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -DSUPPORT_INIT $(INCS) -c -DCRT0 $< -o $@

crt0ni.o:       crt0n.c
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -DSUPPORT_INIT -c $< -o $@


else

all:            libc.a libc_p.a libbc.a libb32c.a crt0.o lcrt0.o bcrt0.o rcrt0.o \
		mcrt0.o gcrt0.o scrt0.o

bcrt0.o:        crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -fbaserel -DBASECRT0 $(INCS) -c $< -o $@

rcrt0.o:        crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -fbaserel -DBASECRT0 -DRCRT0 $(INCS) -c $< -o $@

lcrt0.o:        crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -m68020 -fbaserel32 -DBASECRT0 -DLBASE $(INCS) -c $< -o $@

scrt0.o:        crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) -m68020 -fbaserel32 -DBASECRT0 -DRCRT0 -DLBASE $(INCS) -c $< -o $@

crt0.o:         crt0.c version.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) $(INCS) -c -DCRT0 $< -o $@

mcrt0.o:        crt0.c version.h gmon.c gmon.h
		$(CC) $(CFLAGS) $(OTHER_CFLAGS) $(INCS) -c -DMCRT0 -fno-omit-frame-pointer $< -o $@

gcrt0.o:        mcrt0.o
		cp $< $@

endif


define createlib
	$(RM) -r $@ tempobj
	@mkdir tempobj
	cp $(OBJ) tempobj
	cp $< $@
	@(cd tempobj; for i in $(filter-out $< $(OBJ),$^) ; do echo extracting library $$i ; $(AR) x ../$$i ; done)
	(cd tempobj; $(AR) q ../$@ *.o)
	$(RANLIB) $@
	$(RM) -r tempobj
endef

libc.a libc_p.a libbc.a libb32c.a: $(OBJ)

libc.a:         $(NOBASELIBS)
		$(createlib)

libbc.a:        $(BASELIBS)
		$(createlib)

libb32c.a:      $(LBASELIBS)
		$(createlib)

libc_p.a:       $(NOBASELIBS:%glue.a=%glue_p.a)
		$(createlib)

version.h:      parse_version $(srcdir)/../version.in
		./parse_version $(srcdir) >$@

parse_version:  parse_version.o
		$(NATIVE_CC) $(NATIVE_LDFLAGS) $< -o $@

parse_version.o:  $(srcdir)/../library/parse_version.c
		$(NATIVE_CC) $(NATIVE_FLAGS) -c $< -o $@

clean:
		rm -rf tempobj *.[oa]
		rm -f parse_version

clobber:        clean
		rm -f Makefile version.h
