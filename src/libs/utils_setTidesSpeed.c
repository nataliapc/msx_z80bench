#include "utils.h"
#include "msx_const.h"


// ========================================================
void setTidesSpeed(uint8_t speed) __naked __z88dk_fastcall
{
	speed;
	__asm
		di

		ld	 c, #0xb4			; [$b4] select block 2
		ld	 a, #13
		out	 (c), a
		inc	 c					; [$b5]
		ld	 a, #2
		out	 (c), a

		dec	 c					; [$b4] select register 14
		ld	 a, #14
		out	 (c), a

		inc	 c					; [$b5] set Tides-Rider speed
		ld	 a, l
		and  #0b00001111
		out	 (c), a

		ei
		ret
	__endasm;
}