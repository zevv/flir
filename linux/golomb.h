#ifndef golomb_h
#define golomb_h

struct bitwriter {
	uint8_t v;
	uint8_t m;
	uint8_t *p;
	uint8_t *pend;
};


struct bitreader {
	uint8_t v;
	uint8_t m;
	uint8_t *p;
	uint8_t *pend;
};


void bw_init(struct bitwriter *bw, void *buf, size_t len);
void bw_flush(struct bitwriter *bw);
void bw_add(struct bitwriter *bw, uint8_t bit);
void bw_golomb(struct bitwriter *bw, int32_t v, uint8_t k);

void br_init(struct bitreader *br, void *buf, size_t len);
int br_get(struct bitreader *br, uint8_t *bit);
int br_golomb(struct bitreader *br, int32_t *v, uint8_t k);

#endif

