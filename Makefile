# Generated automatically from Makefile.in by configure.
#### Start of system configuration section. ####

srcdir =        .

# Common prefix for machine-independent installed files.
prefix =        /usr/local

# Common prefix for machine-dependent installed files.
exec_prefix =   ${prefix}

bindir =        $(exec_prefix)/bin
libdir =        $(exec_prefix)/lib
syslibsdir =    $(exec_prefix)/Sys/libs
sysldir =       $(exec_prefix)/Sys/l
incdir =        $(exec_prefix)/include
etcdir =        $(exec_prefix)/etc
mandir =        $(prefix)/man

INSTALL =         /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA =    ${INSTALL} -m 644

# Will we produce 68k or ppc binaries ?

BUILD_M68K =    yes
BUILD_PPC =     no

# Tools used to build native executables for the machine used for
# compilation.

CC =            gcc
AS =            
AR =            ar
CFLAGS =        -O2
LDFLAGS =       
COPTFLAGS =     
RANLIB =        ranlib

# Tools used to produce 68k executables.

CC68K =         m68k-amigaos-gcc -V 3.4.0
AS68K =         m68k-amigaos-as
AR68K =         m68k-amigaos-ar
CFLAGS68K =     
LDFLAGS68K =    
COPTFLAGS68K =  
RANLIB68K =     m68k-amigaos-ranlib

# Tools used to produce ppc executables.

CCPPC =         ppc-morphos-gcc
ASPPC =         ppc-morphos-as
ARPPC =         ppc-morphos-ar
CFLAGSPPC =     
LDFLAGSPPC =    
COPTFLAGSPPC =  
RANLIBPPC =     ppc-morphos-ranlib

#### End system configuration section ####

# Supported OSes
OS =            morphos amigaos #pos

# Directories that depend on baserel/baserel32/no-baserel only
BASE =          glue stack static net db

# Directories that depend on the CPU and OS type
CPUOSONLY =     ixnet

# Directories that depend on the CPU, FPU and OS type
CPUFPUOS =      general string stdlib stdio wchar library

# Directories that depend on both baserel/baserel32/no-baserel and CPU/FPU type
# None at the moment, although 'stack' and 'static' should be moved here
# once we can use a CPU/FPU specific libc.a.
BASECPUFPU =

# For use in 'make clean' and 'make clobber'
DIRS =          $(BASE) $(CPUOSONLY) $(CPUFPUOS) $(BASECPUFPU)


###  CPU-FPU types.
###
###  The default thing to do is to make all reasonable combinations of the
###  library.
###
###  Note that libsrc, which builds the runtime startup files and all
###  versions of libc.a, and utils completely ignore the base/cpu/fpu variables,
###  so these are handled in the all:
###
###  There is one special combination: "notrap.soft-float".
###  In this case an 68000.soft-float version will be compiled with the -DNOTRAP
###  option. See the OTHER_CFLAGS explanation for more information.
###
###  You can remove specific CPU/FPU combinations that you are not interested in.
###
###  Examples of valid entries:
###
###  68000.soft-float, notrap.soft-float,
###  68020.soft-float 68020.68881 68040.soft-float 68040.68881
###  68060.68881 68060.soft-float
###  powerpc.604e powerpc.603e

CPU-FPU-TYPES = 68060.68881 68060.soft-float powerpc.604e powerpc.603e

# Some additional options for OTHER_CFLAGS that may or may not be useful are:
#
# -DDEBUG_VERSION       (build a debugging version)
# -DNOTRAP              (build a version that uses tc_UserData instead of
#                        tc_TrapData and also doesn't install any traphandlers.
#                        This can be used for debugging with a debugger other
#                        than gdb.

OTHER_CFLAGS = -static -Wall -O2 -DNOTRAP

# Flags used when compiling 68k code
M68K_CFLAGS = -fomit-frame-pointer

# Flags used when compiling ppc code (MorphOS and PowerUp)
POWERPC_CFLAGS = -mmultiple -fsigned-char

# Flags used when compiling for MorphOS
MORPHOS_CFLAGS = -DNATIVE_MORPHOS

