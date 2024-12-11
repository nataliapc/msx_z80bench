#include <stdbool.h>
#include "utils.h"


// https://www.msx.org/forum/msx-talk/development/uncovering-the-r800
bool detectZ280() __naked __sdcccall(0)
{
	__asm
		ld   l,#1
		ld   a,#0x40
		.db  0xcb, 0x37		; TSET A
		ret  p
		;jp m,cpu_z80
		; or
		; jp p,z280
		dec  l
		ret
	__endasm;
}
