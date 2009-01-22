# Generated automatically from Makefile.in by configure.
#### Start of system configuration section. ####

srcdir =	.

# Common prefix for machine-independent installed files.
prefix =	/usr/local

# Common prefix for machine-dependent installed files.
exec_prefix =	${prefix}

bindir =	$(exec_prefix)/bin
libdir =	$(exec_prefix)/Sys/libs

INSTALL =	/usr/bin/install -c
INSTALL_DATA =	${INSTALL} -m 644

CC =		gcc

CFLAGS =	-O2
LDFLAGS =	

RANLIB =	ranlib
AR =		ar
RM =		rm -f

#### End system configuration section ####

SHELL =		/bin/sh

VPATH :=	$(wildcard $(srcdir)/man?)
DIRS :=		$(VPATH:$(srcdir)/man%=cat%)
SRC :=		$(wildcard $(srcdir)/man?/*.?)
DOCS :=		$(addsuffix .0,$(basename $(SRC:$(srcdir)/man%=cat%)))

all:		$(DIRS) $(DOCS)

suf =		.$(subst /,,$(patsubst cat%,%,$(dir $@)))

$(DOCS):
		groff -Tascii -mandoc $(srcdir)/$(join $(patsubst cat%,man%,$(basename $@)),$(suf)) >$@

$(DIRS):
		mkdir $@

clean:
		rm -rf cat?

clobber:	clean
		rm -f Makefile
