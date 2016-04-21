
ifdef PLAT_BMS_AVR

SRC	+= plat/bms-avr/plat.c
SRC-$(DRV_UART_BITBANG) = plat/bms-avr/uart-bitbang.c

CFLAGS	+= -Iplat/bms-avr
LINTFLAGS += -iplat/bms-avr

CROSS 	= avr-

CFLAGS	+= -DF_CPU=16000000
CFLAGS  += -mmcu=atmega32
LDFLAGS += -mmcu=atmega32

endif
