#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "heap.h"
#include "dos.h"
#include "conio.h"
#include "utils.h"
#include "ocm_ioports.h"
#include "patterns.h"
#include "msx_const.h"
#include "z80bench.h"


#define MSX_CLOCK		3.579545455f	// MHz

#define NTSC_LINES		525
#define PAL_LINES		625
#define NTSC_HBLANK		(192 - NTSC_LINES/4)	// ~60 (60.75)
#define PAL_HBLANK		(192 - PAL_LINES/4)		// ~35 (35.75)

#define intVecTabAd		0x8000		// Put the Interrupt Vector Table here
#define intVecHalfAd	0x80		// High memory address pointer
#define intRoutStart	0x8181		// Let interrupt routine start here
#define intRoutHalfAd	0x81		// Address high and low


static uint8_t vdpFreq;
static float calculatedFreq = 0;
static uint16_t im2_counter = 0;

#define MSX_1		0
#define MSX_2		1
#define MSX_2P		2
#define MSX_TR		3
static uint8_t msxVersionROM;
static uint8_t machineBrand;

#define CPU_Z80		0
#define CPU_R800	1
#define CPU_Z280	2
static uint8_t cpuType = CPU_Z80;

static uint8_t vdpType;

static bool turboPanaDetected;
static bool turboPanaEnabled = false;

static bool turboRdetected;
#define TR_Z80			0
#define TR_R800_ROM		1
#define TR_R800_DRAM	2
static uint8_t turboRmode = TR_Z80;

static bool ocmDetected;
static uint8_t ocmSpeedIdx = -1;

static uint8_t tidesSpeed = TIDES_20MHZ;

static uint8_t kanjiMode;
static uint8_t originalLINL40;
static uint8_t originalSCRMOD;
static uint8_t originalFORCLR;
static uint8_t originalBAKCLR;
static uint8_t originalBDRCLR;
static uint8_t originalCLIKSW;


// ========================================================
static void checkPlatformSystem()
{
	originalLINL40 = varLINL40;
	originalSCRMOD = varSCRMOD;
	originalFORCLR = varFORCLR;
	originalBAKCLR = varBAKCLR;
	originalBDRCLR = varBDRCLR;
	originalCLIKSW = varCLIKSW;

	// Check MSX2 ROM or higher
	msxVersionROM = getRomByte(MSXVER);
	if (!msxVersionROM) {
		die("This don't works on MSX1!");
	}

	// Check MSX-DOS 2 or higher
	if (dosVersion() < VER_MSXDOS1x) {
		die("MSX-DOS 2.x or higher required!");
	}
	// Set abort exit routine
	dos2_setAbortRoutine((void*)abortRoutine);

	// Check CPU type
	cpuType = detectCPUtype();

	// Check MSX2+ w/tPANA
	turboPanaDetected = detectTurboPana();

	// Check TurboR
	turboRdetected = detectTurboR();

	// Check OCM-PLD/MSX++
	ocmDetected = ocm_detectDevice(DEVID_OCMPLD);

	// Machine type
	machineBrand = detectMachineBrand();

	// VDP type
	vdpType = detectVDP();
}

uint8_t detectCPUtype()
{
	if (detectZ280()) return CPU_Z280;
	else if (detectR800()) return CPU_R800;
	return CPU_Z80;
}

uint8_t detectMachineBrand()
{
	if (ocmDetected) {
		return BRAND_OCMPLD;
	}
	uint8_t result = 0;
	uint8_t port40 = 255 - inportb(0x40);
	for (uint8_t i=1; i<=26; i++) {
		outportb(0x40, i);
		if (inportb(0x40) == 255 - i) {
			result = i;
			break;
		}
	}
	outportb(0x40, port40);
	return result;
}

void abortRoutine()
{
	restoreScreen();
	dos2_exit(1);
}

void putstrxy(uint8_t x, uint8_t y, char *str)
{
	putlinexy(x, y, strlen(str), str);
}

inline void redefineCharPatterns()
{
	_copyRAMtoVRAM((uint16_t)charPatters, 0x1000+0x80*8, 19*8);
}

void click() __naked
{
	__asm
		ld   a, #(7<<1)|1
		out  (0xAB), a

		ld   b,#10
	click_loop:
		djnz click_loop

		and  #0b11111110
		out  (0xAB),a
		ret
	__endasm;
}

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

