# Generated automatically from Makefile.in by configure.
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

CFLAGS =        -O2
LDFLAGS =       

ifeq ($(BASE), morphos)
RANLIB =        ranlib
AR =            ar
else
RANLIB =        m68k-amigaos-ranlib
AR =            m68k-amigaos-ar
endif

#### End system configuration section ####

ifeq ($(BASE), morphos)
FLAVOR_CFLAGS = -mcpu=$(FPU)
else
ifeq ($(BASE), morphos-rel)
FLAVOR_CFLAGS = -mcpu=$(FPU) -mbaserel
else
ifeq ($(BASE), morphos-rel32)
FLAVOR_CFLAGS = -mcpu=$(FPU) -mbaserel32
else
FLAVOR_CFLAGS = -m$(CPU) -m$(FPU) -f$(BASE)
endif
endif
endif

ALL_CFLAGS =    $(CFLAGS) $(OTHER_CFLAGS) $(FLAVOR_CFLAGS) $(INCS) $(DEFS)

DEFS =          -DTM_GMTOFF=tm_gmtoff -DTM_ZONE=tm_zone -DNOID
INCS =          -I$(srcdir) -I$(srcdir)/../library -I$(srcdir)/../include

.c.o:
		$(CC) $(ALL_CFLAGS) -c $< -o $@

SRC =           $(wildcard $(srcdir)/*.c)

OBJ =           $(notdir $(SRC:.c=.o))

libstatic.a:    $(OBJ)
		rm -f $@
		$(AR) rc $@ $(OBJ)
		$(RANLIB) $@

clean:
		rm -rf *baserel* *morphos*

clobber:        clean
		rm -f Makefile