# Flags used when compiling for PowerUp
POWERUP_CFLAGS = -DSHAREDL

###############################################
###                                         ###
###  DO NOT EDIT ANYTHING BELOW THIS LINE!  ###
###                                         ###
###############################################

### All targets that are generated look like this:
###
###     directory-name.cpu.fpu.base
###
### The following four defines extract one of these fields from the target name:

dir  = $(word 1, $(subst ., ,$@))
cpu  = $(word 2, $(subst ., ,$@))
fpu  = $(word 3, $(subst ., ,$@))
base = $(word 4, $(subst ., ,$@))
os   = $(word 4, $(subst ., ,$@))


FLAGS_TO_PASS = "AS=$(AS)"                      \
		"BASE=$(base)"                  \
		"BUILDDIR=$(dir)"               \
		"CC=$(CC)"                      \
		"CFLAGS=$(CFLAGS) $(OTHER_CFLAGS)" \
		"CPU=$(cpu)"                    \
		"FPU=$(fpu)"                    \
		"LDFLAGS=$(LDFLAGS)"            \
		"OS=$(os)"                      \
		"AR=$(AR)"                      \
		"RANLIB=$(RANLIB)"

PPC_FLAGS_TO_PASS = "AS=$(ASPPC)"                   \
		    "BASE=$(base)"                  \
		    "BUILDDIR=$(dir)"               \
		    "CC=$(CCPPC)"                   \
		    "CFLAGS=$(CFLAGSPPC) $(POWERPC_CFLAGS) $(OTHER_CFLAGS)" \
		    "CPU=$(cpu)"                    \
		    "FPU=$(fpu)"                    \
		    "LD=$(LDPPC)"                   \
		    "LDFLAGS=$(LDFLAGSPPC)"         \
		    "OS=$(os)"                      \
		    "AR=$(ARPPC)"                   \
		    "RANLIB=$(RANLIBPPC)"

M68K_FLAGS_TO_PASS = "AS=$(AS68K)"                   \
		     "BASE=$(base)"                  \
		     "BUILDDIR=$(dir)"               \
		     "CC=$(CC68K)"                   \
		     "CFLAGS=$(CFLAGS68K) $(M68K_CFLAGS) $(OTHER_CFLAGS)" \
		     "CPU=$(cpu)"                    \
		     "FPU=$(fpu)"                    \
		     "LD=$(LD68K)"                   \
		     "LDFLAGS=$(LDFLAGS68K)"         \
		     "OS=$(os)"                      \
		     "AR=$(AR68K)"                   \
		     "RANLIB=$(RANLIB68K)"

### Generate the targets we have to build. First we create defines for each
### type (dependent on base only, or only on cpu-fpu, or dependent on all
### three). Only the directory needs to be filled in in the second stage.

m68k-base-types         = $(d).68000.soft-float.baserel \
			  $(d).68020.soft-float.baserel32 \
			  $(d).68000.soft-float.no-baserel

m68k-cpu-types          = $(d).m68k

m68k-cpu-os-types       = $(addprefix $(d).,$(sort $(filter-out powerpc.%, $(CPU-FPU-TYPES:.68881=.soft-float))))

m68k-cpu-fpu-os-types   = $(addprefix $(d).,$(filter-out powerpc.%, $(CPU-FPU-TYPES)))

m68k-base-cpu-fpu-types = $(addsuffix .baserel, $(cpu-fpu-types)) \
			  $(addsuffix .baserel32, $(cpu-fpu-types)) \
			  $(addsuffix .no-baserel, $(cpu-fpu-types))


ppc-base-types         = $(d).powerpc.604e.morphos \
			 $(d).powerpc.604e.morphos-rel32 \
#			 $(d).powerpc.604e.morphos-rel \
#                        $(d).powerpc.604e.powerup

ppc-cpu-types          = $(d).powerpc

ppc-cpu-os-types       = $(addprefix $(d).,$(sort $(filter powerpc.%, $(CPU-FPU-TYPES))))

