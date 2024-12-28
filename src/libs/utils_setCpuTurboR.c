#include "utils.h"
#include "msx_const.h"


void setCpuTurboR(uint8_t mode) __naked __z88dk_fastcall
{
	mode;
	__asm
		push hl
		call _detectTurboR
		xor  a
		cp   l
		pop  hl
		ret  z
		ld	 a, l				; Param L = mode
		or   #0x80				; Update de CPU led
		ld   ix, #0x180			; CHGCPU
		JP_BIOSCALL
	__endasm;
}
