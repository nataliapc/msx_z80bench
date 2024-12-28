#include "utils.h"


void click() __naked
{
	__asm
		ld   a, #(7<<1)|1
		out  (0xAB), a

		ld   b,#10
	click_loop:
		djnz click_loop

		and  #0b11111110
		out  (0xAB),a
		ret
	__endasm;
}
