
ifdef ARCH_LPC

CROSS 	:= arm-none-eabi-
CFLAGS  += -mthumb -ffreestanding -nostartfiles -mcpu=cortex-m3
LDFLAGS += -mthumb -ffreestanding -nostartfiles -mcpu=cortex-m3

SRC	+= arch/lpc/arch.c
SRC	+= $(wildcard arch/lpc/src/*.c)

CFLAGS	+= -DCHIP_LPC1343 -DCORE_M3

CFLAGS	+= -DARCH=\"lpc\"
LINTFLAGS += -dARCH=\"lpc\"

CFLAGS	+= -Iarch/lpc
CFLAGS	+= -Iarch/lpc/inc

LINTFLAGS += -iarch/lpc/lint
LINTFLAGS += '+libm(arch\lpc\*.c)'
LINTFLAGS += -iarch/lpc

endif
