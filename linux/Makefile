
BIN   	= flir
SRC 	= flir.c golomb.c

CFLAGS  += -Wall -O0 -g `pkg-config --cflags sdl2 hidapi-libusb`
LDFLAGS += -g `pkg-config --libs sdl2 hidapi-libusb` -lm

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
	