ppc-cpu-fpu-os-types   = $(addprefix $(d).,$(filter powerpc.%, $(CPU-FPU-TYPES)))

ppc-base-cpu-fpu-types = $(addsuffix .morphos, $(cpu-fpu-types)) \
			 $(addsuffix .morphos-rel32, $(cpu-fpu-types)) \
#			 $(addsuffix .morphos-rel, $(cpu-fpu-types)) \
#                        $(addsuffix .powerup, $(cpu-fpu-types))


### Now fill in the directory name

ifeq ($(BUILD_M68K), yes)
	m68k-base-targets         := $(foreach d, $(BASE), $(m68k-base-types))
	m68k-cpu-targets          := $(foreach d, $(CPUONLY), $(m68k-cpu-types))
	m68k-cpu-os-targets       := $(foreach o, $(filter amigaos pos, $(OS)), $(addsuffix .$(o),$(foreach d, $(CPUOSONLY), $(m68k-cpu-os-types))))
	m68k-cpu-fpu-os-targets   := $(foreach o, $(filter amigaos pos, $(OS)), $(addsuffix .$(o),$(foreach d, $(CPUFPUOS), $(m68k-cpu-fpu-os-types))))
	m68k-base-cpu-fpu-targets := $(foreach d, $(BASECPUFPU), $(m68k-base-cpu-fpu-types))
endif

ifeq ($(BUILD_PPC), yes)
	ppc-base-targets         := $(foreach d, $(BASE), $(ppc-base-types))
	ppc-cpu-targets          := $(foreach d, $(CPUONLY), $(ppc-cpu-types))
	ppc-cpu-os-targets       := $(foreach o, $(filter morphos powerup, $(OS)), $(addsuffix .$(o),$(foreach d, $(CPUOSONLY), $(ppc-cpu-os-types))))
	ppc-cpu-fpu-os-targets   := $(foreach o, $(filter morphos powerup, $(OS)), $(addsuffix .$(o),$(foreach d, $(CPUFPUOS), $(ppc-cpu-fpu-os-types))))
	ppc-base-cpu-fpu-targets := $(foreach d, $(BASECPUFPU), $(ppc-base-cpu-fpu-types))
endif



### Make all targets and libsrc, utils and man pages.

all:    $(gcc-m68k-ok) $(m68k-base-targets) $(m68k-cpu-os-targets) $(m68k-cpu-fpu-os-targets) \
	$(m68k-base-cpu-fpu-targets) $(gcc-ppc-ok) $(ppc-base-targets) $(ppc-cpu-os-targets) \
	$(ppc-cpu-fpu-os-targets) $(ppc-base-cpu-fpu-targets)
	@(cd libsrc && $(MAKE) $(FLAGS_TO_PASS) CC=m68k-amigaos-gcc) 
	@(cd libsrc && $(MAKE) $(PPC_FLAGS_TO_PASS) "OTHER_CFLAGS=$(MORPHOS_CFLAGS)" "OS=$(os)")
	@(cd utils && $(MAKE) $(FLAGS_TO_PASS) CC=ppc-morphos-gcc)
#        @(cd man && $(MAKE) $(FLAGS_TO_PASS))


#inlines:    ../inlines/Makefile
#	 @(cd ../inlines &&$(MAKE))

#../inlines/Makefile:
#	 @mkdir -p ../inlines
#	 @(cd ../inlines && $(srcdir)/../inlines/configure --prefix=$(prefix))

gcc-m68k-ok:
ifeq ($(BUILD_M68K), yes)
	@echo >t.c
	@echo If the next command fails, then your gcc is out of date!
	$(CC68K) -c -m68020 -resident32 -mrestore-a4 t.c
	@rm -f t.c t.o
endif
	@echo >gcc-m68k-ok

gcc-ppc-ok:
ifeq ($(BUILD_PPC), yes)
	@echo >t.c
	@echo If the next command fails, then your gcc can't generate PowerPC code
	$(CCPPC) -c -mcpu=604e t.c
	@rm -f t.c t.o
endif
	@echo >gcc-ppc-ok

