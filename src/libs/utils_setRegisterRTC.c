#include "utils.h"
#include "conio.h"
#include "msx_const.h"


void setRegisterRTC(uint8_t reg, uint8_t block, uint8_t value)
{
	ASM_DI;
	outportb(0xb4, 13);
	outportb(0xb5, block);
	outportb(0xb4, reg);
	outportb(0xb5, value);
	ASM_EI;
}
