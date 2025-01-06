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
#include "msx1_functions.h"
#include "patterns.h"
#include "msx_const.h"
#include "z80bench.h"


#define MSX_CLOCK		((float)3.579545f)	// MHz

#define NTSC_LINES		525
#define PAL_LINES		625

#define intVecTabAd		0x8000		// Put the Interrupt Vector Table here
#define intVecHalfAd	0x80		// High memory address pointer
#define intRoutStart	0x8181		// Let interrupt routine start here
#define intRoutHalfAd	0x81		// Address high and low

uint8_t screenMode = BW80;
uint16_t blinkBase = 0x0800;
uint16_t patternsBase = 0x1000;
void (*setCustomInterrupt_ptr)();
void (*calculateMhz_ptr)();
void (*drawCpuSpeed_ptr)();
void (*drawPanel_ptr)();
void (*showCPUtype_ptr)();
void (*showVDPtype_ptr)();
void (*textattr_ptr)(uint16_t attr) __z88dk_fastcall;
void (*textblink_ptr)(uint8_t x, uint8_t y, uint16_t length, bool enabled);

static uint8_t vdpFreq;
float calculatedFreq = 0;
uint8_t speedLineScale = 1;
uint32_t im2_counter = 0;
uint32_t counterRestHL = 0;

#define FLOATSTR_LEN	8
char *floatStr;

#define MSX_1		0
#define MSX_2		1
#define MSX_2P		2
#define MSX_TR		3
uint8_t msxVersionROM;
uint8_t machineBrand;

#define CPU_Z80		0
#define CPU_R800	1
#define CPU_Z280	2
uint8_t cpuType = CPU_Z80;

uint8_t vdpType;
bool isNTSC;

static bool turboPanaDetected;
static bool turboPanaEnabled = false;

static bool turboRdetected;
#define TR_Z80			0
#define TR_R800_ROM		1
#define TR_R800_DRAM	2
static uint8_t turboRmode = TR_Z80;

static bool ocmDetected;
static uint8_t ocmSpeedIdx = -1;

static bool tidesDetected;
static uint8_t tidesSpeed = TIDES_20MHZ;

static uint8_t kanjiMode;
static uint8_t originalLINL40;
static uint8_t originalSCRMOD;
static uint8_t originalFORCLR;
static uint8_t originalBAKCLR;
static uint8_t originalBDRCLR;
static uint8_t originalCLIKSW;


void setCustomInterrupt_v1();
void setCustomInterrupt_v2();
void calculateMhz_v1();
void calculateMhz_v2();
void drawCpuSpeed();
void drawPanel();
void showCPUtype();
void showVDPtype();
bool detectNTSC();


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
		screenMode = BW40;
		blinkBase = 0x4000;
		patternsBase = 0x0800;
		setCustomInterrupt_ptr = setCustomInterrupt_v1;
		calculateMhz_ptr = calculateMhz_v1;
		drawCpuSpeed_ptr = msx1_drawCpuSpeed;
		drawPanel_ptr = msx1_drawPanel;
		showCPUtype_ptr = msx1_showCPUtype;
		showVDPtype_ptr = msx1_showVDPtype;
		textattr_ptr = msx1_textattr;
		textblink_ptr = msx1_textblink;
	} else {
		screenMode = BW80;
		blinkBase = 0x0800;
		patternsBase = 0x1000;
		setCustomInterrupt_ptr = setCustomInterrupt_v2;
		calculateMhz_ptr = calculateMhz_v2;
		drawCpuSpeed_ptr = drawCpuSpeed;
		drawPanel_ptr = drawPanel;
		showCPUtype_ptr = showCPUtype;
		showVDPtype_ptr = showVDPtype;
		textattr_ptr = textattr;
		textblink_ptr = textblink;
	}

	// Check CPU type
	cpuType = detectCPUtype();

	// Check MSX2+ w/tPANA
	turboPanaDetected = detectTurboPana();

	// Check TurboR
	turboRdetected = detectTurboR();

	// Check OCM-PLD/MSX++
	ocmDetected = ocm_detectDevice(DEVID_OCMPLD);

	// Check Tides-Rider
	tidesDetected = detectTidesRider();

	// Machine type
	machineBrand = detectMachineBrand();

	// VDP type
	vdpType = detectVDP();

	// NTSC/PAL
	isNTSC = detectNTSC();
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

bool detectNTSC()
{
	return !(msxVersionROM ?
		varRG9SAV.NT :					// for MSX2 or higher
		getRomByte(LOCALE) >> 7			// for MSX1
	);
}

