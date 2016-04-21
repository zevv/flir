
#include "lib/atoi.h"

int32_t a_to_s32(const char *s)
{
	int32_t base = 10;
	int8_t neg = 0;
	char c = *s;

	s++;

	if(c == '-') {
		neg = 1;
		c = *s;
		s++;
	}

	if((c == '0') && ((*s == 'x') || (*s == 'X'))) {
		c = s[1];
		s++;
		s++;
		base = 16;
	} else if((c == '0') && ((*s == 'b') || (*s == 'B'))) {
		c = s[1];
		s++;
		s++;
		base = 2;
	} else {
		/* misra */
	}

	int32_t acc = 0;

	while(c != '\0') {

		int32_t v = (int32_t)c;


		if((c >= '0') && (c <= '9')) {
			v -= (int32_t)'0';
		} else if((c >= 'A') && (c <= 'Z')) {
			v -= 'A' - 10;
		} else if((c >= 'a') && (c <= 'z')) {
			v -= 'a' - 10;
		} else {
			break;
		}

		if(v >= base) {
			break; /*lint !e9011 */
		}
	
		acc *= base;
		acc += v;
		c = *s;
		s ++;
	}

	if(neg == 1) {
		acc = -acc;
	}

	return acc;
}

/*
 * End
 */
