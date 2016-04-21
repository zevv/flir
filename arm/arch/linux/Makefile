
ifdef ARCH_LINUX

SRC	+= arch/linux/arch.c
SRC	+= arch/linux/mainloop.c

LINTFLAGS += -iarch/linux/lint
LINTFLAGS += '+libm(arch\linux\*.c)'

CFLAGS	+= -DARCH=\"linux\"
LINTFLAGS += -dARCH=\"linux\"

#CFLAGS  += -m32
#LDFLAGS += -m32

CFLAGS	+= $(shell pkg-config --cflags sdl)
LDFLAGS	+= $(shell pkg-config --libs sdl)

CFLAGS	+= -fno-strict-aliasing
LDFLAGS += -lrt -lm

endif
