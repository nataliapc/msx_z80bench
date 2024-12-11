#include <stdbool.h>
#include "utils.h"


// https://www.msx.org/forum/msx-talk/development/uncovering-the-r800
bool detectR800() __naked __sdcccall(0)
{
	__asm
		ld   l, #0
		xor  a
		ld   c, a
		inc  a
		.db  0xed, 0xc9		; MULUB A,C
		ret  nz
		inc  l
		ret
	__endasm;
}
