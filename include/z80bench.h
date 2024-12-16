#include <stdint.h>


// ========================================================

static const char titleStr[] = "\x86 Z80 Frequency Benchmark v1.2 (2024) \x87";
static const char authorStr[] = "\x86 NataliaPC \x87";

static const char axisXLabelsStr[6][6] = {
	" 5MHz", "10MHz", "15MHz", "20MHz", "25MHz", "  30"
};
static const char axisYLabelsStr[8][14] = {
	"CPU speed", "(MHz)", "", "MSX standard", "(3.579MHz)", "", "MSX2+ tPANA", "(5.369MHz)"
};

static const char cpuTypesStr[3][12] = {
	"Z80        ", "R800", "Z280       "
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

#define BRAND_OCMPLD	27
static const char *machineBrandStr[] = {
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
