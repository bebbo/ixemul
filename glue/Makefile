# Generated automatically from Makefile.in by configure.
# Glue comes in two flavors, one for flat address spaces and one for base
# relative.  When gen_glue is compiled, which form of glue is generated
# when it is run depends upon whether "__no_baserel" or "__baserel" was defined
# when it was compiled.

#### Start of system configuration section. ####

srcdir =        .

ifeq ($(srcdir),.)
srcdir = ..
endif

VPATH :=        $(srcdir)

# Common prefix for machine-independent installed files.
prefix =        /usr/local

# Common prefix for machine-dependent installed files.
exec_prefix =   ${prefix}

bindir =        $(exec_prefix)/bin
libdir =        $(exec_prefix)/Sys/libs

INSTALL =       /usr/bin/install -c
INSTALL_DATA =  ${INSTALL} -m 644

CC =            gcc
AS =            

CFLAGS =        -O2
LDFLAGS =       

RANLIB =        ranlib
AR =            ar
RM =            rm -f

#### End system configuration section ####

# BASE is defined to be either "no-baserel" or "baserel" by an upper level
# makefile.  The default is "no-baserel".

BASE =          no-baserel

FLAVOR_CFLAGS =
OTHER_CFLAGS =
ALL_CFLAGS =    $(CFLAGS) $(FLAVOR_CFLAGS) $(OTHER_CFLAGS) $(INCS)

INCS =          -I$(srcdir) -idirafter $(srcdir)/../library -idirafter $(srcdir)/../include
LIB =           libglue.a
LIB_P =         libglue_p.a
SHELL =         /bin/sh

.c.o:
		$(CC) $(ALL_CFLAGS) -c $< -o $@

ifeq ($(BASE),baserel)
all:            $(LIB)
else
ifeq ($(BASE),baserel32)
all:            $(LIB)
else
ifeq ($(BASE),powerup)
all:            $(LIB)
else
ifeq ($(BASE),morphos)
all:            $(LIB)
else
ifeq ($(BASE),powerup-rel)
all:            $(LIB)
else
ifeq ($(BASE),morphos-rel)
all:            $(LIB)
else
ifeq ($(BASE),morphos-rel32)
all:            $(LIB)
else
all:            $(LIB) $(LIB_P)
endif
endif
endif
endif
endif
endif
endif

$(LIB):         gen_glue
		rm -f *.o
		./gen_glue $(BASE)
		$(AR) r $@ *.o
		rm -f *.o

$(LIB_P):       gen_glue
		rm -f *.o
		./gen_glue profiling
		$(AR) r $@ *.o
		rm -f *.o

gen_glue:       $(srcdir)/gen_glue.c $(srcdir)/../include/sys/syscall.def
		$(CC) $(ALL_CFLAGS) $< -o $@

clean:
		rm -rf *baserel* *morphos* *powerup*

clobber:        clean
		rm -f Makefile