void setCpuTurboR(uint8_t mode) __naked __z88dk_fastcall
{
	mode;
	__asm
		push hl
		call _detectTurboR
		xor  a
		cp   l
		pop  hl
		ret  z
		ld	 a, l				; Param L = mode
		or   #0x80				; Update de CPU led
		ld   ix, #0x180			; CHGCPU
		JP_BIOSCALL
	__endasm;
}

void setNTSC(bool enabled) __naked __sdcccall(1)
{
	enabled;
	__asm
		and  #1
		add  a
		ld   l, a
		ld   a, (RG9SAV)
		and  #0b11111101
		or   l
		ld   (RG9SAV), a
		ld   b, a
		ld   c, #0x09		; Register #09
		ld   ix, #WRTVDP
		JP_BIOSCALL
	__endasm;
}

void waitVBLANK() __naked
{
	__asm
		ei
		halt
		ret
	__endasm;
}

char *formatFloat(float value, char *txt, int8_t decimals)
{
	uint8_t digit;
	uint16_t decenes = 1000;
	char *p = txt;

	while (value < decenes) {
		decenes /= 10;
	}

	while (decenes) {
		digit = (uint8_t)floorf(value / decenes);
		*p++ = '0' + digit;
		value -= digit * decenes;
		decenes /= 10;
	}

	if (txt == p) *p++ = '0';
	if (decimals) *p++ = '.';

	while (decimals) {
		value *= 10.f;
		digit = (uint8_t)floorf(value);
		*p++ = '0' + digit;
		value -= digit;
		decimals--;
	}
	*p = '\0';

	return p;
}

// ========================================================

void showCPUtype()
{
	if (turboRmode) {
		csprintf(heap_top, "%s (%s) ", cpuTypesStr[cpuType], turboRmodeStr[turboRmode]);
		putstrxy(17,4, heap_top);
	} else {
		putstrxy(17,4, cpuTypesStr[cpuType]);
	}
}

void showVDPtype()
{
	sprintf(heap_top, "%s %s", vdpTypeStr[vdpType], vdpModesStr[varRG9SAV.NT]);
	putstrxy(17,6, heap_top);

}

