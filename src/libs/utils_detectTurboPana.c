#include <stdbool.h>
#include "utils.h"


//https://map.grauw.nl/resources/msx_io_ports.php#expanded_io
bool detectTurboPana() __naked __z88dk_fastcall
{
	__asm
		in   a,(0x40)		; back up the value of I/O port 40h
		cpl
		push af

		ld   a,#8
		out  (0x40),a		; out the manufacturer code 8 (Panasonic) to I/O port 40h
		
		ld   l,#0			; return L = false
		in   a,(0x40)		; read the value you have just written
		cpl					; complement all bits of the value
		cp   #8				; if it does not match the value you originally wrote,
		jr   nz,NoTurbo		; it does not have the Panasonic expanded I/O ports
		
		in   a,(0x41)
		and  #0b00000100	; is turbo mode available?
		jr   nz,NoTurbo
		inc  l				; return L = true
	NoTurbo:
		pop  af				; restore the value of I/O port 40h
		out  (0x40),a
		ret
	__endasm;
}