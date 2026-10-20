#include <stdint.h>
#include <stdbool.h>
#include "utils.h"
#include "conio.h"
#include "msx_const.h"


// V9990 (GFX9000 and others, also inside the MSXimus Z) at I/O ports 60h-6Fh.
// Port 64h selects a register (bits 7-6 set: no auto-increment on write and
// read) and port 63h writes or reads it. R#15 (back drop color, bits 5-0) is
// written with two patterns and read back, then its value is restored.
bool detectV9990()
{
	ASM_DI;
	outportb(0x64, 0xc0 | 15);
	uint8_t backdrop = inportb(0x63);
	outportb(0x63, 0x15);
	bool found = ((inportb(0x63) & 0x3f) == 0x15);
	outportb(0x63, 0x2a);
	found = found && ((inportb(0x63) & 0x3f) == 0x2a);
	outportb(0x63, backdrop);
	ASM_EI;
	return found;
}
