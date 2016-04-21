
ifdef PLAT_BMS

SRC += $(wildcard plat/bms/*.c)

CFLAGS += -Iplat/bms
LINTFLAGS += -iplat/bms

JLINK	:= jlink

CFLAGS  += -DSTM32F10X_MD

LDFLAGS += -Tplat/bms/rom.lds -lgcc

all: $(BIN)

RESET_VECTOR = $(shell arm-none-eabi-nm $(BIN) | grep _entry_rom | awk '{print $$1}')

install: $(BIN)
	@arm-none-eabi-objcopy -O binary $(BIN) $(BIN).bin
	@echo > /tmp/jlink.cmd
	@echo exec device=stm32f105rc >> /tmp/jlink.cmd
	@echo r >> /tmp/jlink.cmd
	@echo h >> /tmp/jlink.cmd
	@echo loadbin $(BIN).bin,0 >> /tmp/jlink.cmd
	@echo setpc 0x$(RESET_VECTOR) >> /tmp/jlink.cmd
	@echo go >> /tmp/jlink.cmd
	@echo q >> /tmp/jlink.cmd
	@$(JLINK) /tmp/jlink.cmd || true
	@rm -f /tmp/jlink.cmd JLink.log Default.ini

endif
