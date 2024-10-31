#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "golomb.h"


void bw_init(struct bitwriter *bw, void *buf, size_t len)
{
	bw->p = buf;
	bw->pend = buf + len;
	bw->m = 0x80;
	bw->v = 0;
}


void bw_flush(struct bitwriter *bw)
{
	if(bw->p < bw->pend) {
		*bw->p = bw->v;
	}
}


void bw_add(struct bitwriter *bw, uint8_t bit)
{
	bw->v |= bit ? bw->m : 0;
	bw->m >>= 1;

	if(bw->m == 0x00) {
		bw_flush(bw);
		bw->p ++;
		bw->v = 0;
		bw->m = 0x80;
	}
}


void bw_golomb(struct bitwriter *bw, int32_t val, uint8_t k)
{
	/* biject signed to unsgined */

	uint32_t v;
	if(val > 0) {
		v = val * 2 - 1;
	} else {
		v = -val * 2;
	}

	/* encode */

	v = v + (1<<k);
	uint8_t bits = 32 - __builtin_clz(v);
	uint32_t m = (1<<(bits-1));
	uint8_t i;
	for(i=k+1; i<bits; i++) {
		bw_add(bw, 0);
	}
	for(i=0; i<bits; i++) {
		bw_add(bw, !!(v & m));
		v <<= 1;
	}
}
	



void br_init(struct bitreader *br, void *buf, size_t len)
{
	br->p = buf;
	br->pend = buf + len;
	br->m = 0;
	br->v = *(br->p);
}


int br_get(struct bitreader *br, uint8_t *bit)
{
	if(br->p >= br->pend) {
		return 0;
	}

	if(br->m == 0) {
		br->v = *br->p;
		br->m = 0x80;
		br->p ++;
	}

	*bit = !!(br->v & br->m);

	br->m >>= 1;
	return 1;
}


int br_golomb(struct bitreader *br, int32_t *val, uint8_t k)
{
	uint8_t bits = k+1;
	uint8_t bit;
	int r;

	*val = 0;

	do {
		r = br_get(br, &bit);
		if(r == 0) return 0;
		if(bit == 0) bits ++;
	} while(bit == 0);

	uint8_t i;
	uint32_t v = 0;
	for(i=0; i<bits; i++) {
		if(i > 0) {
			r = br_get(br, &bit);
			if(r == 0) return 0;
		}
		v <<= 1;
		v |= bit;
	}

	v -= (1<<k);

	/* biject unsigned to signed */

	if(v & 1) {
		*val = (v+1) >> 1;
	} else {
		*val = -(v >> 1);
	}
	return 1;
}

#ifdef UNIT_TEST

int main(int argc, char **argv)
{
	uint8_t buf[32] = { 0 };

	struct bitwriter bw;
	bw_init(&bw, buf, sizeof(buf));
	
	struct bitreader br;
	br_init(&br, buf, sizeof(buf));

	bw_golomb(&bw, 8000, 2);
	bw_golomb(&bw, 10, 2);
	bw_golomb(&bw, 10, 2);
	bw_golomb(&bw, -20, 2);
	bw_golomb(&bw, -10, 2);
	bw_golomb(&bw, 10, 2);
	bw_golomb(&bw, 1000, 2);
	bw_golomb(&bw, 0, 2);
	bw_flush(&bw);

	printf("\n");

	if(0) {
		int i;
		for(i=0; i<32; i++) {
			uint8_t bit;
			br_get(&br,& bit);
			printf("%d", bit);
		}
		printf("\n");
	}

	br_init(&br, buf, sizeof(buf));
	int i;
	for(i=0; i<10; i++) {
		int32_t v;
		br_golomb(&br, &v, 2);
		printf("%d\n", v);
	}

	uint8_t t[] = {
		0x00, 0x3d, 0xeb, 0x7d, 0x1c, 0xf4, 0xa2, 0x10, 0x49, 0x87,
		0x0e, 0x0c, 0x98, 0x20, 0x42, 0xb2, 0xf2, 0xf1, 0x1b, 0x29,
		0x09, 0x32, 0xd1, 0x78, 0x7a, 0x8c, 0xd4, 0x71, 0x0d, 0xdc,
		0x80, 0x0
	};

	br_init(&br, t, sizeof(t));
	if(0) {
		int i;
		for(i=0; i<32; i++) {
			uint8_t bit;
			br_get(&br,& bit);
			printf("%d", bit);
		}
		printf("\n");
	}

	printf("\n");
	for(i=0; i<30; i++) {
		int32_t v;
		br_golomb(&br, &v, 3);
		printf("%d\n", v);
	}

	return 0;
}

#endif

/*
 * End
 */
