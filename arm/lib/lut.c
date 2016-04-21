
#include "lut.h"


static lut_index lut_T_to_index(temperature T_in, temperature T_min, temperature T_max, lut_index rows)
{
	int64_t i = rows;
	temperature T = T_in - T_min;
	temperature T_span = T_max - T_min;
	i = i * T;
	i = i / T_span;
	return (lut_index)i;
}


static lut_index lut_I_to_index(current I_in, current I_min, current I_max, lut_index rows)
{
	int64_t i = rows;
	current I = I_in - I_min;
	current I_span = I_max - I_min;
	i = i * I;
	i = i / I_span;
	return (lut_index)i;
}


static charge lut_val_to_charge(lut_val v_in, charge C_min, charge C_max)
{
	charge C = C_min;
	charge dC = C_max - C_min;
	C += (charge)v_in * dC;
	C /= 255;
	return C;
}


lut_val lut_get(const struct lut *lut, lut_index row, lut_index col)
{
	lut_index y = row;
	lut_index x = col;

	if(y < 0) { y = 0; }
	if(y > (lut->rows - 1)) { y = lut->rows - 1; }

	if(x < 0) { x = 0; }
	if(x > (lut->cols - 1)) { x = lut->cols - 1; }

	lut_index index = (lut_index)(lut->cols * y) + x;
	lut_val v = lut->data[index];
	return v;
}


charge lut_get_TI_C(const struct lut_TI_C *lut, temperature T_row, current I_col)
{
	lut_index row = lut_T_to_index(T_row, lut->T_min, lut->T_max, lut->lut.rows);
	lut_index col = lut_I_to_index(I_col, lut->I_min, lut->I_max, lut->lut.cols);

	lut_val v = lut_get(&lut->lut, row, col);

	return lut_val_to_charge(v, lut->C_min, lut->C_max);
}



/*
 * End
 */