#define GR_X	17
#define GR_Y	8
#define GR_H	9
#define LABELSY_X 	GR_X-13
#define LABELSY_Y 	GR_Y+1
void drawPanel()
{
	uint16_t i, j;

	waitVBLANK();

	// Big frame
	_fillVRAM(0, 80, '\x80');
	_fillVRAM(23*80, 80, '\x80');
	for (i=79; i<23*80; i+=80) {
		_fillVRAM(i, 2, '\x81\x81');
	}
	setByteVRAM(0, '\x82');
	setByteVRAM(79, '\x83');
	setByteVRAM(23*80, '\x84');
	setByteVRAM(23*80+79, '\x85');

	// Title
	uint16_t len = strlen(titleStr);
	putlinexy(5, 1, len, titleStr);
	textblink(6, 1, len-2, true);

	// Author
	putlinexy(66, 1, 13, authorStr);

	// Info
	drawFrame(3,2, 39,7);
	putstrxy(5,3, "Machine   :");
	putstrxy(5,4, "CPU Type  :");
	putstrxy(5,5, "CPU Speed :");
	putstrxy(5,6, "VDP Type  :");
	textblink(16,3, 23, true);
	textblink(16,4, 23, true);
	textblink(16,5, 23, true);
	textblink(16,6, 23, true);

	// Machine type
	if (machineBrand == 0) {
		csprintf(heap_top, "%s", machineTypeStr[msxVersionROM]);
	} else {
		csprintf(heap_top, "%s (%s)", machineTypeStr[msxVersionROM], machineBrandStr[machineBrand]);
	}
	putstrxy(17,3, heap_top);

	// CPU type
	showCPUtype();

	// Frequency
	putlinexy(17,5, 2, "--");

	// Video mode
	showVDPtype();

	// Info frame
	drawFrame(40, 2, 78, 7);
	putstrxy(42, 3, "This computer performs ---% of an");
	putstrxy(42, 4, "original MSX Z80.");
	putstrxy(42, 5, "Clock measurement is approximate.");
	putstrxy(42, 6, "May vary with external RAM mappers.");
	textblink(65,3, 5, true);

	// Graph
	chlinexy(GR_X,GR_Y+GR_H, 58);
	cvlinexy(GR_X,GR_Y, GR_H);
	putlinexy(GR_X,GR_Y+GR_H, 1, "\x1a");

	// Labels X
	for (i=1; i<7; i++) {
		putlinexy(GR_X+i*10-9,GR_Y+GR_H, 10, "\x91\x90\x91\x90\x91\x90\x91\x90\x91\x8f");
		putstrxy(GR_X-2+i*10,GR_Y+GR_H+1, axisXLabelsStr[i-1]);
		for (j=GR_Y; j<GR_Y+GR_H; j++) {
			putlinexy(GR_X+i*10,j, 1, "\x92");
		}
	}

	// Labels Y
	for (i=0; i<8; i++) {
		putstrxy(LABELSY_X, LABELSY_Y+i, axisYLabelsStr[i]);
	}

	// Draw fixed graphs
	putstrxy(GR_X+1, GR_Y+4, "\x8e\x8e\x8e\x8e\x8e\x8e\x8e\x89 3.58");
	putstrxy(GR_X+1, GR_Y+7, "\x8e\x8e\x8e\x8e\x8e\x8e\x8e\x8e\x8e\x8e\x8d 5.37");

	// Change color of current CPU speed
	textblink(GR_X+1, GR_Y+1, 79-(GR_X+1), true);

	// Key labels
	chlinexy(2, 21, 78);

	if (turboPanaDetected) {
		putstrxy(3,22, "[F1] Toggle");
		putstrxy(3,23, "tPANA speed");
	}

	if (msxVersionROM == MSX_TR) {
		putstrxy(16,22, "[F2] Cycle");
		putstrxy(16,23, "TurboR CPU");
	}

	if (ocmDetected) {
		putstrxy(29,22, "[F3] Cycle");
		putstrxy(29,23, "OCM Speed");
	}

//	putstrxy(42,22, "[F4] Toggle");
//	putstrxy(42,23, "Tides Speed");

	putstrxy(68,22, "[F6] Toggle");
	putstrxy(68,23, "NTSC/PAL");

	putstrxy(3, 20, "Hold keys until click sound");
	putstrxy(65, 20, "[ESC] to exit");
}

void drawCpuSpeed()
{
	float speed = calculatedFreq + 0.001f;
	uint16_t speedUnits = (uint8_t)(speed * 2.f);
	float speedDecimal = calculatedFreq - speedUnits * 0.5f;
	char *q = heap_top, *p;

	// Draw counter in top-right border
	gotoxy(57,1);
	cprintf("\x86 %u \x87\x80\x80", im2_counter);

	// Draw % of CPU speed
	p = formatFloat(calculatedFreq * 100.f / MSX_CLOCK , heap_top, 0);
	*p++ = '%';
	*p = '\0';
	putstrxy(65, 3, heap_top);

	// Prepare line buffer
	memcpy(q, speedLineStr, 60);
	memset(q, '\x8e', speedUnits);
	q += speedUnits;
	for (uint8_t i=0; i<sizeof(speedDecLimits); i++) {
		if (speedDecimal <= speedDecLimits[i]) {
			*q = '\x88'+i;
			break;
		}
	}
	*++q = ' ';
	q++;

	// Print speed numbers
	p = formatFloat(speed, q, 2);

	// Dump to screen
	waitVBLANK();
	textblink(GR_X+1, GR_Y+1, 79-(GR_X+1), false);
	waitVBLANK();
	textblink(GR_X+1, GR_Y+1, (p-heap_top < 60 ? p-heap_top+1 : 60), true);

	// Print speed line
	putlinexy(GR_X+1, GR_Y+1, 60, heap_top);

	// Print CPU speed in top panel
	memcpy(p, " MHz   ", 8);
	putstrxy(17,5, q);
}

