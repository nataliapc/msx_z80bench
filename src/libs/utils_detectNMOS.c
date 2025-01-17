#include "utils.h"


//https://www.malinov.com/sergeys-blog/z80-type-detection.html
//https://github.com/skiselev/z80-tests
//https://www.cpcwiki.eu/forum/amstrad-cpc-hardware/z80-cpu-nmos-or-cmos/
//https://worldofspectrum.org/z88forever/dn327/z80undoc.htm
bool detectNMOS() __naked __sdcccall(1)
{
	__asm
		ld   a, #0xff
		di
		out  (0x99),a
		ld   a, #0x3f|0x40
		out  (0x99),a

		ld   c, #0x98
		.db 0xED,0x71				; = OUT (C),0

		ex (sp),hl					; Dummy instructions (Wait VRAM access timing)
		ex (sp),hl
		ei
		in   a,(c)					; Read written value (0x00:NMOS 0xFF:CMOS)

		or   a
		jr   nz, .notNMOS
		inc  a
		ret							; Returns A = 1 [NMOS]
	.notNMOS:
		xor  a
		ret							; Returns A = 0 [CMOS]
	__endasm;
}
