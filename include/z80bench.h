#include <stdint.h>


// ========================================================

const char titleStr[] = "\x86 Z80 Frequency Benchmark v1.3 \x87";
const char authorStr[] = "\x86 NataliaPC \x87";
const char infoMachineStr[] = "Machine   : ";
const char infoCpuTypeStr[] = "CPU Type  : ";
const char infoVdpTypeStr[] = "VDP Type  : ";

static const char *axisXLabelsStr[] = {
	" 5MHz", "10MHz", "15MHz", "20MHz", "25MHz", "  30"
};
static const char *axisYLabelsStr[] = {
	"CPU speed", "(MHz)", "", "MSX standard", "(3.579MHz)", "", "MSX2+ tPANA", "(5.369MHz)"
};

const char *cpuTypesStr[] = {
	"Z80        ", "R800", "Z280       "
};

const char *vdpTypeStr[] = {
	"TMS9918A", "V9938", "V9958"
};

static const char *turboRmodeStr[] = {
	"", "ROM", "DRAM"
};

const char *vdpModesStr[] = {
	"(PAL 50Hz) ", "(NTSC 60Hz)"
};

const char *machineTypeStr[] = {
	// [0-3] ROM Byte $2D
	"MSX1", "MSX2", "MSX2+", "MSX TurboR"
};

#define BRAND_OCMPLD	27
const char *machineBrandStr[] = {
	// [0] Unknown
	"",
	// [1-26] I/O port $40
	"ASCII", "Canon", "Casio", "Fujitsu", "General", "Hitachi", "Kyocera", "Panasonic", "Mitsubishi", "NEC", 
	"Yamaha", "JVC", "Philips", "Pioneer", "Sanyo", "Sharp", "Sony", "Spectravideo", "Toshiba", "Mitsumi", 
	"Telematica", "Gradiente", "Sharp Brasil", "GoldStar", "Daewoo", "Samsung",
	// [27] OCM/MSX++
	"OCM/MSX++"
};

static const char *speedLineStr = "         \x92         \x92         \x92         \x92         \x92         \x92";
const char *info1Str = "Clock measurement is approximate.";
const char *info2Str = "May vary with external RAM mappers.";


static const float speedDecLimits[] = { .07f, .14f, .21f, .28f, .35f, .42f, .5f };

static const uint8_t ocmSmartCmd[] = {
	OCM_SMART_CPU358MHz, OCM_SMART_TurboPana, OCM_SMART_CPU410MHz, OCM_SMART_CPU448MHz, OCM_SMART_CPU490MHz,
	OCM_SMART_CPU539MHz, OCM_SMART_CPU610MHz, OCM_SMART_CPU696MHz, OCM_SMART_CPU806MHz
};


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
