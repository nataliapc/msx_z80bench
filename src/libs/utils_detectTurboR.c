#include "utils.h"


bool detectTurboR() __naked __z88dk_fastcall
{
	__asm
		ld   hl, #0x180			; CHGCPU
		call _getRomByte
		cp	 #0xC3				; Check if CHGCPU is available
		ld   l, #0				; Return L = false
		ret  nz
		inc  l					; Return L = true
		ret
	__endasm;
}
