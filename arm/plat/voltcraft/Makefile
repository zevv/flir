
ifdef PLAT_VOLTCRAFT

SRC	+= $(wildcard plat/voltcraft/*.c)

CFLAGS	+= -Iplat/voltcraft
LINTFLAGS += -iplat/voltcraft

CROSS 	= avr-

CFLAGS	+= -DF_CPU=16000000
CFLAGS  += -mmcu=atmega32
LDFLAGS += -mmcu=atmega32

endif
