
ifdef ARCH_WIN32

CROSS	:= i686-w64-mingw32-

SRC	+= arch/win32/arch.c
SRC	+= arch/win32/mainloop.c

CFLAGS	+= -DARCH=\"win32\"

LINTFLAGS += -iarch/win32/lint
LINTFLAGS += '+libm(arch\win32\*.c)'

CFLAGS	+= -fno-strict-aliasing
CFLAGS	+= -gdwarf-2
LDFLAGS	+= -gdwarf-2


LIBS	+= -lws2_32

endif
