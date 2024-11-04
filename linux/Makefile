
BIN   	= flir
SRC 	= flir.c golomb.c

PKG := sdl2 hidapi-libusb SDL2_ttf
CFLAGS  += -Wall -O0 -g `pkg-config --cflags $(PKG)`
LDFLAGS += -g `pkg-config --libs $(PKG)` -lm

CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address

CROSS	=
OBJS    = $(subst .c,.o, $(SRC))
CC 	= $(CROSS)gcc
LD 	= $(CROSS)gcc

.c.o:
	$(CC) $(CFLAGS) -c $<

$(BIN):	$(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDFLAGS)

clean:	
	rm -f $(OBJS) $(BIN) core
	