###  Some targets are only dependent on BASE, others on CPU-FPU, and
###  others depend on all three.

###  This makes targets that are only dependent on BASE.

$(m68k-base-targets):
	@if [ ! -d $(dir)/$(base) ]; then       \
	    echo mkdir -p $(dir)/$(base);       \
	    mkdir -p $(dir)/$(base);            \
	fi;                                     \
	if [ $(dir) = "glue" ]; then            \
	  (cd $(dir)/$(base) ;                          \
	   $(MAKE) -f ../Makefile $(FLAGS_TO_PASS)      \
		"AR=$(AR68K)");                         \
	else                                            \
	  (cd $(dir)/$(base) ;                          \
	   $(MAKE) -f ../Makefile $(M68K_FLAGS_TO_PASS));\
	fi

$(ppc-base-targets):
	@if [ ! -d $(dir)/$(base) ]; then       \
	    echo mkdir -p $(dir)/$(base);       \
	    mkdir -p $(dir)/$(base);            \
	fi;                                     \
	if [ $(dir) = "glue" ]; then            \
	  (cd $(dir)/$(base) ;                          \
	    $(MAKE) -f ../Makefile $(FLAGS_TO_PASS)     \
		"AR=$(ARPPC)");                         \
	else                                            \
	  (cd $(dir)/$(base) ;                          \
	    $(MAKE) -f ../Makefile $(PPC_FLAGS_TO_PASS) \
		"OTHER_CFLAGS=$(MORPHOS_CFLAGS)");      \
	fi


###  This makes targets that are only dependent on the CPU.
###  Override the settings CPU and OTHER_CFLAGS if CPU is "notrap".

$(m68k-cpu-os-targets):
	@if [ ! -d $(dir)/$(cpu)/$(os) ]; then                          \
		echo mkdir -p $(dir)/$(cpu)/$(os);                      \
		mkdir -p $(dir)/$(cpu)/$(os);                           \
	fi;                                                             \
	if [ $(cpu) = "notrap" ]; then                                  \
		(cd $(dir)/$(cpu)/$(os) ;                               \
		$(MAKE) -f ../../Makefile $(M68K_FLAGS_TO_PASS)         \
		  CPU=68000 "OTHER_CFLAGS=$(M68K_CFLAGS) -DNOTRAP");    \
	else                                                            \
		(cd $(dir)/$(cpu)/$(os) ;                               \
		$(MAKE) -f ../../Makefile $(M68K_FLAGS_TO_PASS));       \
	fi;

$(ppc-cpu-os-targets):
	@if [ ! -d $(dir)/$(cpu)/$(os) ]; then                          \
		echo mkdir -p $(dir)/$(cpu)/$(os);                      \
		mkdir -p $(dir)/$(cpu)/$(os);                           \
	fi;                                                             \
	if [ $(os) = "morphos" ]; then                                  \
		(cd $(dir)/$(cpu)/$(os) ;                               \
		$(MAKE) -f ../../Makefile $(PPC_FLAGS_TO_PASS)          \
		  "OTHER_CFLAGS=$(MORPHOS_CFLAGS)");                    \
	else                                                            \
		(cd $(dir)/$(cpu)/$(os) ;                               \
		$(MAKE) -f ../../Makefile $(PPC_FLAGS_TO_PASS)          \
		  "OTHER_CFLAGS=$(POWERUP_CFLAGS)");                    \
	fi;


###  This makes targets that are only dependent on CPU and FPU.
###  Override the settings CPU and OTHER_CFLAGS if CPU is "notrap".

$(m68k-cpu-fpu-os-targets):
	@if [ ! -d $(dir)/$(cpu)/$(fpu)/$(os) ]; then           \
		echo mkdir -p $(dir)/$(cpu)/$(fpu)/$(os);       \
		mkdir -p $(dir)/$(cpu)/$(fpu)/$(os);            \
	fi;                                                     \
	if [ $(cpu) = "notrap" ]; then                                  \
		(cd $(dir)/$(cpu)/$(fpu)/$(os) ;                        \
		$(MAKE) -f ../../../Makefile $(M68K_FLAGS_TO_PASS)      \
		  CPU=68000 "OTHER_CFLAGS= -DNOTRAP");                  \
	else                                                            \
		(cd $(dir)/$(cpu)/$(fpu)/$(os) ;                        \
		$(MAKE) -f ../../../Makefile $(M68K_FLAGS_TO_PASS)	\
		  "OTHER_CFLAGS=$(OTHER_CFLAGS)");			\
	fi;

