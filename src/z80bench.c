#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "globals.h"
#include "msx_const.h"
#include "heap.h"
#include "dos.h"
#include "conio.h"
#include "utils.h"
#include "ocm_ioports.h"
#include "msx1_functions.h"
#include "patterns.h"
#include "z80bench.h"


/**
 * Variables and functions that vary from MSX1 to MSX2 or higher.
 */
uint8_t screenMode = BW80;
uint16_t blinkBase = 0x0800;
uint16_t patternsBase = 0x1000;
void (*drawCpuSpeed_ptr)();
void (*drawPanel_ptr)();
void (*showCPUtype_ptr)();
void (*showVDPtype_ptr)();
void (*textattr_ptr)(uint16_t attr) __z88dk_fastcall;
void (*textblink_ptr)(uint8_t x, uint8_t y, uint16_t length, bool enabled);

/**
 * Variables for calculating CPU speed.
 */
static uint8_t vdpFreq;
float calculatedFreq = 0;
uint8_t speedLineScale = 1;
uint64_t int_counter = 0;
uint64_t counterRestHL = 0;

/**
 * Pointer to a string buffer used to hold a floating-point value as a string.
 * This is likely used for displaying the floating-point value in a formatted
 * way, such as in a user interface.
 */
#define FLOATSTR_LEN	8
char  *floatStr;

/**
 * Stores the MSX version of the ROM.
 * This variable is likely used to determine the capabilities and features of the
 * MSX system the program is running on, in order to adapt the behavior and
 * presentation accordingly.
 */
#define MSX_1		0
#define MSX_2		1
#define MSX_2P		2
#define MSX_TR		3
uint8_t msxVersionROM;
uint8_t machineBrand;

/**
 * Stores the current CPU type of the MSX system.
 */
#define CPU_Z80		0
#define CPU_R800	1
#define CPU_Z280	2
uint8_t cpuType = CPU_Z80;
bool    isCMOS;

/**
 * Stores the current VDP (Video Display Processor) type of the MSX system.
 */
uint8_t vdpType;
bool    isNTSC;

/**
 * Stores a flag indicating whether the TurboPana feature has been detected.
 * TurboPana is a hardware enhancement for the MSX2+ allowing CPU to run at 5.36MHz.
 */
static bool turboPanaDetected;
static bool turboPanaEnabled = false;

/**
 * Stores the current TurboR mode of the MSX system.
 */
#define TR_Z80			0			// The system is running in Z80 mode.
#define TR_R800_ROM		1			// The system is running in R800 ROM mode.
#define TR_R800_DRAM	2			// The system is running in R800 DRAM mode.
static uint8_t turboRmode = TR_Z80;
static bool    turboRdetected;

/**
 * Stores a flag indicating whether the OCM (oneChipMSX/MSX++) has been detected.
 * OCM firmware allows multiple CPU custom speeds:
 * 3.57MHz, 4.10MHz, 4.48MHz, 4.90MHz, 5.39MHz, 6.10MHz, 6.96MHz, 8.06MHz.
 */
static bool ocmDetected;
static uint8_t ocmSpeedIdx = -1;

/**
 * Stores a flag indicating whether the Tides-Rider board has been detected.
 * Tides-Rider is a MSX2+ board allowing the CPU to run at:
 * 3.57MHz, 6.66MHz, 10MHz, 20MHz.
 */
static bool tidesDetected;
static uint8_t tidesSpeed = TIDES_20MHZ;

/**
 * Stores the original values of various system variables before they are modified.
 * These variables are used to restore the system state when the program exits.
 */
static uint8_t kanjiMode;
static uint8_t originalLINL40;
static uint8_t originalCRTCNT;
static uint8_t originalSCRMOD;
static uint8_t originalFORCLR;
static uint8_t originalBAKCLR;
static uint8_t originalBDRCLR;
static uint8_t originalCLIKSW;


void drawCpuSpeed();
void drawPanel();
void showCPUtype();
void showVDPtype();
bool detectNTSC();