void abortRoutine()
{
	restoreScreen();
	dos2_exit(1);
}

void putstrxy(uint8_t x, uint8_t y, char *str)
{
	if (!*str) return;
	putlinexy(x, y, strlen(str), str);
}

inline void redefineCharPatterns()
{
	_copyRAMtoVRAM((uint16_t)charPatters, patternsBase+0x80*8, 19*8);
}

void waitVBLANK() __naked
{
	__asm
		ei
		halt
		ret
	__endasm;
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
	csprintf(heap_top, "%s %s", vdpTypeStr[vdpType], vdpModesStr[detectNTSC()]);
	putstrxy(17,6, heap_top);
}

// ========================================================
#define GR_X	17
#define GR_Y	8
#define GR_H	9
#define LABELSY_X 	GR_X-13
#define LABELSY_Y 	GR_Y

uint16_t formatSpeedLine(char *floatStr, float speed)
{
	uint16_t speedUnits = (uint16_t)(speed * 2.f / speedLineScale);
	float speedDecimal = speed / speedLineScale - speedUnits * 0.5f;
	char *q = heap_top, *p;

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
	p = formatFloat(speed, floatStr, 2);
	memcpy(q, floatStr, p-floatStr);

	return q-heap_top + p-floatStr;
}

void updateScale()
{
	char *floatStr = malloc(FLOATSTR_LEN);
	char *p = heap_top;

	// X labels
	memset(heap_top, ' ', 54);
	char mhz[] = "MHz";
	uint8_t offset = 0;
	for (uint8_t i=1; i<7; i++) {
		if (i==6) {
			*mhz = '\0';
			offset++;
		}
		formatFloat(i*speedLineScale*5.f, floatStr, 0);
		csprintf(p+offset, "%s%s", floatStr, mhz);
		p+=10;
	}
	putlinexy(GR_X+8, GR_Y+GR_H+1, 54, heap_top);

	// Draw fixed graphs
	formatSpeedLine(floatStr, MSX_CLOCK);		// 3.57 MHz
	putlinexy(GR_X+1, GR_Y+4, 60, heap_top);
	formatSpeedLine(floatStr, MSX_CLOCK*1.5f);	// 5.36 MHz
	putlinexy(GR_X+1, GR_Y+7, 60, heap_top);

	free(floatStr);
}

void drawPanel()
{
	uint16_t i;

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
	putstrxy(5,3, infoMachineStr);
	putstrxy(5,4, infoCpuTypeStr);
	putstrxy(5,5, "CPU Speed : --");
	putstrxy(5,6, infoVdpTypeStr);
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

	// Video mode
	showVDPtype();

	// Info frame
	drawFrame(40, 2, 78, 7);
	putstrxy(42, 3, "This computer performs ---% of an");
	putstrxy(42, 4, "original MSX Z80.");
	putstrxy(42, 5, info1Str);
	putstrxy(42, 6, info2Str);
	textblink(65,3, 5, true);

	// Graph
	chlinexy(GR_X,GR_Y+GR_H, 58);
	cvlinexy(GR_X,GR_Y, GR_H);
	putlinexy(GR_X,GR_Y+GR_H, 1, "\x1a");

	// Axis X
	for (i=1; i<7; i++) {
		putlinexy(GR_X+i*10-9,GR_Y+GR_H, 10, "\x91\x90\x91\x90\x91\x90\x91\x90\x91\x8f");
	}

	// Y labels
	for (i=0; i<9; i++) {
		putlinexy(GR_X+1, LABELSY_Y+i, 60, speedLineStr);
		putstrxy(LABELSY_X, LABELSY_Y+i, axisYLabelsStr[i]);
	}

	// Update scale (X labels & fixed graphs)
	updateScale();

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

	if (tidesDetected) {
		putstrxy(42,22, "[F4] Cycle");
		putstrxy(42,23, "Tides Speed");
	}

	putstrxy(68,22, "[F6] Toggle");
	putstrxy(68,23, "NTSC/PAL");

	putstrxy(3, 20, "Hold keys until click sound");
	putstrxy(65, 20, "[ESC] to exit");
}

