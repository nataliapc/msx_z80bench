#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "dos.h"
#include "conio.h"
#include "msx_const.h"

#define intVecTabAd		0x8000		// Put the Interrupt Vector Table here
#define intVecHalfAd	0x80		// High memory address pointer
#define intRoutStart	0x8181		// Let interrupt routine start here
#define intRoutHalfAd	0x81		// Address high and low

#define MSX_CLOCK		3.579545	// Hz

uint16_t im2_counter;


// http://map.grauw.nl/articles/interrupts.php
void setCustomInterrupt() __naked
{
	__asm
		ei
		halt
		di						;No interrupts during switch

		ld hl,#0				;
		ld (_im2_counter),hl	;Counter = 0
		ld (_im2_counter+2),hl	;

		ld hl,#intVecTabAd		;Generate IVT here
		ld (hl),#intRoutHalfAd	;Use this as high and low address part
		ld d,h					;Copy destination pointer from
		ld e,l					; the source pointer
		inc de					;Destination 1 byte further
		ld bc,#128*2			;128 vectors, 1 byte extra for 256th
		ldir					;Generate table

		ld hl,#intRoutHere$		;Routine for IM 2
		ld de,#intRoutStart		;Put routine here
		ld bc,#intRoutEnd$-intRoutIM2$	;Length of the routine
		ldir					;Copy the routine

		ld a,#intVecHalfAd		;Use this as high address part
		ld i,a					;Set high address part

		im 2					;Switch to IM 2
		ei						;Enable interrupts

		xor  a
		ld   b, #10
	loop1$:
		ld   hl, #0xffff
	loop2$:
		dec  hl
		cp   h
		jp   nz, loop2$
		cp   l
		jp   nz, loop2$
		djnz loop1$
		;
		; 10 -> 212
		; 100 -> 2123
		; 1000 -> 21233
		; 10000 -> 212331
		; 213 -> 3,58 Mhz (50Hz)
		; 254 -> 3,58 Mhz (60Hz)
		; 169 -> 5,39 Mhz
		; 110 -> 8.29 Mhz
		;
		; OCM-PLD
		; 254 -> 3.58 Mhz
		; 169 -> 5.37 Mhz
		; 221 -> 4.10 Mhz
		; 202 -> 4.48 Mhz
		; 184 -> 4.90 Mhz
		; 165 -> 5.39 Mhz
		; 147 -> 6.10 Mhz
		; 129 -> 6.96 Mhz
		; 110 -> 8.04 Mhz

		di						;End test
		im 1					;Switch back to IM 1
		ei						;Now interrupts are permitted again
		ret

	intRoutHere$:				;Code is now here

	intRoutIM2$:		;[144]
		push hl					;Save registers that are modified
		push af

	cont$:
		in  a,(0x99)			;Read S#0
		bit 0, a				;Does INT originate from VDP?
		jp  z,notFromVDP$		;No -> go to exit

		ld  hl,#_im2_counter	;Nr. of interrupts counter
		inc (hl)				;Increase counter by one

	notFromVDP$:
		pop af					;Restore modified registers
		pop hl
		ei						;Interrupts are permitted again
		reti					;Return to main program
	intRoutEnd$:				;For Length of routine code	
	__endasm;
}


const char OUTPUT[] = { ' ', '0', '.', '0', '0', '\0' };

void getMhz(uint16_t ticks, uint8_t freqVDP)
{
	float secReference = 213 / 60.;			// tests seconds in a MSX1 at VDP50Hz
	float seconds = (float)ticks / (float)freqVDP;

	float relative = secReference / seconds;
	float result = relative * MSX_CLOCK;

	uint8_t digit;
	char *p;
	
	p = OUTPUT;
	digit = (uint8_t)floorf(result);
	if (digit>=10) 
		*p = '0' + digit/10;
	*++p = '0' + digit%10;
	result -= floorf(result);

	p++;
	result *= 10.;
	digit = (uint8_t)floorf(result);
	*++p = '0' + digit;
	result -= floorf(result);

	result *= 10.;
	digit = (uint8_t)floorf(result);
	*++p = '0' + digit;
}

void main(char **argv, int argc)
{
	do {
		setCustomInterrupt();

		putch(0x0C);
		getMhz(im2_counter, 50);
		printf("50 -> %s [%u]\n\r", OUTPUT, im2_counter);
		getMhz(im2_counter, 60);
		printf("60 -> %s [%u]\n\r", OUTPUT, im2_counter);
	} while (varNEWKEY_row7.esc);

	while (kbhit()) getch();

	exit();
}