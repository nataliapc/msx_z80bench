#include <stdbool.h>
#include "utils.h"


// Zilog Z380 User's Manual (DC-8297-03): CB 37 = EX A,A'
// On the Z380 the opcode CB 37 exchanges A with A' and leaves the flags alone.
// The other CPUs modify A in place (Z80: SLL A, Z280: TSET A), so a marker
// stored in A' only shows up in A on a Z380.
// It must run before any other CPU test: the Z380 traps undefined opcodes
// (like R800 MULUB, ED C9) by jumping to address 0x0000.
bool detectZ380() __naked __sdcccall(1)
{
	__asm
		di
		ex   af,af				; Save the original alternate AF
		push af
		ld   a, #0x5a			; Marker for the alternate A
		ex   af,af

		ld   a, #0x40
		.db  0xcb, 0x37			; Z380: EX A,A(alt) | Z80: SLL A | Z280: TSET A
		ld   l, a

		ex   af,af				; Restore the original alternate AF
		pop  af
		ex   af,af
		ei

		ld   a, l
		cp   #0x5a				; Alternate A marker in A?
		ld   a, #1
		ret  z					; Returns A = 1 [Z380]
		xor  a
		ret						; Returns A = 0 [other CPU]
	__endasm;
}
