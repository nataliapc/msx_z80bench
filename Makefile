.PHONY: clean test release

SDCC_VER := 4.2.0
DOCKER_IMG = nataliapc/sdcc:$(SDCC_VER)
DOCKER_RUN = docker run -i --rm -u $(shell id -u):$(shell id -g) -v .:/src -w /src $(DOCKER_IMG)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	COL_RED = \e[1;31m
	COL_YELLOW = \e[1;33m
	COL_ORANGE = \e[1;38:5:208m
	COL_BLUE = \e[1;34m
	COL_GRAY = \e[1;30m
	COL_WHITE = \e[1;37m
	COL_RESET = \e[0m
endif

AS = $(DOCKER_RUN) sdasz80
AR = $(DOCKER_RUN) sdar
CC = $(DOCKER_RUN) sdcc
HEX2BIN = hex2bin
MAKE = make -s --no-print-directory
EMUSCRIPTS = -script ./emulation/boot.tcl

ROOTDIR = .
INCDIR = $(ROOTDIR)/include
SRCDIR = $(ROOTDIR)/src
SRCLIB = $(SRCDIR)/libs
LIBDIR = $(ROOTDIR)/libs
OBJDIR = $(ROOTDIR)/out
DSKDIR = $(ROOTDIR)/dsk
EXTERNALS = $(ROOTDIR)/externals

ODIR_GUARD=@mkdir -p $(OBJDIR)
OLIB_GUARD=@mkdir -p $(LIBDIR)

LDFLAGS = -rc
CRT = $(OBJDIR)/crt0msx_msxdos_advanced.s.rel

DEFINES := -D_DOSLIB_
#DEBUG := -D_DEBUG_
FULLOPT :=  --max-allocs-per-node 200000
LDFLAGS = -rc
OPFLAGS = --std-sdcc2x --less-pedantic --opt-code-size -pragma-define:CRT_ENABLE_STDIO=0
WRFLAGS = --disable-warning 196 --disable-warning 84
CCFLAGS = --code-loc 0x0180 --data-loc 0 -mz80 --no-std-crt0 --out-fmt-ihx $(OPFLAGS) $(WRFLAGS) $(DEFINES) $(DEBUG)

LIBS = $(LIBDIR)/conio.lib $(LIBDIR)/dos.lib $(LIBDIR)/utils.lib
SRC =	heap.c \
		msx1_functions.c \
		ocm_ioports.c

PROGRAM = z80bench.com

all: $(OBJDIR)/$(PROGRAM) $(LIBS) release


$(LIBDIR)/conio.lib: $(EXTERNALS)/sdcc_msxconio/src/* $(EXTERNALS)/sdcc_msxconio/include/* $(EXTERNALS)/sdcc_msxconio/Makefile
	$(OLIB_GUARD)
	$(MAKE) -C $(EXTERNALS)/sdcc_msxconio all SDCC_VER=$(SDCC_VER) DEFINES=-DXXXXX
	@cp $(EXTERNALS)/sdcc_msxconio/include/conio.h $(INCDIR)
	@cp $(EXTERNALS)/sdcc_msxconio/include/conio_aux.h $(INCDIR)
	@cp $(EXTERNALS)/sdcc_msxconio/lib/conio.lib $(LIBDIR)

$(LIBDIR)/dos.lib: $(EXTERNALS)/sdcc_msxdos/src/* $(EXTERNALS)/sdcc_msxdos/include/* $(EXTERNALS)/sdcc_msxdos/Makefile
	$(OLIB_GUARD)
	$(MAKE) -C $(EXTERNALS)/sdcc_msxdos/ all SDCC_VER=$(SDCC_VER) DEFINES=-DDISABLE_CONIO
	@cp $(EXTERNALS)/sdcc_msxdos/include/dos.h $(INCDIR)
	@cp $(EXTERNALS)/sdcc_msxdos/lib/dos.lib $(LIBDIR)
#	@$(AR) -d $@ dos_putchar.c.rel ;

$(LIBDIR)/utils.lib: $(patsubst $(SRCLIB)/%, $(OBJDIR)/%.rel, $(wildcard $(SRCLIB)/utils_*))
	$(OLIB_GUARD)
	@echo "$(COL_WHITE)######## Compiling $@$(COL_RESET)"
	@$(LIB_GUARD)
	@$(AR) $(LDFLAGS) $@ $^ ;
	@$(AR) -d $@ utils_exit.c.rel ;

$(OBJDIR)/%.s.rel: $(SRCDIR)/%.s
	@echo "$(COL_BLUE)#### ASM $@$(COL_RESET)"
	@$(ODIR_GUARD)
	@$(AS) -go $@ $^ ;

$(OBJDIR)/%.c.rel: $(SRCDIR)/%.c
	@echo "$(COL_BLUE)#### CC $@$(COL_RESET)"
	@$(ODIR_GUARD)
	@$(CC) -I$(INCDIR) $(CCFLAGS) -c -o $@ $^ ;

$(OBJDIR)/%.c.rel: $(SRCLIB)/%.c
	@echo "$(COL_BLUE)#### CC $@$(COL_RESET)"
	@$(DIR_GUARD)
	@$(CC) $(CCFLAGS) $(FULLOPT) -I$(INCDIR) -c -o $@ $^ ;

$(OBJDIR)/%.s.rel: $(SRCLIB)/%.s
	@echo "$(COL_BLUE)#### ASM $@$(COL_RESET)"
	@$(DIR_GUARD)
	@$(AS) -go $@ $^ ;

$(OBJDIR)/$(PROGRAM): $(CRT) $(LIBS) $(addprefix $(OBJDIR)/,$(subst .c,.c.rel,$(SRC)))
	@echo "$(COL_YELLOW)######## Compiling $@$(COL_RESET)"
	@$(CC) $(CCFLAGS) -I$(INCDIR) -L$(LIBDIR) $^ $(subst $(OBJDIR),$(SRCDIR),$(ROOTDIR)/$(subst .com,.c,$@)) -o $(subst .com,.ihx,$@) ;
	@$(HEX2BIN) -e com $(subst .com,.ihx,$@)

release:
	@echo "$(COL_WHITE)**** Copying .COM file to $(DSKDIR)$(COL_RESET)"
	@cp $(OBJDIR)/$(PROGRAM) $(DSKDIR)


###################################################################################################

clean: cleanobj cleanlibs

cleanobj:
	@echo "$(COL_ORANGE)##  Cleaning obj$(COL_RESET)"
	@rm -f $(DSKDIR)/$(PROGRAM)
	@rm -f $(OBJDIR)/*

cleanlibs:
	@echo "$(COL_ORANGE)##  Cleaning libs$(COL_RESET)"
	@rm -f $(addprefix $(LIBDIR)/, $(LIBS))
	@$(MAKE) -C $(EXTERNALS)/sdcc_msxconio clean
	@touch $(EXTERNALS)/sdcc_msxconio/Makefile
	@$(MAKE) -C $(EXTERNALS)/sdcc_msxdos clean
	@touch $(EXTERNALS)/sdcc_msxdos/Makefile


###################################################################################################

test: all
#	openmsx -machine msx1_eu -ext Mitsubishi_ML-30DC_ML-30FD -ext debugdevice -diska $(DSKDIR) $(EMUSCRIPTS)
#	openmsx -machine msx1 -ext debugdevice -diska $(DSKDIR) $(EMUSCRIPTS)
#	openmsx -machine msx2_eu -ext debugdevice -diska $(DSKDIR) $(EMUSCRIPTS)
	openmsx -machine msx2plus -ext debugdevice -ext msxdos2 -diska $(DSKDIR) $(EMUSCRIPTS)
#	openmsx -machine Sony_HB-F1XD -ext debugdevice -diska $(DSKDIR) $(EMUSCRIPTS)
#	openmsx -machine turbor -ext debugdevice -diska $(DSKDIR) $(EMUSCRIPTS)
