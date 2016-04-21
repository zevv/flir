#ifndef lut_h
#define lut_h

#include <stdint.h>
#include "bios/bios.h"

typedef uint8_t lut_val;
typedef int8_t lut_index;

struct lut {
	lut_index rows;
	lut_index cols;
	uint8_t *data;
};


struct lut_TI_C {
	temperature T_min, T_max;
	current I_min, I_max;
	charge C_min, C_max;
	struct lut lut;
};

lut_val lut_get(const struct lut *lut, lut_index row, lut_index col);
charge lut_get_TI_C(const struct lut_TI_C *lut, temperature T_row, current I_col);


#endif
