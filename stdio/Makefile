# Generated automatically from Makefile.in by configure.
#### Start of system configuration section. ####

srcdir =        .

ifeq ($(srcdir),.)
srcdir = ../../..
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

CFLAGS =        -O2
LDFLAGS =       

RANLIB =        ranlib
AR =            ar

DO_CATENATE =   no

#### End system configuration section ####

# I *love* GNU make!
define catenate
/bin/echo -n creating $@...
/bin/echo "$(^:%=#include \"%\"\n)" >$@
/bin/echo done
endef

ifeq ($(BASE), morphos)
FLAVOR_CFLAGS = -mcpu=$(FPU)
else
FLAVOR_CFLAGS = -m$(CPU) -m$(FPU)
endif

INCS =          -I$(srcdir) -I$(srcdir)/../library -I$(srcdir)/../include
DEFS =          -DFLOATING_POINT

ALL_CFLAGS =    $(CFLAGS) $(FLAVOR_CFLAGS) $(INCS) $(OTHER_CFLAGS) $(DEFS)

LIB =           libstdio.a

.c.o:
		$(CC) $(ALL_CFLAGS) -c $< -o $@

SRC =           $(notdir $(wildcard $(srcdir)/*.c))

ifeq ($(DO_CATENATE), yes)

$(LIB) :        all.o
		rm -f $@
		$(AR) rv $@ $^
		$(RANLIB) $@

all.c:          $(SRC)
		@$(catenate)

else

OBJS= $(SRC:.c=.o)

$(LIB):         $(OBJS)
		rm -f $@
		$(AR) rv $@ $^
		$(RANLIB) $@

endif


clean:
		rm -rf 680?0 notrap powerpc

clobber:        clean
		rm -f Makefile
