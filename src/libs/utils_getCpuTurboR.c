#include "utils.h"
#include "msx_const.h"


uint8_t getCpuTurboR() __naked __sdcccall(1)
{
	__asm
		push hl
		call _detectTurboR
		xor  a
		cp   l
		pop  hl
		ret  z
		push ix
		ld   ix, #0x183			; GETCPU
		BIOSCALL
		pop  ix
		ret						; Returns A = 0:Z80 1:R800(ROM) 2:R800(DRAM)
	__endasm;
}
