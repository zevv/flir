#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "hidapi.h"

uint8_t img[60][80] = { 0 };

int main(int argc, char* argv[])
{
	int res;
	unsigned char buf[65];
	#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	hid_device *handle;
	int i;

	handle = hid_open(0x1234, 0x5678, NULL);

	for(;;) {

		buf[0] = 1;

		res = hid_read(handle, buf, 64);
		if (res < 0) {
			printf("Unable to read()\n");
			exit(1);
		}

		uint8_t line = buf[0];
		if(line < 60) {
			memcpy(img[line], buf+1, 63);
		}

		if(line == 59) {
			fprintf(stderr, "Frame\n");

			uint8_t vmin = 255;
			uint8_t vmax = 0;
			int y, x;

			for(y=0; y<60; y++) {
				for(x=0; x<63; x++) {
					uint8_t v = img[y][x];
					if(v < vmin) vmin = v;
					if(v > vmax) vmax = v;
				}
			}

			int dv = vmax - vmin;

			if(dv != 0) {
			
				for(y=59; y>=0; y--) {
					for(x=62; x>=0; x--) {
						uint8_t v = 255 * (img[y][x] - vmin) / dv;
						putchar(v);
					}
				}
			}

			fflush(stdout);
		}

	}

	return 0;
}
