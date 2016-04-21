
-include .config

BIN	= bin/$(subst ",,$(NAME))
OBJS    = $(subst .c,.o, $(SRC))
DEPS    = $(subst .c,.d, $(SRC))

include arch/Makefile
include plat/Makefile
include drv/Makefile
include bios/Makefile
include lib/Makefile
include app/Makefile
include lint/Makefile

#
# Build options
#

VERSION_MAJ     := 2
VERSION_MIN     := 0
VERSION_DEV     := 1
VERSION_SVN     := 0
VERSION_REV     := $(subst M,,$(VERSION_SVN))

CFLAGS	+= -std=c99 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE
CFLAGS	+= -I.
CFLAGS  += $(BUILD_OPTIMIZATION)
CFLAGS  += -Wall -Wextra -Werror -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS  += -ggdb
CFLAGS	+= -DVERSION_SVN=\"$(VERSION_SVN)\"
CFLAGS	+= -DVERSION_MIN=$(VERSION_MIN)
CFLAGS	+= -DVERSION_MAJ=$(VERSION_MAJ)
CFLAGS	+= -DVERSION_DEV=$(VERSION_DEV)
CFLAGS	+= -DVERSION_REV=$(VERSION_REV)
CFLAGS	+= -DBUILD_USER=\"$(USER)\" 
CFLAGS	+= -DBUILD_HOSTNAME=\"$(shell hostname)\"

SRC	+= $(SRC-y)
CFLAGS	+= $(CFLAGS-y)
LDFLAGS	+= $(LDFLAGS-y)
LIBS	+= $(LIBS-y)


LDFLAGS += -ggdb

ifdef BUILD_GC_SECTIONS
CFLAGS  += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections
endif

ifdef BUILD_SANITIZE
CFLAGS  += -fsanitize=address
LDFLAGS += -fsanitize=address
endif

ifdef BUILD_COVERAGE
CFLAGS  += -fprofile-arcs -ftest-coverage
LIBS    += -lgcov
endif

ifdef BUILD_FLTO
CFLAGS  += -flto -O $(BUILD_OPTIMIZATION)
LDFLAGS += -flto -O $(BUILD_OPTIMIZATION)
endif

ifdef BUILD_CCACHE
CCACHE	= ccache
endif

ifdef BUILD_VERBOSE
E	=
P	= @true
else
E	= @
P	= @echo
MAKE	= make -s
endif


#
# Tools
#

HOSTCC	= gcc
CC 	= $(CROSS)gcc
OBJCOPY	= $(CROSS)objcopy
LD 	= $(CROSS)gcc
SIZE 	= $(CROSS)size


#
# Rules
# 

ifdef HAVE_DOT_CONFIG
all: $(BIN)
else
all: help
endif

menuconfig: mconf/mconf
	$(E) CONFIG_="" KCONFIG_AUTOHEADER=config.h mconf/mconf config.in

oldconfig: mconf/conf
	$(E) CONFIG_="" KCONFIG_AUTOHEADER=config.h mconf/conf --oldconfig config.in

config.h: .config
	$(E) $(MAKE) oldconfig

$(BIN):	config.h $(OBJS)
	$(P) " LD  $@"
	$(E) mkdir -p bin
	$(E) $(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(E) $(SIZE) $(BIN)

.c.o:
	$(P) " CC  $@"
	$(E) $(CCACHE) $(CC) $(CFLAGS) -MMD -c $< -o $@

clean:
	$(P) " CLEAN"
	$(E) find . -name '*.[od]' -delete
	$(E) find . -name '*.gcno' -delete
	$(E) find . -name '*.gcda' -delete
	$(E) rm -f $(OBJS) $(DEPS) $(BIN) core cscope.out bin/* 
	$(E) rm -f lint/gcc-include-path.lnt lint/lint_cmac.h lint/lint_cppmac.h lint/size-options.lnt
	$(E) rm -rf lcov/

lint:
	$(E) cd lint && $(MAKE) -f /opt/flint/supp/lnt/co-gcc.mak CFLAGS="$(CFLAGS)" GCC_BIN="$(CC)"
	$(E) #lint $(LINTFLAGS-y) $(LINTFLAGS) $(sort $(SRC))

lcov:
	$(E) mkdir -p lcov
	$(E) lcov -q -d . -c -o lcov/$(NAME).info
	$(E) genhtml -q -o lcov lcov/$(NAME).info

help:
	@echo ""
	@echo "No configuration availabe. Run 'make menuconfig' to configure build target"
	@echo ""

.PHONY: lint lcov

-include $(DEPS) 
include mconf/Makefile

