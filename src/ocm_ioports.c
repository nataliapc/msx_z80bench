/*
	Copyright (c) 2024 Natalia Pujol Cremades
	info@abitwitches.com

	See LICENSE file.
*/
#include "ocm_ioports.h"


bool ocm_detectDevice(uint8_t devId) __naked __z88dk_fastcall
{
	devId;								// L = Param devId
	__asm
		in   a, (0x40)					; backup current manufacturer/device
		cpl
		push af

		call .detectExtIODevice
		ld   l, a						; Returns L = 0:false 1:true

	.odv_end:
		pop  af							; restore original manufacturer/device
		out  (0x40), a
		ret

	.detectExtIODevice:					; IN: L=DeviceID | OUT: A=0:false 1:true
		ld   a, l
		out  (0x40), a					; out the manufacturer/device code to I/O port 0x40
		in   a, (0x40)					; read the value you have just written
		cpl								; complement all bits of the value
		cp   l							; if it does not match the value you originally wrote,

		jr   z, .detected
		xor  a							; it does not exists on expanded I/O ports
		ret
	.detected:
		ld   a, #1						; does exists
		ret
	__endasm;
}

bool ocm_sendSmartCmd(uint8_t cmd) __naked __z88dk_fastcall
{
	cmd;								// L = Param cmd
	__asm
		in a, (0x40)					; backup current manufacturer/device
		cpl
		push af

		ld   b, l						; Store Param cmd
		ld   l, #0xd4					; DEVID_OCMPLD
		call .detectExtIODevice

		ld   l, a
		jr   nz, .odv_end				; Return L = 0:fail
										; L = 1:success

		ld   a, b						; Restore Param cmd
		out  (0x41), a					; send smart command to OCM_SMARTCMD_PORT

		in   a, (0x41)					; Read the value you have just written for testing
		cpl
		cp   b							; Check the match of the original value
		jr   z, .odv_end
		dec  l							; Return L = 0:fail

		jr .odv_end
	__endasm;
}