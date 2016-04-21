
ifdef ARCH_STM32

CROSS 	:= arm-none-eabi-
CFLAGS  += -mthumb -ffreestanding -nostartfiles -mcpu=cortex-m3
LDFLAGS += -mthumb -ffreestanding -nostartfiles -mcpu=cortex-m3

SRC		+= arch/stm32/arch.c

CFLAGS		+= -DARCH=\"stm32\"
LINTFLAGS	+= -dARCH=\"stm32\"

SRC		+= $(wildcard arch/stm32/STM32F1xx_HAL_Driver/Src/*c)
CFLAGS		+= -Iarch/stm32
CFLAGS		+= -Iarch/stm32/STM32F1xx_HAL_Driver/Inc
CFLAGS		+= -Iarch/stm32/CMSIS/Device/ST/STM32F1xx/Include/
CFLAGS		+= -Iarch/stm32/CMSIS/Include/

LINTFLAGS += -iarch/stm32/lint
LINTFLAGS += '+libm(arch\stm32\*.c)'
LINTFLAGS += -iarch/stm32
LINTFLAGS += -iarch/stm32/STM32F1xx_HAL_Driver/Inc
LINTFLAGS += -iarch/stm32/CMSIS/Device/ST/STM32F1xx/Include/
LINTFLAGS += -iarch/stm32/CMSIS/Include/

endif
