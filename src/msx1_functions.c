#include <string.h>
#include "msx1_functions.h"
#include "msx_const.h"
#include "heap.h"
#include "conio.h"
#include "utils.h"
#include "conio_aux.h"


#define MSX_CLOCK		3.579545455f	// MHz

// ========================================================
extern uint16_t im2_counter;
extern float calculatedFreq;
extern uint8_t msxVersionROM;
extern uint8_t machineBrand;
extern uint8_t cpuType;
extern uint8_t vdpType;
extern bool isNTSC;
extern const char titleStr[];
extern const char authorStr[];
extern const char *machineTypeStr[];
extern const char *machineBrandStr[];
extern const char *cpuTypesStr[];
extern const char *vdpTypeStr[];
extern const char *vdpModesStr[];
extern const char *info1Str;
extern const char *info2Str;

static const char *axisXLabelsStr[] = {
	"  5", " 10", " 15", " 20", " 25", "30"
};
static const char *axisYLabelsStr[] = {
	"CPU speed", "MSX std", "turboPANA"
};
static const char *speedLineStr = "    \x92    \x92    \x92    \x92    \x92    \x92";
static const float speedDecLimits[] = { .14f, .29f, .43f, .57f, .71f, .86f, 1.f };


// ========================================================
void waitVBLANK();
char *formatFloat(float value, char *txt, int8_t decimals);
void putstrxy(uint8_t x, uint8_t y, const char *str);


// ========================================================
void msx1_showCPUtype()
{
	putstrxy(15,4, cpuTypesStr[cpuType]);
}

void msx1_showVDPtype()
{
	csprintf(heap_top, "%s  %s", vdpTypeStr[vdpType], vdpModesStr[isNTSC]);
	putstrxy(15,6, heap_top);
}

#define GR_X	10
#define GR_Y	8
#define GR_H	9
#define LABELSY_X 	1
#define LABELSY_Y 	GR_Y+1
void msx1_drawPanel()
{
	uint8_t i, j;

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
	putstrxy(3,3, "Machine  :");
	putstrxy(3,4, "CPU Type :");
	putstrxy(3,5, "CPU Speed:  --         ---%");
	putstrxy(3,6, "VDP Type :");

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

	// Labels X
	for (i=1; i<7; i++) {
		putlinexy(GR_X+i*5-4,GR_Y+GR_H, 5, "\x91\x91\x91\x91\x8f");
		putstrxy(GR_X-1+i*5,GR_Y+GR_H+1, axisXLabelsStr[i-1]);
		for (j=GR_Y; j<GR_Y+GR_H; j++) {
			putlinexy(GR_X+i*5,j, 1, "\x92");
		}
	}

	// Labels Y
	for (i=0; i<3; i++) {
		putstrxy(LABELSY_X, LABELSY_Y+i*3, axisYLabelsStr[i]);
	}

	// Draw fixed graphs
	putstrxy(GR_X+1, GR_Y+4, "\x8e\x8e\x8e\x8c 3.58");
	putstrxy(GR_X+1, GR_Y+7, "\x8e\x8e\x8e\x8e\x8e\x8a 5.37");

	// Information
	putstrxy(3,20, info1Str);
	putstrxy(3,21, info2Str);
	putstrxy(23, 23, "Hold [ESC] to exit");
}

void msx1_drawCpuSpeed()
{
	float speed = calculatedFreq;
	uint16_t speedUnits = (uint16_t)speed;
	float speedDecimal = calculatedFreq - speedUnits;
	char *q = heap_top, *p;

	// Draw counter in top-right border
	gotoxy(3,24);
	cprintf("\x86 %u \x87\x80\x80", im2_counter);

	// Draw % of CPU speed
	p = formatFloat(calculatedFreq * 100.f / MSX_CLOCK , heap_top, 0);
	*p++ = '%';
	*p++ = ' ';
	*p = '\0';
	putstrxy(26,5, heap_top);

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

	// Print speed numbers
	p = formatFloat(speed, q, 2);

	// Print speed line
	putlinexy(GR_X+1, GR_Y+1, 30, heap_top);

	// Print CPU speed in top panel
	memcpy(p, " MHz  ", 7);
	putstrxy(15,5, q);
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