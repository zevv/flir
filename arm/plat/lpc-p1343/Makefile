
ifdef PLAT_LPC_P1343

SRC	+= $(wildcard plat/lpc-p1343/*.c)

CFLAGS += -Iplat/lpc-p1343
LINTFLAGS += -iplat/lpc-p1343

LDFLAGS += -Tplat/lpc-p1343/rom.lds -lgcc

all: $(BIN)

ROM := $(BIN).rom

arch/lpc/lpcsum:
	gcc arch/lpc/lpcsum.c -o arch/lpc/lpcsum

$(ROM): $(BIN) arch/lpc/lpcsum
	$(E) $(OBJCOPY) -O binary $(BIN) $(BIN).rom 
	$(E) gcc arch/lpc/lpcsum.c -o arch/lpc/lpcsum
	$(E) arch/lpc/lpcsum $(ROM)

install: $(ROM)
	$(E) lpc21isp -bin $(ROM) /dev/ttyUSB0 115200 14000
	$(E) rm -f $(ROM)
	
USBDIR = $(shell grep -l "NXP LPC13XX IFLASH" /sys/bus/usb/devices/*/product | cut -d / -f 6)
USBDEV = /dev/$(shell ls /sys/bus/usb/devices/$(USBDIR)/*/*/*/*/block/)

usb: $(ROM)
	$(E) test ! -z $(USBDIR) || echo "NXP not found"
	$(E) test -b $(USBDEV) || echo "$(USBDEV) not found"
	$(E) mkdir -p mnt
	$(E) sudo mount $(USBDEV) mnt
	$(E) test -r mnt/firmware.bin
	$(E) sudo dd conv=nocreat,notrunc if=$(ROM) of=mnt/firmware.bin
	$(E) sudo umount mnt
	$(E) sudo rmdir mnt


endif
