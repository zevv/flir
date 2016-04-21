
ifdef PLAT_LPC_P1343

SRC	+= $(wildcard plat/lpc-p1343/*.c)

CFLAGS += -Iplat/lpc-p1343
LINTFLAGS += -iplat/lpc-p1343

LDFLAGS += -Tplat/lpc-p1343/rom.lds -lgcc

all: $(BIN)

install: $(BIN)
	$(OBJCOPY) -O binary $(BIN) $(BIN).rom 
	lpc21isp -bin $(BIN).rom /dev/ttyUSB0 115200 14000
	rm -f $(BIN).rom

endif
