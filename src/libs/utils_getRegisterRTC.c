#include "utils.h"
#include "conio.h"
#include "msx_const.h"


uint8_t getRegisterRTC(uint8_t reg, uint8_t block)
{
	ASM_DI;
	outportb(0xb4, 13);
	outportb(0xb5, block);
	outportb(0xb4, reg);
	uint8_t result = inportb(0xb5);
	ASM_EI;
	return result;
}