$(ppc-cpu-fpu-os-targets):
	@if [ ! -d $(dir)/$(cpu)/$(fpu)/$(os) ]; then           \
		echo mkdir -p $(dir)/$(cpu)/$(fpu)/$(os);       \
		mkdir -p $(dir)/$(cpu)/$(fpu)/$(os);            \
	fi;                                                     \
	if [ $(os) = "morphos" ]; then                                  \
		(cd $(dir)/$(cpu)/$(fpu)/$(os) ;                        \
		$(MAKE) -f ../../../Makefile $(PPC_FLAGS_TO_PASS)       \
		  "OTHER_CFLAGS=$(MORPHOS_CFLAGS) $(OTHER_CFLAGS)");    \
	else                                                            \
		(cd $(dir)/$(cpu)/$(fpu)/$(os) ;                        \
		$(MAKE) -f ../../../Makefile $(PPC_FLAGS_TO_PASS)       \
		  "OTHER_CFLAGS=$(POWERUP_CFLAGS) $(OTHER_CFLAGS)");    \
	fi;


###  This makes targets that are dependent on CPU, FPU, and BASE.

$(m68k-base-cpu-fpu-targets):
	@if [ ! -d $(dir)/$(base)/$(cpu)/$(fpu) ]; then         \
		echo mkdir -p $(dir)/$(base)/$(cpu)/$(fpu);     \
		mkdir -p $(dir)/$(base)/$(cpu)/$(fpu);          \
	fi;                                                     \
	(cd $(dir)/$(base)/$(cpu)/$(fpu) ;                      \
	$(MAKE) -f ../../../Makefile $(M68K_FLAGS_TO_PASS));

$(ppc-base-cpu-fpu-targets):
	@if [ ! -d $(dir)/$(base)/$(cpu)/$(fpu) ]; then         \
		echo mkdir -p $(dir)/$(base)/$(cpu)/$(fpu);     \
		mkdir -p $(dir)/$(base)/$(cpu)/$(fpu);          \
	fi;                                                     \
	(cd $(dir)/$(base)/$(cpu)/$(fpu) ;                      \
	$(MAKE) -f ../../../Makefile $(PPC_FLAGS_TO_PASS));


###  Install all the libraries, include files, runtime files, etc.

ixemul-targets   := $(addprefix ixemul.,$(foreach o, $(OS), $(addsuffix .$(o),$(CPU-FPU-TYPES))))
ixcpu  = $(subst 68,,$(word 2, $(subst ., ,$@)))


$(ixemul-targets):
	@echo Installing $@;                                            \
	if [ "$(fpu)" = "68881" ]; then                                 \
		fpu=-fpu;                                               \
	else                                                            \
		fpu=;                                                   \
	fi;                                                             \
	if [ "$(cpu)" = "notrap" ]; then                                \
		fpu=-notrap; cpu=000;                                   \
	else                                                            \
		cpu=$(ixcpu);                                           \
	fi;                                                             \
	if [ "$(os)" = "pos" ]; then                                    \
		os=-pos;                                                \
	else                                                            \
		pos=;                                                   \
	fi;                                                             \
	if [ "$(os)" = "morphos" ]; then                                \
		os=-morphos;                                            \
	fi;                                                             \
	if [ -f library/$(cpu)/$(fpu)/$(os)/ixemul.library ]; then      \
		$(INSTALL) library/$(cpu)/$(fpu)/$(os)/ixemul.library   \
			$(syslibsdir)/ixemul-$$cpu$$fpu$$os.library;    \
	else true; fi;                                                  \
	if [ -f library/$(cpu)/$(fpu)/$(os)/ixemul.trace ]; then        \
		$(INSTALL) library/$(cpu)/$(fpu)/$(os)/ixemul.trace     \
			$(syslibsdir)/ixemul-$$cpu$$fpu$$os.trace;      \
	else true; fi;                                                  \
	if [ -f ixnet/$(cpu)/$(os)/ixnet.library ]; then                \
		$(INSTALL) ixnet/$(cpu)/$(os)/ixnet.library             \
			$(syslibsdir)/ixnet-$$cpu$$os.library;          \
	else true; fi