void drawCpuSpeed()
{
	char *p;

	// Draw counter in top-right border
	csprintf(heap_top, "\x86 %lu \x87\x80\x80", im2_counter);
	putstrxy(50,1, heap_top);

	// Draw % of CPU speed
	p = formatFloat(calculatedFreq * 100.f / MSX_CLOCK + 0.5f , heap_top, 0);
	*p++ = '%';
	*p++ = ' ';
	*p = '\0';
	putstrxy(65,3, heap_top);

	// Format line & calculate scale
	uint16_t len;
	uint8_t oldLineScale = speedLineScale;
	speedLineScale = 1;
	do {
		len = formatSpeedLine(floatStr, calculatedFreq);
		if (len > 60) { speedLineScale*=2; continue; }
		break;
	} while (true);

	// Update scale if changed
	waitVBLANK();
	if (oldLineScale != speedLineScale) {
		malloc(60);
		updateScale();
		freeSize(60);
	}

	// Clear text blink & wait
	textblink(GR_X+1, GR_Y+1, 79-(GR_X+1), false);
	waitVBLANK();

	// Print speed line & set text blink
	putlinexy(GR_X+1, GR_Y+1, 60, heap_top);
	textblink(GR_X+1, GR_Y+1, (len < 60 ? len : 60), true);

	// Print CPU speed in top panel
	p = floatStr;
	while (*p) p++;
	memcpy(p, " MHz   ", 8);
	putstrxy(17,5, floatStr);
}

// ========================================================
// http://map.grauw.nl/articles/interrupts.php
void setCustomInterrupt_v1() __naked
{
	__asm
		ld   hl, #0					; Counter = 0
		ld   (_im2_counter), hl
		ld   (_im2_counter+2), hl
		ld   (_counterRestHL), hl
		ld   (_counterRestHL+2), hl

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

		ld (_counterRestHL), hl			; Save HL rest of last interrupt

		ei							; Interrupts are permitted again
		reti						; Return to main program
	intRoutEnd:						; For Length of routine code
	__endasm;
}

void calculateMhz_v1()
{
	isNTSC = detectNTSC();
	uint32_t reference = isNTSC ? 906850774L : 758809112L;		// constants for 3.58 MHz NTSC/PAL (fixed point 1e6)
	uint32_t offset = isNTSC ? 5876L : 5231L;					// offsets for NTSC/PAL (fixed point 1e6)

	calculatedFreq = (reference / im2_counter * 1000.f + offset) / 1000000.f;
}


// ========================================================
void setCustomInterrupt_v2() __naked
{
	__asm
		ld   hl, #0					; Counter = 0
		ld   (_im2_counter), hl
		ld   (_im2_counter+2), hl
		ld   (_counterRestHL), hl
		ld   (_counterRestHL+2), hl

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

		ld (_counterRestHL), hl			; Save HL rest of last interrupt

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
	isNTSC = detectNTSC();
	uint32_t reference = isNTSC ? 2724284914L : 2279352268L;	// constants for 3.58 MHz NTSC/PAL (fixed point 1e6)
	uint32_t offset = isNTSC ? 54933L : 46129L;					// offsets for NTSC/PAL (fixed point 1e6)

	calculatedFreq = (reference / im2_counter * 1000.f + offset) / 1000000.f;
}

void calculateCounterRest() {
	// Calculate decimals with counterRestHL
	im2_counter = (counterRestHL * 1000L / ((655350L-counterRestHL)/im2_counter)) + im2_counter * 1000L;
	calculateMhz_ptr();	
}


// ========================================================
void commandLine(char type)
{
	*((char*)&titleStr[33]) = '\0';
	*((char*)&authorStr[11]) = '\0';
	cprintf("%s (by %s)\n", &titleStr[2], &authorStr[2]);

	if (type == '1') {
		setCustomInterrupt_ptr = setCustomInterrupt_v1;
		calculateMhz_ptr = calculateMhz_v1;
	} else if (type == '2' && msxVersionROM) {
		setCustomInterrupt_ptr = setCustomInterrupt_v2;
		calculateMhz_ptr = calculateMhz_v2;
	} else {
		die("\nz80bench [1|2]\n\n  1: Debug TestLoop V1\n  2: Debug TestLoop V2 (not for MSX1)\n");
	}

	// Machine type
	cputs(infoMachineStr);
	if (machineBrand == 0) {
		cprintf("%s\n", machineTypeStr[msxVersionROM]);
	} else {
		cprintf("%s (%s)\n", machineTypeStr[msxVersionROM], machineBrandStr[machineBrand]);
	}
	// CPU type
	cputs(infoCpuTypeStr);
	if (turboRmode) {
		cprintf("%s (%s)", cpuTypesStr[cpuType], turboRmodeStr[turboRmode]);
	} else {
		cputs(cpuTypesStr[cpuType]);
	}
	putch('\n');
	// Video mode
	cputs(infoVdpTypeStr);
	cprintf("%s %s\n", vdpTypeStr[vdpType], vdpModesStr[detectNTSC()]);
	
	// CPU speed
	cprintf("Testing TestLoop V%cb4:\n", type);

	setCustomInterrupt_ptr();
	calculateCounterRest();

	uint32_t im2 = im2_counter;
	for (int32_t i=im2+100; i>=im2-100; i+=-50) {
		im2_counter = i;
		calculateMhz_ptr();
		formatFloat(calculatedFreq, floatStr, 4);
		cprintf("%s %lu: %s MHz\n", i==im2?"->":"  ", i, floatStr);
	}
}

