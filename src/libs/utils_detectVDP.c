#include "utils.h"
#include "msx_const.h"


// https://map.grauw.nl/sources/vdp_detection.php
uint8_t detectVDP() __naked __sdcccall(1)
{
	__asm
		;
		; Detect VDP version
		;
		; a <- 0: TMS9918A, 1: V9938, 2: V9958, x: VDP ID
		; f <- z: TMS9918A, nz: other
		;
		call VDP_IsTMS9918A			; use a different way to detect TMS9918A
		ret  z
		ld   a, #1					; select s#1
		di
		out  (0x99),a
		ld   a,#15 + 0x80
		out  (0x99),a
		in   a,(0x99)				; read s#1
		and  #0b00111110			; get VDP ID
		rrca
		ex   af,af
		xor  a						; select s#0 as required by BIOS
		out  (0x99),a
		ld   a,#15 + 128
		ei
		out  (0x99),a
		ex   af,af
		ret  nz						; return VDP ID for V9958 or higher
		inc  a						; return 1 for V9938
		ret

		;
		; Test if the VDP is a TMS9918A.
		;
		; The VDP ID number was only introduced in the V9938, so we have to use a
		; different method to detect the TMS9918A. We wait for the vertical blanking
		; interrupt flag, and then quickly read status register 2 and expect bit 6
		; (VR, vertical retrace flag) to be set as well. The TMS9918A has only one
		; status register, so bit 6 (5S, 5th sprite flag) will return 0 in stead.
		;
		; f <- z: TMS9918A, nz: V99X8
		;
	VDP_IsTMS9918A:
		in   a,(0x99)					; read s#0, make sure interrupt flag (F) is reset
		di
	VDP_IsTMS9918A_Wait:
		in   a,(0x99)					; read s#0
		and  a							; wait until interrupt flag (F) is set
		jp   p,VDP_IsTMS9918A_Wait
		ld   a,#2						; select s#2 on V9938
		out  (0x99),a
		ld   a,#15 + 0x80				; (this mirrors to r#7 on TMS9918 VDPs)
		out  (0x99),a
		in   a,(0x99)					; read s#2 / s#0
		ex   af,af
		xor  a							; select s#0 as required by BIOS
		out  (0x99),a
		ld   a,#15 + 0x80
		out  (0x99),a
		ld   a,(0xF3E6)					; RG7SAV
		out  (0x99),a					; restore r#7 if it mirrored (small flash visible)
		ld   a,#7 + 0x80
		ei
		out  (0x99),a
		ex   af,af
		and  #0b01000000				; check if bit 6 was 0 (s#0 5S) or 1 (s#2 VR)
		ret
	__endasm;
}