// ========================================================
// http://map.grauw.nl/articles/interrupts.php
void setCustomInterrupt_v1() __naked
{
	__asm
		ld   hl, #0					; Counter = 0
		ld   (_im2_counter), hl

		ld   hl, #intVecTabAd		; Generate IVT here
		ld   (hl), #intRoutHalfAd	; Use this as high and low address part
		ld   d,h					; Copy destination pointer from
		ld   e,l					;   the source pointer
		inc  de						; Destination 1 byte further
		ld   bc, #128*2				; 128 vectors, 1 byte extra for 256th
		ldir						; Generate table

		ld   hl, #intRoutIM2			; Routine for IM 2
		ld   de, #intRoutStart			; Put routine here
		ld   bc, #intRoutEnd-intRoutIM2	; Length of the routine
		ldir							; Copy the routine

		call _waitVBLANK

		di							; No interrupts during switch
		ld   a, #intVecHalfAd		; Use this as high address part
		ld   i, a					; Set high address part
		im   2						; Switch to IM 2
		ei							; Enable interrupts

		xor  a
		ld   b, #10
	loop1:
		ld   hl, #0xffff
	loop2:
		dec  hl
		cp   h
		jp   nz, loop2
		cp   l
		jp   nz, loop2
		djnz loop1

		di							; End test
		im   1						; Switch back to IM 1
		ei							; Now interrupts are permitted again
		ret

	// ######### INTERRUPT ROUTINE #########
	intRoutIM2:						; Code is now here
		push hl						; Save registers that are modified
		push af

	cont:
		in   a, (0x99)				; Read S#0
		bit  7, a					; Does INT originate from VDP?
		jr   z, notFromVDP			; No -> go to exit

		ld   hl, (_im2_counter)		; Nr. of interrupts counter
		inc  hl						; Increase counter by one
		ld   (_im2_counter), hl

	notFromVDP:
		pop  af						; Restore modified registers
		pop  hl
		ei							; Interrupts are permitted again
		reti						; Return to main program
	intRoutEnd:						; For Length of routine code
	__endasm;
}

void calculateMhz_v1()
{
	vdpFreq = varRG9SAV.NT ? 50 : 60;
	float offset = varRG9SAV.NT ? 0.0f : 0.005f;

	float secReference = varRG9SAV.NT ? 4.2441f : 4.221666667f;//253.3f / 60.f --- tests/sec in a MSX at VDP60Hz
	float seconds = (float)im2_counter / (float)vdpFreq;
	calculatedFreq = (secReference * MSX_CLOCK / seconds) + offset;
}