// ========================================================
int main(char **argv, int argc) __sdcccall(0)
{
	argv, argc;

	// A way to avoid using low memory when using BIOS calls from DOS
	if (heap_top < (void*)0xa000)
		heap_top = (void*)0xa000;

	floatStr = malloc(FLOATSTR_LEN);

	//Platform system checks
	checkPlatformSystem();

	// Command line
	if (argc != 0) {
		commandLine(argv[0][0]);
		return 0;
	}	

	// Set abort exit routine
	dos2_setAbortRoutine((void*)abortRoutine);

	// Save current kanji mode
	kanjiMode = (detectKanjiDriver() ? getKanjiMode() : 0);
	if (kanjiMode) {
		setKanjiMode(0);
	}

	// Initialize screen 0[80]
	textmode(screenMode);
	textattr_ptr(0xa4f4);
	setcursortype(NOCURSOR);
	memset((char*)FNKSTR, 0, 160);			// Redefine Function Keys to empty strings
	redefineCharPatterns();
	varCLIKSW = 0;

	// Initialize header & panel
	drawPanel_ptr();

//im2_counter = 220;
	do {
		setCustomInterrupt_ptr();
		calculateCounterRest();
		drawCpuSpeed_ptr();
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
			showCPUtype_ptr();
		} else
		// F3: Cycle OCM Speed
		if (!varNEWKEY_row6.f3 && varNEWKEY_row6.shift && ocmDetected) {
			ocmSpeedIdx = ++ocmSpeedIdx % sizeof(ocmSmartCmd);
			ocm_sendSmartCmd(ocmSmartCmd[ocmSpeedIdx]);
		} else
		// F4: Toggle Tides Speed
		if (!varNEWKEY_row7.f4 && varNEWKEY_row6.shift && tidesDetected) {
			tidesSpeed = ++tidesSpeed % 4;
			setTidesSpeed(tidesSpeed | TIDES_SLOTS357);
		} else
		// F6: Toggle NTSC/PAL
		if (!varNEWKEY_row6.f1 && !varNEWKEY_row6.shift && msxVersionROM) {
			setNTSC(!isNTSC);
			showVDPtype_ptr();
		}
		varPUTPNT = varGETPNT;
	} while (varNEWKEY_row7.esc);

	restoreScreen();
	varPUTPNT = varGETPNT;
	return 0;
}

void restoreScreen()
{
	// Clean & restore original screen parameters & colors
	__asm
		ld   ix, #DISSCR
		BIOSCALL
	__endasm;

	textattr(0x00f4);				// Clear blink
	_fillVRAM(0x0800, 240, 0);

	varLINL40 = originalLINL40;
	varFORCLR = originalFORCLR;
	varBAKCLR = originalBAKCLR;
	varBDRCLR = originalBDRCLR;
	varCLIKSW = originalCLIKSW;
	clrscr();

	// Restore original screen mode
	__asm
		push ix
		ld   a, (_originalSCRMOD)
		or   a
		jr   nz, .screen1
		ld   ix, #INITXT				; Restore SCREEN 0
		jr   .bioscall
	.screen1:
		ld   ix, #INIT32				; Restore SCREEN 1
	.bioscall:
		BIOSCALL
		ld   ix, #INIFNK				; Restore function keys
		BIOSCALL
		pop  ix
	__endasm;

	// Restore kanji mode if needed
	if (kanjiMode) {
		setKanjiMode(kanjiMode);
	}

	__asm
		ld   ix, #CLSSCR
		xor  a
		BIOSCALL
		ld   ix, #ENASCR
		BIOSCALL
	__endasm;

	// Restore abort routine
	dos2_setAbortRoutine(0x0000);
}