install: installdirs $(ixemul-targets)
	@echo Installing library/68000/soft-float/amigaos/ixemul.library
	@if [ -f library/68000/soft-float/amigaos/ixemul.library ] ; then \
		$(INSTALL) library/68000/soft-float/amigaos/ixemul.library \
			$(syslibsdir)/ixemul.library; \
	else true; fi
	@echo Installing library/68000/soft-float/amigaos/ixemul.trace
	@if [ -f library/68000/soft-float/amigaos/ixemul.trace ] ; then \
		$(INSTALL) library/68000/soft-float/amigaos/ixemul.trace \
			$(syslibsdir)/ixemul.trace; \
	else true; fi
	@echo Installing ixnet/68000/amigaos/ixnet.library
	@if [ -f ixnet/68000/amigaos/ixnet.library ] ; then \
		$(INSTALL) ixnet/68000/amigaos/ixnet.library \
			$(syslibsdir)/ixnet.library; \
	else true; fi

	$(INSTALL) libsrc/crt0.o $(libdir)/crt0.o
	$(INSTALL) libsrc/bcrt0.o $(libdir)/bcrt0.o
	$(INSTALL) libsrc/gcrt0.o $(libdir)/gcrt0.o
	$(INSTALL) libsrc/mcrt0.o $(libdir)/mcrt0.o
	$(INSTALL) libsrc/rcrt0.o $(libdir)/rcrt0.o
	$(INSTALL) libsrc/scrt0.o $(libdir)/scrt0.o
	$(INSTALL) libsrc/lcrt0.o $(libdir)/lcrt0.o
	$(INSTALL) libsrc/libc.a $(libdir)/libc.a
	$(INSTALL) libsrc/libbc.a $(libdir)/libb/libc.a
	$(INSTALL) libsrc/libb32c.a $(libdir)/libb32/libm020/libc.a
	$(INSTALL) libsrc/libc_p.a $(libdir)/libc_p.a
	$(INSTALL_PROGRAM) utils/ixtrace $(bindir)/ixtrace
	$(INSTALL_PROGRAM) utils/ixprefs $(bindir)/ixprefs
	$(INSTALL_PROGRAM) utils/ixtimezone $(bindir)/ixtimezone
	$(INSTALL_PROGRAM) utils/ixrun $(bindir)/ixrun
	$(INSTALL_PROGRAM) utils/tzselect $(bindir)/tzselect
	$(INSTALL_PROGRAM) utils/ixstack $(bindir)/ixstack
	$(INSTALL_PROGRAM) utils/ipcs $(bindir)/ipcs
	$(INSTALL_PROGRAM) utils/ipcrm $(bindir)/ipcrm
	$(INSTALL) utils/ixpipe-handler $(sysldir)/ixpipe-handler
	(cd $(srcdir)/include && cp -pr . $(incdir))
	find $(incdir) -type d -name CVS -print | xargs rm -rf
	(cd $(srcdir)/man && cp -r man? $(mandir))
	find $(mandir) -type d -name CVS -print | xargs rm -rf
	mkdir -p $(etcdir)/zoneinfo
	cp $(srcdir)/utils/*.tab $(etcdir)/zoneinfo
	(cd utils/zoneinfo && cp -r . $(etcdir)/zoneinfo)


installdirs:    mkinstalldirs
	$(srcdir)/mkinstalldirs $(bindir) $(libdir) $(syslibsdir) \
		$(sysldir) $(incdir) $(etcdir) $(mandir) $(libdir)/libb \
		$(libdir)/libb32/libm020


clean:
	@-for i in $(DIRS) libsrc utils man; \
		do (cd $$i ; $(MAKE) $(FLAGS_TO_PASS) clean) ; done


clobber:
	@-for i in $(DIRS) libsrc utils man; \
		do (cd $$i ; $(MAKE) $(FLAGS_TO_PASS) clobber) ; done
	-rm -f Makefile config.* conftest.c gcc-ok


# Build source and binary distribution files.

ixdist-targets   := $(addprefix ixdist.,$(foreach o, $(OS), $(addsuffix .$(o),$(CPU-FPU-TYPES))))

$(ixdist-targets):
	@echo Distributing $@;                                          \
	mkdir -p gg/Sys/libs;                                           \
	ext=library;                                                    \
	if [ "$(fpu)" = "68881" ]; then                                 \
		fpulha=f; fpu=-fpu;                                     \
	else                                                            \
		fpulha=s; fpu=;                                         \
	fi;                                                             \
	if [ "$(cpu)" = "notrap" ]; then                                \
		cpu=000; fpulha=n; fpu=-notrap; ext=trace;              \
	else                                                            \
		cpu=$(ixcpu);                                           \
	fi;                                                             \
	if [ "$(os)" = "pos" ]; then                                    \
		os=-pos;                                                \
	else                                                            \
		os=;                                                    \
	fi;                                                             \
	mkdir -p gg/Sys/libs;                                           \
	cp -p library/$(cpu)/$(fpu)/$(os)/ixemul.$$ext                  \
		gg/Sys/libs/ixemul$$cpu$$fpu$$os.library;               \
	cp -p ixnet/$(cpu)/$(os)/ixnet.library                          \
		gg/Sys/libs/ixnet$$cpu$$os.library;                     \
	cp -p $(srcdir)/README gg/Sys/libs/README;                      \
	cp -p $(srcdir)/README.pOS gg/Sys/libs/README.pOS;              \
	(cd gg && lha -mraxeq a /distrib/ixemul-$$cpu$$fpulha$$os.lha Sys);\
	rm -f gg/Sys/libs/*

dist-prepare:
	#
	# First get rid of any leftovers from a previous "make dist"
	#
	rm -rf gg ixemul distrib
	mkdir -p distrib

dist:   dist-prepare $(ixdist-targets)
	#
	# Create ixemul-bin.lha, which contains utilities
	# that are generally useful to ixemul.library users.
	#
	mkdir -p gg/bin gg/Sys/l gg/Sys/devs
	cp -p utils/ixprefs gg/bin/ixprefs
	cp -p utils/ixtrace gg/bin/ixtrace
	cp -p utils/ixrun gg/bin/ixrun
	cp -p utils/ixstack gg/bin/ixstack
	cp -p utils/ipcs gg/bin/ipcs
	cp -p utils/ipcrm gg/bin/ipcrm
	cp -p utils/ixpipe-handler gg/Sys/l/ixpipe-handler
	cp -p $(srcdir)/utils/Mountlist gg/Sys/devs/MountList.ixpipe
	(cd gg && lha -mraxeq a /distrib/ixemul-bin.lha bin Sys)
	sleep 5
	rm -rf gg
	#
	# Create ixemul-tz.lha, which has timezone related pieces.
	#
	mkdir -p gg/etc gg/bin
	cp -p utils/ixtimezone gg/bin/ixtimezone
	cp -p utils/zic gg/bin/zic
	cp -p utils/tzselect gg/bin/tzselect
	cp -pr utils/zoneinfo gg/etc/zoneinfo
	cp $(srcdir)/utils/*.tab gg/etc/zoneinfo
	(cd gg && lha -mraxeq a /distrib/ixemul-tz.lha bin etc)
	sleep 5
	rm -rf gg
	#
	# Create ixemul-sdk.lha, which contains the files that
	# programmers need to build applications that use ixemul.library.
	#
	mkdir -p gg/lib/libb
	mkdir -p gg/lib/libb32/libm020
	cp -p libsrc/crt0.o gg/lib/crt0.o && strip -g gg/lib/crt0.o
	cp -p libsrc/bcrt0.o gg/lib/bcrt0.o && strip -g gg/lib/bcrt0.o
	cp -p libsrc/gcrt0.o gg/lib/gcrt0.o && strip -g gg/lib/gcrt0.o
	cp -p libsrc/mcrt0.o gg/lib/mcrt0.o && strip -g gg/lib/mcrt0.o
	cp -p libsrc/rcrt0.o gg/lib/rcrt0.o && strip -g gg/lib/rcrt0.o
	cp -p libsrc/lcrt0.o gg/lib/lcrt0.o && strip -g gg/lib/lcrt0.o
	cp -p libsrc/scrt0.o gg/lib/scrt0.o && strip -g gg/lib/scrt0.o
	cp -p libsrc/libc.a gg/lib/libc.a && strip -g gg/lib/libc.a
	cp -p libsrc/libbc.a gg/lib/libb/libc.a && strip -g gg/lib/libb/libc.a
	cp -p libsrc/libb32c.a gg/lib/libb32/libm020/libc.a && \
	  strip -g gg/lib/libb32/libm020/libc.a
	cp -p libsrc/libc_p.a gg/lib/libc_p.a && strip -g gg/lib/libc_p.a
	cp -pr $(srcdir)/include gg/include
	mkdir gg/man
	cp -pr man/cat? gg/man
	find gg -name CVS -print | xargs rm -rf
	(cd gg && lha -mraxeq a /distrib/ixemul-sdk.lha lib include man)
	sleep 5
	rm -rf gg
	#
	# Create ixemul-doc.lha, which contains various documentation
	# files.
	#
	mkdir ixemul
	cp -p $(srcdir)/COPYING ixemul/COPYING
	cp -p $(srcdir)/COPYING.LIB ixemul/COPYING.LIB
	cp -p $(srcdir)/INSTALL ixemul/INSTALL
	cp -p $(srcdir)/NEWS ixemul/NEWS
	cp -p $(srcdir)/BUGS ixemul/BUGS
	cp -p $(srcdir)/TODO ixemul/TODO
	cp -p $(srcdir)/README ixemul/README
	cp -p $(srcdir)/README.pOS ixemul/README.pOS
	cp -p $(srcdir)/utils/ixprefs.guide ixemul/ixprefs.guide
	cp -p $(srcdir)/utils/ixprefs.guide.info ixemul/ixprefs.guide.info
	lha -mraxeq a distrib/ixemul-doc.lha ixemul
	rm -rf ixemul
	#
	# Create ixemul-src.tgz, which contains the source code.
	# Do it in a temporary location so we don't have to
	# know where the build dir is, or translate between
	# Unix & AmigaOS forms.  We also delete any CVS dirs
	# at this time.
	#
	rm -rf ixemul-src.tgz ixemul
	cp -pr $(srcdir) ixemul
	find ixemul -name CVS -print | xargs rm -rf
	lha -mraxeq a distrib/ixemul-src.lha ixemul
	rm -rf ixemul
	#
	# Create ixemul-000t-<os>.lha
	#
	mkdir -p gg/Sys/libs
	cp -p library/68000/soft-float/amigaos/ixemul.trace gg/Sys/libs/ixemul000.trace
	cp -p $(srcdir)/README gg/Sys/libs/README
	cp -p $(srcdir)/README.pOS gg/Sys/libs/README.pOS
	(cd gg && lha -mraxeq a /distrib/ixemul-000t.lha Sys)
	rm -f gg/Sys/libs/*
	cp -p library/68000/soft-float/pos/ixemul.trace gg/Sys/libs/ixemul000-pos.trace
	cp -p $(srcdir)/README gg/Sys/libs/README
	cp -p $(srcdir)/README.pOS gg/Sys/libs/README.pOS
	(cd gg && lha -mraxeq a /distrib/ixemul-000t-pos.lha Sys)
	rm -f gg/Sys/libs/*
	sleep 5
	rm -rf gg
