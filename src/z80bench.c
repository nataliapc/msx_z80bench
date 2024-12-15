#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "heap.h"
#include "dos.h"
#include "conio.h"
#include "utils.h"
#include "patterns.h"
#include "msx_const.h"

#define intVecTabAd		0x8000		// Put the Interrupt Vector Table here
#define intVecHalfAd	0x80		// High memory address pointer
#define intRoutStart	0x8181		// Let interrupt routine start here
#define intRoutHalfAd	0x81		// Address high and low

#define MSX_CLOCK		3.579545455f	// MHz

static const char titleStr[] = "\x86 Z80 Frequency Benchmark v1.1.0b \x87";
static const char authorStr[] = "\x86 NataliaPC \x87";

static const char axisXLabelsStr[6][6] = {
	" 5MHz", "10MHz", "15MHz", "20MHz", "25MHz", "  30"
};
static const char axisYLabelsStr[8][14] = {
	"CPU speed", "(MHz)", "", "MSX standard", "(3.579MHz)", "", "MSX2+ tPANA", "(5.369MHz)"
};

static const char cpuTypesStr[3][12] = {
	"Z80        ", "R800", "Z280"
};

static const char vdpTypeStr[3][9] = {
	"TMS9918A", "V9938", "V9958"
};

static const char turboRmodeStr[3][5] = {
	"", "ROM", "DRAM"
};

static const char vdpModesStr[2][12] = {
	"(NTSC 60Hz)", "(PAL 50Hz) "
};

static const char *machineTypeStr[] = {
	// [0-3] ROM Byte $2D
	"MSX1", "MSX2", "MSX2+", "MSX TurboR"
};
static const char *machineBrandStr[] = {
	// [0] Unknown
	"??",
	// [1-26] I/O port $40
	"ASCII", "Canon", "Casio", "Fujitsu", "General", "Hitachi", "Kyocera", "Panasonic", "Mitsubishi", "NEC", 
	"Yamaha", "JVC", "Philips", "Pioneer", "Sanyo", "Sharp", "Sony", "Spectravideo", "Toshiba", "Mitsumi", 
	"Telematica", "Gradiente", "Sharp Brasil", "GoldStar", "Daewoo", "Samsung"
	// [27] OCM/MSX++
	"OCM/MSX++"
};

const char speedLineStr[] = "         \x92         \x92         \x92         \x92         \x92";

const float speedDecLimits[] = { .07f, .14f, .21f, .28f, .35f, .42f, .49f };

static uint8_t vdpFreq;
static float calculatedFreq = 0;
static uint16_t im2_counter;

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

static uint8_t tidesSpeed = TIDES_20MHZ;

static uint8_t kanjiMode;
static uint8_t originalLINL40;
static uint8_t originalSCRMOD;
static uint8_t originalFORCLR;
static uint8_t originalBAKCLR;
static uint8_t originalBDRCLR;
static uint8_t originalCLIKSW;


// ========================================================

uint8_t getRomByte(uint16_t address) __sdcccall(1);

void setByteVRAM(uint16_t vram, uint8_t value) __sdcccall(0);
void _fillVRAM(uint16_t vram, uint16_t len, uint8_t value) __sdcccall(0);
void _copyRAMtoVRAM(uint16_t memory, uint16_t vram, uint16_t size) __sdcccall(0);

void abortRoutine();
void restoreScreen();
uint8_t detectMachineBrand();
bool detectTurboR() __z88dk_fastcall;
uint8_t detectCPUtype();


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
		and  #0x7F
		out  (0xAA),a

		ei
		halt

		in   a,(0xAA)
		or   #0x80
		out  (0xAA),a
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

void setNTSC(bool enabled) __naked __z88dk_fastcall
{
	enabled;
	__asm
		ld   a, l
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

	ASM_EI; ASM_HALT

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
	putstrxy(42, 3, "This computer performs like a Z80");
	putstrxy(42, 4, "at (CPU Speed) MHz.");
	putstrxy(42, 5, "Clock measurement is approximate.");
	putstrxy(42, 6, "May vary with external RAM mappers.");

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
		putstrxy(16,22, "[F2] Toggle");
		putstrxy(16,23, "TurboR CPU");
	}

	putstrxy(42,22, "[F4] Toggle");
	putstrxy(42,23, "Tides Speed");

	putstrxy(68,22, "[F6] Toggle");
	putstrxy(68,23, "NTSC/PAL");

	putstrxy(3, 20, "Hold keys until click sound");
	putstrxy(65, 20, "[ESC] to exit");
}

