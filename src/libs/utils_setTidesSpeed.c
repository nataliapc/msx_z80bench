#include "utils.h"
#include "msx_const.h"


// ========================================================
void setTidesSpeed(uint8_t speed) __naked __z88dk_fastcall
{
	speed;
	__asm
		di						; Param L = speed [0-4]
		call selectRegister14block2
		ld	 a, l
		or   #0b10000000
		out	 (c), a

		ei
		ret

	selectRegister14block2:
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
		ret
	__endasm;
}

// ========================================================
bool detectTidesRider() __naked __sdcccall(1)
{
	__asm
		di
		call selectRegister14block2
		in	 a, (c)
		ei

		cp   #255
		jr   nz, .isTidesRider
		inc  a
		ret						; Return A = false (0)
	.isTidesRider:
		ld   a, #1
		ret						; Return A = true (1)
	__endasm;
}