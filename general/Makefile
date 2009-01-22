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

ifeq ($(CPU), powerpc)
FLAVOR_CFLAGS = -mcpu=$(FPU)
FIXED_A4 = -ffixed-r13
else
FLAVOR_CFLAGS = -m$(CPU) -m$(FPU)
FIXED_A4 = -ffixed-a4
endif

INCS =          -I$(srcdir) -I$(srcdir)/../library -I$(srcdir)/../include

ALL_CFLAGS =    $(CFLAGS) $(FLAVOR_CFLAGS) $(INCS) $(OTHER_CFLAGS)

LIB =           libgeneral.a

.c.o:
		$(CC) $(ALL_CFLAGS) -c $< -o $@

A4_SRC =        glob.c

SRC =           $(filter-out $(A4_SRC),$(notdir $(wildcard $(srcdir)/*.c)))

ifeq ($(DO_CATENATE), yes)

$(LIB):         all.o a4.o
		rm -f $@
		$(AR) rv $@ $^
		$(RANLIB) $@

all.c:          $(SRC)
		@$(catenate)

a4.c:           $(A4_SRC)
		@$(catenate)

a4.o:           a4.c
		$(CC) $(ALL_CFLAGS) $(FIXED_A4) -c $< -o $@

else

OBJS= $(SRC:.c=.o)
A4_OBJS= $(A4_SRC:.c=.o)

$(LIB):         $(OBJS) $(A4_OBJS)
		rm -f $@
		$(AR) rv $@ $^
		$(RANLIB) $@

$(A4_OBJS): %.o: %.c
		$(CC) $(ALL_CFLAGS) $(FIXED_A4) -c $< -o $@

endif

clean:
		rm -rf 680?0 notrap powerpc

clobber:        clean
		rm -f Makefile