// ========================================================
static void checkPlatformSystem()
{
	originalLINL40 = varLINL40;
	originalCRTCNT = varCRTCNT;
	originalSCRMOD = varSCRMOD;
	originalFORCLR = varFORCLR;
	originalBAKCLR = varBAKCLR;
	originalBDRCLR = varBDRCLR;
	originalCLIKSW = varCLIKSW;

	// Check MSX2 ROM or higher
	msxVersionROM = getRomByte(MSXVER);

	// VDP type
	vdpType = detectVDP();
	if (!msxVersionROM && vdpType >= VDP_V9938) {	// for MSX1 with V9938 or higher
		varRG9SAV.raw = 0;
		varRG9SAV.NT = !(getRomByte(LOCALE) >> 7);
		setNTSC(varRG9SAV.NT);
	}

	// Configure dynamics functions
	if (!msxVersionROM) {
		screenMode = BW40;
		blinkBase = 0x4000;
		patternsBase = 0x0800;
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
	if (turboRdetected) {
		turboRmode = getCpuTurboR();
	}

	// Check OCM-PLD/MSX++
	ocmDetected = ocm_detectDevice(DEVID_OCMPLD);

	// Check Tides-Rider
	tidesDetected = detectTidesRider();

	// Machine type
	machineBrand = detectMachineBrand();

	// NTSC/PAL
	isNTSC = detectNTSC();

	// Detect if Z80 is NMOS/CMOS
	isCMOS = detectNMOS();
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
	return !(vdpType >= VDP_V9938 ?
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
		ld   hl, (#JIFFY)
		ei
	.waitLoop:
		ld   a, (#JIFFY)
		cp   l
		jp   z, .waitLoop
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
		char *str = cpuTypesStr[cpuType];
		if (cpuType == CPU_Z80) {
			csprintf(heap_top, "%s(%s)    ", str, cmosStr[isCMOS]);
			str = heap_top;
		}
		putstrxy(17,4, str);
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

	if (cpuType == CPU_Z280) {
		// Z280 speed test not supported
		csprintf(heap_top, "\x86 N/A \x87\x80\x80");
		putstrxy(50,1, heap_top);
		
		putstrxy(65,3, "N/A ");
		
		// Show message instead of speed graph
		putstrxy(GR_X+1, GR_Y+1, "Z280 speed test not supported (prevents freeze)");
		putstrxy(GR_X+1, GR_Y+2, "Interrupt handling incompatible with test loop");
		
		// Clear speed display in top panel
		putstrxy(17,5, "N/A MHz   ");
		return;
	}

	// Draw counter in top-right border
	csprintf(heap_top, "\x86 %lu \x87\x80\x80", int_counter);
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
void doInterruptLoop() __naked
{
	__asm
		ld   hl, #0					; Counter = 0 (64 bits long)
		ld   (_int_counter), hl
		ld   (_int_counter+2), hl
		ld   (_int_counter+4), hl
		ld   (_int_counter+6), hl
		ld   (_counterRestHL), hl	; CounterRest = 0 (64 bits long)
		ld   (_counterRestHL+2), hl
		ld   (_counterRestHL+4), hl
		ld   (_counterRestHL+6), hl

		ei
		halt						; Wait interruption to change the hook

		di							; Change the VBLANK interrupt hook
		ld   hl, (#0x38+1)
		ld   (#.rstBackup), hl
		ld   hl, #.intRoutine
		ld   (#0x38+1), hl
		ei
		halt						; Important to have stable values (skip first interrupt)

		xor  a						; Test loop
		ld   b, #LOOP2
	.loop1:
		ld   hl, #0xffff
	.loop2:
		dec  hl
		cp   h
		jp   nz, .loop2
		cp   l
		jp   nz, .loop2
		djnz .loop1

		ld   hl, (#.rstBackup)		; End test
		ld   (#0x38+1), hl			; Restore original interrupt hook
		ei

		ld   hl, #_int_counter		; Discart the first interrupt from counter
		dec  (hl)

		ret

	// ######### INTERRUPT ROUTINE #########
	.intRoutine:					; Code is now here
		push hl						; Save registers that are modified
		push af

		in   a, (0x99)				; Read S#0
		and  #0b10000000			; Does INT originate from VDP?
		jr   z, .notFromVDP			; No -> go to exit

		ld   hl, (_int_counter)		; Nr. of interrupts counter
		inc  hl						; Increase counter by one
		ld   (_int_counter), hl

	.notFromVDP:
		pop  af						; Restore modified registers
		pop  hl

		ld (_counterRestHL), hl		; Save HL rest of last interrupt

		ei							; Interrupts are permitted again
		ret							; Return to main program
	.intRoutEnd:					; For Length of routine code

	.rstBackup:
		.ds 2
	__endasm;
}

void calculateCounterRest() {
	// Calculate decimals with counterRestHL
	int_counter = (counterRestHL * 1000000ULL / (((655350ULL)-counterRestHL)/int_counter)) + int_counter * 1000000ULL;
}

void calculateMhz()
{
	isNTSC = detectNTSC();
	uint64_t reference = isNTSC ? 905922538492488ULL : 758316840607900ULL;	// constants for 3.58 MHz NTSC/PAL (fixed point 1e6)
	uint64_t offset = isNTSC ? 8437ULL : 6724ULL;						// offsets for NTSC/PAL (fixed point 1e6)

	calculatedFreq = (uint32_t)(reference / int_counter + offset) / 1000000.f;
}


// ========================================================
void commandLine(char type)
{
	*((char*)&titleStr[strlen(titleStr)-2]) = '\0';
	*((char*)&authorStr[11]) = '\0';
	cprintf("%s (by %s)\n", &titleStr[2], &authorStr[2]);

	if (type != 'd' && type != 'D') {
		die("\nz80bench [d]\n\n"
			"  <none>: Run GUI mode (default)\n"
			"  d:      Run Command line Debug mode\n");
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
		if (cpuType == CPU_Z80) {
			cprintf("%s(%s)", cpuTypesStr[cpuType], cmosStr[isCMOS]);
		} else {
			cputs(cpuTypesStr[cpuType]);
		}
	}
	putch('\n');
	// Video mode
	cputs(infoVdpTypeStr);
	cprintf("%s %s\n", vdpTypeStr[vdpType], vdpModesStr[detectNTSC()]);
	
	// CPU speed test
	cputs("Running TestLoop v"TESTLOOP_VERSION":\n");

	if (cpuType == CPU_Z280) {
		cputs("Z280 speed test not supported (interrupt handling incompatible)\n");
		cputs("Z280 detected but speed measurement skipped to prevent freeze\n");
	} else {
		doInterruptLoop();
		calculateCounterRest();
//		calculateMhz();

		uint32_t cnt = int_counter;
		for (int32_t i=cnt+20000; i>=cnt-20000; i+=-10000) {
			int_counter = i;
			calculateMhz();
			formatFloat(calculatedFreq, floatStr, 6);
			cprintf("%s %lu: %s MHz\n", i==cnt?"->":"  ", i, floatStr);
		}
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

//int_counter = 220;
	do {
		if (cpuType == CPU_Z280) {
			// Z280 speed test not supported, just wait and show static message
			for (volatile int i = 0; i < 50000; i++);  // Simple delay
			// Set static values to prevent display issues
			int_counter = 0;
			calculatedFreq = 0.0f;
		} else {
			doInterruptLoop();
			calculateCounterRest();
			calculateMhz();
		}
		drawCpuSpeed_ptr();
		click();
//getch();
//int_counter--;

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
			isCMOS = detectNMOS();
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
		if (!varNEWKEY_row6.f1 && !varNEWKEY_row6.shift && vdpType >= VDP_V9938) {
			isNTSC = !detectNTSC();
			setNTSC(isNTSC);
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
	varCRTCNT = originalCRTCNT;
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
