#include "utils.h"


//https://www.cpcwiki.eu/forum/amstrad-cpc-hardware/z80-cpu-nmos-or-cmos/
//https://worldofspectrum.org/z88forever/dn327/z80undoc.htm
bool detectNMOS() __naked __sdcccall(1)
{
	__asm
		di
		ld   a, #0xff
		out  (0x99),a
		ld   a, #0x3f|0x40
		out  (0x99),a

		ld   c, #0x98
		.db 0xED,0x71				; = OUT (C),0

		xor  a						; Wait VRAM access timing
		nop
		nop
		in   a,(c)					; Read written value (0:NMOS 255:CMOS)

		ei
		inc  a
		ret							; Returns A = [0:CMOS 1:NMOS]
	__endasm;
}