// ========================================================
void setCustomInterrupt_v2() __naked
{
	__asm
		ld   hl, #0					; Counter = 0
		ld   (_im2_counter), hl

		ld   hl, #intVecTabAd		; Generate IVT here
		ld   (hl), #intRoutHalfAd	; Use this as high and low address part
		ld   d, h					; Copy destination pointer from
		ld   e, l					;   the source pointer
		inc  de						; Destination 1 byte further
		ld   bc, #128*2				; 128 vectors, 1 byte extra for 256th
		ldir						; Generate table

		ld   hl, #.v2_intRoutIM2				; Routine for IM 2
		ld   de, #intRoutStart					; Put routine here
		ld   bc, #.v2_intRoutEnd-.v2_intRoutIM2	; Length of the routine
		ldir									; Copy the routine

		call _waitVBLANK

		ld   a, (RG0SAV)			; We want to have line interrupts, so enable them.
		or   #16
		ld   (RG0SAV), a
		ld   b, a					; data to write
		ld   c, #0					; Register number (9 to 24	Control registers 8 to 23	Read / Write	MSX2 and higher)
		call WRTVDP_without_DI_EI	; Write B value to C register

		ld   a, (RG9SAV)
		bit  1, a
		jr   z, .v2_ntsc
		ld   a, #87					; 192 - PAL_LINES / 2 / 3
		ld   (.v2_secondLine), a
		ld   a, #239				; 192 - PAL_LINES / 2 / 3 * 2
		ld   (.v2_firstLine), a
		jr   .v2_setline
	.v2_ntsc:
		ld   a, #104				; 192 - NTSC_LINES / 2 / 3
		ld   (.v2_secondLine), a
		ld   a, #17					; 192 - NTSC_LINES / 2 / 3 * 2
		ld   (.v2_firstLine), a
	.v2_setline:
		ld   b, a					; Set the next line interrupt on line
		ld   c, #19					; Register number (9 to 24	Control registers 8 to 23	Read / Write	MSX2 and higher)
		call WRTVDP_without_DI_EI	; Write B value to C register

		ld   b, #0					; Set display offset to 0
		ld   c, #23					; Register number (9 to 24	Control registers 8 to 23	Read / Write	MSX2 and higher)
		call WRTVDP_without_DI_EI	; Write B value to C register

		di							; No interrupts during switch
		ld   a,#intVecHalfAd		; Use this as high address part
		ld   i, a					; Set high address part
		im   2						; Switch to IM 2
		ei							; Enable interrupts

		xor  a
		ld   b, #10
	.v2_loop1:
		ld   hl, #0xffff
	.v2_loop2:
		dec  hl
		cp   h
		jp   nz, .v2_loop2
		cp   l
		jp   nz, .v2_loop2
		djnz .v2_loop1

		di							; End test

		ld   a, (RG0SAV)			; Before we can exit the program we have to disable line interrupts
		and  #255-16				; Disable line interrupts
		ld   (RG0SAV), a
		ld   b, a					; data to write
		ld   c, #0					; register number (9 to 24	Control registers 8 to 23	Read / Write	MSX2 and higher)
		call WRTVDP_without_DI_EI	; Write B value to C register
		in   a, (0x99)				; Read S#0

		im   1						; Switch back to IM 1
		ei							; Now interrupts are permitted again
		ret

	// ######### INTERRUPT ROUTINE #########
	.v2_intRoutIM2:					; Code is now here
		push hl						; Save registers that are modified
		push af
		ld   h, #1

;ld  a,#0xff
;out (0x99),a
;ld  a,#0x87
;out (0x99),a

		in   a,(0x99)				; Read S#0 (VDP Status Register 0)
		and  #0b10000000			; Does INT originated by VBLANK?
		jr   z, .v2_noVBLANK

		ld   a, (.v2_firstLine)		; R#19 = 192 - [NTSC/PAL]_LINES / 2 / 3 * 2
		out  (0x99),a
		ld   a, #19 + 0x80
		out  (0x99), a
		dec  h

	.v2_noVBLANK:
		ld   a, #1					; Select VDP Status Register 1
		out  (0x99), a
		ld   a, #15 + 0x80			; Prepare read (R#15 = 1)
		out  (0x99), a
		in   a, (0x99)				; Read S#1 (VDP Status Register 1)
		and  #0b00000001			; Does INT originated by HBLANK?
		jr   z, .v2_noHBLANK

		ld   a, (.v2_secondLine)	; R#19 = 192 - [NTSC/PAL]_LINES / 2 / 3
		out  (0x99),a
		ld   a, #19 + 0x80
		out  (0x99), a
		dec  h

	.v2_noHBLANK:
		jr   nz, .v2_notFromVDP		; No -> skip counter increase

		ld   hl, (_im2_counter)		; Nr. of interrupts counter
		inc  hl						; Increase counter by one
		ld   (_im2_counter), hl

		xor  a						; Select VDP Status Register 0
		out  (0x99), a
		ld   a, #15 + 0x80			; Prepare read in next interrupt (R#15 = 0)
		out  (0x99), a

	.v2_notFromVDP:

;ld  a,#0xf4
;out (0x99),a
;ld  a,#0x87
;out (0x99),a

		pop  af						; Restore modified registers
		pop  hl
		ei							; Interrupts are permitted again
		reti						; Return to main program
	.v2_intRoutEnd:					; For Length of routine code

	.v2_firstLine:
		.ds  1
	.v2_secondLine:
		.ds  1

	WRTVDP_without_DI_EI:
		ld   a, b
		out  (0x99),a
		ld   a, c
		or   #0x80
		out  (0x99), a
		ret
	__endasm;
}

void calculateMhz_v2()
{
	vdpFreq = varRG9SAV.NT ? 50 : 60;
	float offset = varRG9SAV.NT ? 0.0494868036f : 0.05506993f;	// PAL / NTSC

	// PAL / NTSC
	float secReference = varRG9SAV.NT ? 12.721658986f : 12.668717949f;	// 510.f/60.f
	float seconds = (float)im2_counter / (float)vdpFreq;
	calculatedFreq = (secReference * MSX_CLOCK / seconds) + offset;
}


