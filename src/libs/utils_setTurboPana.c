#include <stdbool.h>
#include "utils.h"


//https://map.grauw.nl/resources/msx_io_ports.php#expanded_io
bool setTurboPana(bool enabled) __naked __sdcccall(1)
{
	enabled;
	__asm
		and  #1				; A = Parameter enabled
		xor  #1
		ld   l, a

		in   a,(0x40)		; back up the value of I/O port 40h
		cpl
		push af

		ld   a,#8
		out  (0x40),a		; out the manufacturer code 8 (Panasonic) to I/O port 40h
		
		in   a,(0x40)		; read the value you have just written
		cpl					; complement all bits of the value
		cp   #8				; if it does not match the value you originally wrote,
		jr   nz,NoTurbo		; it does not have the Panasonic expanded I/O ports
		
		in   a,(0x41)
		bit  2,a			; is turbo mode available?
		jr   nz,NoTurbo

		res  0,a
		or   l				; apply Parameter enabled
		out  (0x41),a		; enable turbo

		ld   a,#1			; return true
		jr   End
	NoTurbo:
		xor  a				; return false
	End:
		pop  af				; restore the value of I/O port 40h
		out  (0x40),a
		ret
	__endasm;
}