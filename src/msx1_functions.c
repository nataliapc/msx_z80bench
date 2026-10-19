#include <string.h>
#include "globals.h"
#include "msx1_functions.h"
#include "msx_const.h"
#include "heap.h"
#include "conio.h"
#include "utils.h"
#include "conio_aux.h"


// ========================================================
extern uint32_t int_counter;
extern float   calculatedFreq;
extern uint8_t speedLineScale;
extern char   *floatStr;
extern uint8_t msxVersionROM;
extern uint8_t machineBrand;
extern uint8_t cpuType;
extern bool    isCMOS;
extern uint8_t vdpType;
extern bool    isNTSC;
extern const char titleStr[];
extern const char authorStr[];
extern const char infoMachineStr[];
extern const char infoCpuTypeStr[];
extern const char infoVdpTypeStr[];
extern const char *machineTypeStr[];
extern const char *machineBrandStr[];
extern const char *cpuTypesStr[];
extern const char *cmosStr[];
extern const char *vdpTypeStr[];
extern const char *vdpModesStr[];
extern const char *info1Str;
extern const char *info2Str;
extern uint8_t testType;
extern const char *testTypesStr[];
extern const float turboRSpeeds[];
extern const float speedFactors[];
extern const uint8_t speedDecimals[];
extern const char *speedUnitsStr[];
extern const char *speedLabelsStr[];

static const float speedPerChar[TESTS_NUM] = { 1.f, 100.f, 100.f };	// Graph units per character
static const char *axisYLabelsStr[] = {
	"", "CPU speed", "", "MSX std", "", "turboPANA", "", "turboR", "R800 DRAM"
};
static const char *speedLineStr = "    \x92    \x92    \x92    \x92    \x92    \x92";
static const float speedDecLimits[] = { .14f, .29f, .43f, .57f, .71f, .86f, 1.f };


// ========================================================
void waitVBLANK();
void putstrxy(uint8_t x, uint8_t y, const char *str);


// ========================================================
void msx1_showCPUtype()
{
	csprintf(heap_top, "%s(%s)    ", cpuTypesStr[cpuType], cmosStr[isCMOS]);
	putstrxy(15,4, heap_top);
}

void msx1_showVDPtype()
{
	csprintf(heap_top, "%s  %s", vdpTypeStr[vdpType], vdpModesStr[isNTSC]);
	putstrxy(15,6, heap_top);
}

// ========================================================
#define GR_X	10
#define GR_Y	8
#define GR_H	9
#define LABELSY_X 	1
#define LABELSY_Y 	GR_Y

static uint16_t formatSpeedLine(char *floatStr, float speed)
{
	speed *= speedFactors[testType];			// MHz to the unit of the test
	float chars = speed / (speedPerChar[testType] * speedLineScale);
	uint16_t speedUnits = (uint16_t)chars;
	float speedDecimal = chars - speedUnits;
	char *q = heap_top, *p;

	// Prepare line buffer
	memcpy(q, speedLineStr, 30);
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

	// Print speed numbers (formatFloat truncates: round the integer kB/s)
	p = formatFloat(speedDecimals[testType] ? speed : speed + 0.5f, floatStr, speedDecimals[testType]);
	memcpy(q, floatStr, p-floatStr);

	return q-heap_top + p-floatStr;
}

void msx1_showTestType()
{
	uint8_t i, len;

	// The labels, units, scale and reference bars follow the selected test
	putstrxy(LABELSY_X, LABELSY_Y+1, testTypesStr[testType]);
	csprintf(heap_top, "%s--         ", speedLabelsStr[testType]);
	putstrxy(3,5, heap_top);

	// Labels X: numbers end next to each tick (the last one, at the screen border, before it)
	// kB/s numbers are wider: only every other tick
	memset(heap_top, ' ', 31);
	putlinexy(GR_X, GR_Y+GR_H+1, 31, heap_top);
	for (i=1; i<7; i++) {
		if (testType != TEST_CPU && (i & 1)) continue;
		len = formatFloat(i*5*speedPerChar[testType], floatStr, 0) - floatStr;
		putstrxy(GR_X+2+i*5-len-(i==6), GR_Y+GR_H+1, floatStr);
	}

	// Reference bars
	formatSpeedLine(floatStr, MSX_CLOCK);		// MSX standard 3.57 MHz
	putlinexy(GR_X+1, GR_Y+3, 30, heap_top);
	formatSpeedLine(floatStr, MSX_CLOCK*1.5f);	// tPANA 5.36 MHz (a Z80: same for every test)
	putlinexy(GR_X+1, GR_Y+5, 30, heap_top);
	formatSpeedLine(floatStr, turboRSpeeds[testType]);	// turboR R800 for the selected test
	putlinexy(GR_X+1, GR_Y+7, 30, heap_top);
}