void drawCpuSpeed()
{
	float speed = calculatedFreq;
	float speedDecimal = calculatedFreq;
	uint16_t digit, speedUnits = (uint8_t)floorf(speed / .5f);
	char *p, *q;

	// Prepare line buffer
	speedDecimal -= speedUnits * 0.5f;
	memcpy(heap_top, speedLineStr, 60);
	memset(heap_top, '\x8e', speedUnits);
	for (uint8_t i=0; i<sizeof(speedDecLimits); i++) {
		if (speedDecimal <= speedDecLimits[i]) {
			*(heap_top+speedUnits) = '\x88'+i;
			break;
		}
	}

	// Print speed numbers
	q = p = heap_top + speedUnits + 1;
	*p = ' ';

	digit = (uint8_t)floorf(speed);
	if (digit>=10) 
		*p = '0' + digit/10;
	*++p = '0' + digit%10;
	speed -= digit;

	*++p = '.';
	speed *= 10.f;
	digit = (uint8_t)floorf(speed);
	*++p = '0' + digit;
	speed -= digit;

	speed *= 10.f;
	digit = (uint8_t)floorf(speed);
	*++p = '0' + digit;

	// Dump to screen
	ASM_EI; ASM_HALT;
	textblink(GR_X+1, GR_Y+1, 79-(GR_X+1), false);
	ASM_EI; ASM_HALT;
	textblink(GR_X+1, GR_Y+1, 79-(GR_X+1), true);

	putlinexy(GR_X+1, GR_Y+1, 50, heap_top);
	*(q+5) = '\0';
	if (*q == ' ') q++;
	csprintf(heap_top, "%s MHz  ", q);
	putstrxy(17,5, heap_top);
}

// ========================================================
// http://map.grauw.nl/articles/interrupts.php
void setCustomInterrupt() __naked
{
	__asm
		ld hl,#0				;
		ld (_im2_counter),hl	;Counter = 0

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
		ei
		halt

		di						;No interrupts during switch
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
/*
		NTSC 60Hz		PAL 50Hz
		253				212				Z80 3.58Mhz
		169				141				Z80 tPANA 5.37Mhz
		44				36				TurboR R800
*/
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
		jr  z,notFromVDP$		;No -> go to exit

		ld hl, (_im2_counter)	;Nr. of interrupts counter
		inc hl					;Increase counter by one
		ld (_im2_counter), hl

	notFromVDP$:
		pop af					;Restore modified registers
		pop hl
		ei						;Interrupts are permitted again
		reti					;Return to main program
	intRoutEnd$:				;For Length of routine code	
	__endasm;
}


// ========================================================
void calculateMhz()
{
	vdpFreq = varRG9SAV.NT ? 50 : 60;
	float offset = varRG9SAV.NT ? 0.0f : 0.005f;

	float secReference = varRG9SAV.NT ? 4.2441f : 4.221666667f;//253.3f / 60.f --- tests/sec in a MSX at VDP60Hz
	float seconds = (float)im2_counter / (float)vdpFreq;
	calculatedFreq = (secReference * MSX_CLOCK / seconds) + offset;
}


// ========================================================
void commandLine()
{
	setCustomInterrupt();
	calculateMhz();

	char freq[] = " 0.00";
	uint8_t funits = (uint16_t)calculatedFreq,
			fdecimal = (uint16_t)((calculatedFreq - funits) * 100.f);

	if (funits >= 10) {
		freq[0] = '0' + (funits / 10);
	}
	freq[1] = '0' + (funits % 10);
	if (fdecimal >= 10) {
		freq[3] = '0' + (fdecimal / 10);
	}
	freq[4] = '0' + (fdecimal % 10);

	cprintf("Counter  : %u\n"
			"Frequency: %s\n", 
			im2_counter, 
			freq);
}

// ========================================================
int main(char **argv, int argc) __sdcccall(0)
{
	argv; argc;

	if (argc != 0) {
		commandLine();
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
	textattr(0x24f4);
	setcursortype(NOCURSOR);
	redefineCharPatterns();
	varCLIKSW = 0;

	// Initialize header & panel
	drawPanel();

	do {
		setCustomInterrupt();
		calculateMhz();
		drawCpuSpeed();
		click();

gotoxy(58,1);
cprintf("\x86 %u \x87\x80", im2_counter);

		// F1: Toggle tPANA speed
		if (!varNEWKEY_row6.f1 && varNEWKEY_row6.shift && turboPanaDetected) {
			turboPanaEnabled = !turboPanaEnabled;
			setTurboPana(turboPanaEnabled);
		} else
		// F2: Toggle TurboR CPU
		if (!varNEWKEY_row6.f2 && varNEWKEY_row6.shift && turboRdetected) {
			turboRmode++;
			if (turboRmode > TR_R800_DRAM) turboRmode = TR_Z80;
			setCpuTurboR(turboRmode);
			cpuType = detectCPUtype();
			showCPUtype();
		} else
		// F4: Toggle Tides Speed
		if (!varNEWKEY_row7.f4 && varNEWKEY_row6.shift) {
			tidesSpeed++;
			tidesSpeed &= 0xb00000011;
			setTidesSpeed(tidesSpeed | TIDES_SLOTS357);
		} else
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
;		ld  ix, #INITXT				; Restore SCREEN 0
		jr  .bioscall
	.screen1:
;		ld  ix, #INIT32				; Restore SCREEN 1
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
