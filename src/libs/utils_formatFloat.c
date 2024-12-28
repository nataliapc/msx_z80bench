#include <math.h>
#include "utils.h"


char *formatFloat(float value, char *txt, int8_t decimals)
{
	uint8_t digit;
	uint16_t decenes = 10000;
	char *p = txt;

	while (value < decenes) {
		decenes /= 10;
	}

	while (decenes) {
		digit = (uint8_t)floorf(value / decenes);
		*p++ = '0' + digit;
		value -= digit * decenes;
		decenes /= 10;
	}

	if (txt == p) *p++ = '0';
	if (decimals) *p++ = '.';

	while (decimals) {
		value *= 10.f;
		digit = (uint8_t)floorf(value);
		*p++ = '0' + digit;
		value -= digit;
		decimals--;
	}
	*p = '\0';

	return p;
}