void msx1_drawPanel()
{
	uint8_t i;

	waitVBLANK();

	// Big frame
	_fillVRAM(0, 40, '\x80');
	_fillVRAM(23*40, 40, '\x80');

	// Title
	putstrxy(2, 1, titleStr);

	// Author
	putlinexy(27, 24, 13, authorStr);

	// Info
	drawFrame(1,2, 40,7);
	putstrxy(3,3, infoMachineStr);
	putstrxy(3,4, infoCpuTypeStr);
	putstrxy(3,5, "CPU Speed : --         ---%");
	putstrxy(3,6, infoVdpTypeStr);

	// Machine type
	if (machineBrand == 0) {
		csprintf(heap_top, "%s", machineTypeStr[msxVersionROM]);
	} else {
		csprintf(heap_top, "%s (%s)", machineTypeStr[msxVersionROM], machineBrandStr[machineBrand]);
	}
	putstrxy(15,3, heap_top);

	// CPU type
	msx1_showCPUtype();

	// Video mode
	msx1_showVDPtype();

	// Graph
	cvlinexy(GR_X,GR_Y, GR_H);
	putlinexy(GR_X,GR_Y+GR_H, 1, "\x1a");

	// Axis X
	for (i=1; i<7; i++) {
		putlinexy(GR_X+i*5-4,GR_Y+GR_H, 5, "\x91\x91\x91\x91\x8f");
	}

	// Labels Y
	for (i=0; i<9; i++) {
		putlinexy(GR_X+1, LABELSY_Y+i, 30, speedLineStr);
		putstrxy(LABELSY_X, LABELSY_Y+i, axisYLabelsStr[i]);
	}

	// Test label, labels X & reference bars
	msx1_showTestType();

	// Information
	putstrxy(3,20, info1Str);
	putstrxy(3,21, info2Str);
	putstrxy(3, 23, "[F5] Test");
	putstrxy(23, 23, "Hold [ESC] to exit");
}

void msx1_drawCpuSpeed()
{
	char *p;

	// Draw counter in top-right border
	gotoxy(3,24);
	cprintf("\x86 %lu \x87\x80\x80", int_counter);

	// Draw % of CPU speed
	p = formatFloat(calculatedFreq * 100.f / MSX_CLOCK + 0.5f, heap_top, 0);
	*p++ = '%';
	*p++ = ' ';
	*p = '\0';
	putstrxy(26,5, heap_top);

	// Prepare line buffer
	formatSpeedLine(floatStr, calculatedFreq);

	// Print speed line
	waitVBLANK();
	putlinexy(GR_X+1, GR_Y+1, 30, speedLineStr);
	waitVBLANK();
	putlinexy(GR_X+1, GR_Y+1, 30, heap_top);

	// Print CPU/memory speed in top panel
	csprintf(heap_top, "%s %s          ", floatStr, speedUnitsStr[testType]);
	heap_top[11] = '\0';						// Up to the % column
	putstrxy(15,5, heap_top);
}

void msx1_textattr(uint16_t attr) __naked __z88dk_fastcall
{
	attr;
	__asm
		ld   a, l
		and  #0xf
		ld   (BAKCLR), a
		ld   a, l
		rra
		rra
		rra
		rra
		and  #0xf
		ld   (FORCLR), a
		ld   ix, #CHGCLR
		JP_BIOSCALL
	__endasm;
}

void msx1_textblink(uint8_t x, uint8_t y, uint16_t length, bool enabled) __naked
{
	x, y, length, enabled;
	__asm
		ret
	__endasm;
}