// ========================================================
void commandLine(char type)
{
	char *txt = malloc(10);

	if (type == '1') {
		setCustomInterrupt_v1();
		calculateMhz_v1();
	} else if (type == '2') {
		setCustomInterrupt_v2();
		calculateMhz_v2();
	} else {
		die("z80bench [1|2]\n\n  1: Debug TestLoop V1\n  2: Debug TestLoop V2\n");
	}

	formatFloat(calculatedFreq, txt, 2);

	cprintf("Counter  : %u\n"
			"Frequency: %s MHz\n", 
			im2_counter, 
			txt);
}

// ========================================================
int main(char **argv, int argc) __sdcccall(0)
{
	argv; argc;

	if (argc != 0) {
		commandLine(argv[0][0]);
		return 0;
	}	

	kanjiMode = (detectKanjiDriver() ? getKanjiMode() : 0);
	if (kanjiMode) {
		setKanjiMode(0);
	}

	// A way to avoid using low memory when using BIOS calls from DOS
	if (heap_top < (void*)0xa000)
		heap_top = (void*)0xa000;

	//Platform system checks
	checkPlatformSystem();

	// Initialize screen 0[80]
	textmode(BW80);
	textattr(0xa4f4);
	setcursortype(NOCURSOR);
	memset((char*)FNKSTR, 0, 160);			// Redefine Function Keys to empty strings
	redefineCharPatterns();
	varCLIKSW = 0;

	// Initialize header & panel
	drawPanel();

//im2_counter = 3;
	do {
		setCustomInterrupt_v2();
		calculateMhz_v2();
		drawCpuSpeed();
		click();

//getch();
//im2_counter--;

		// F1: Toggle tPANA speed
		if (!varNEWKEY_row6.f1 && varNEWKEY_row6.shift && turboPanaDetected) {
			turboPanaEnabled = !turboPanaEnabled;
			setTurboPana(turboPanaEnabled);
		} else
		// F2: Cycle TurboR CPU
		if (!varNEWKEY_row6.f2 && varNEWKEY_row6.shift && turboRdetected) {
			turboRmode++;
			if (turboRmode > TR_R800_DRAM) turboRmode = TR_Z80;
			setCpuTurboR(turboRmode);
			cpuType = detectCPUtype();
			showCPUtype();
		} else
		// F3: Cycle OCM Speed
		if (!varNEWKEY_row6.f3 && varNEWKEY_row6.shift && ocmDetected) {
			ocmSpeedIdx = ++ocmSpeedIdx % sizeof(ocmSmartCmd);
			ocm_sendSmartCmd(ocmSmartCmd[ocmSpeedIdx]);
		} else
		// F4: Toggle Tides Speed
//		if (!varNEWKEY_row7.f4 && varNEWKEY_row6.shift) {
//			tidesSpeed++;
//			tidesSpeed &= 0xb00000011;
//			setTidesSpeed(tidesSpeed | TIDES_SLOTS357);
//		} else
		// F6: Toggle NTSC/PAL
		if (!varNEWKEY_row6.f1 && !varNEWKEY_row6.shift) {
			setNTSC(varRG9SAV.NT ? false : true);
			showVDPtype();
		}
		varPUTPNT = varGETPNT;
	} while (varNEWKEY_row7.esc);

	restoreScreen();
	varPUTPNT = varGETPNT;
	return 0;
}

void restoreScreen()
{
	// Clean & restore screen
	textattr(0x00f4);
	_fillVRAM(0x1b00, 240, 0);
	clrscr();
	varLINL40 = originalLINL40;
	varFORCLR = originalFORCLR;
	varBAKCLR = originalBAKCLR;
	varBDRCLR = originalBDRCLR;
	varCLIKSW = originalCLIKSW;

	__asm
		push ix
		ld  a, (_originalSCRMOD)
		or  a
		jr  nz, .screen1
		ld  ix, #INITXT				; Restore SCREEN 0
		jr  .bioscall
	.screen1:
		ld  ix, #INIT32				; Restore SCREEN 1
	.bioscall:
		BIOSCALL
		ld  ix, #INIFNK				; Restore function keys
		BIOSCALL
		pop ix
	__endasm;

	// Restore kanji mode if needed
	if (kanjiMode) {
		setKanjiMode(kanjiMode);
	}

	dos2_setAbortRoutine(0x0000);
}
