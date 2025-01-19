#include "utils.h"
#include "msx_const.h"


void setNTSC(bool enabled) __naked __sdcccall(1)
{
	enabled;
	__asm
		push ix
		and  #1
		xor  #1
		add  a
		ld   l, a
		ld   a, (RG9SAV)
		and  #0b11111101
		or   l
		ld   (RG9SAV), a
		ld   b, a
		ld   c, #0x09		; Register #09
		ld   ix, #WRTVDP
		BIOSCALL
		pop  ix
		ret
	__endasm;
}
