#include <stdint.h>
#include <stdbool.h>
#include "utils.h"
#include "conio.h"


// MSXnano and MSXimus FPGA boards: https://github.com/Papipapito
// They have the config device of the goa'uld core: ID 0x48 in the expanded
// I/O port 0x40, read back as 0xB7. Their core version is in I/O port 0x2F
// (BCD) and its third digit in I/O port 0x29 (1-15, 0xFF if not present):
//   0x1X-0x2X  MSXnano 1.X-2.X (Tang Nano 20K)
//   0x3X-0x9X  MSXimus 3.X (Tang Console 60K and 138K)
//   0xA0-0xFE  MSXimus Z (Zynq XC7Z020), 0xA0 = Z1.0, 0xA2 = Z1.2...
// The 60K and 138K cores answer the same. Only the 60K core (since 3.7.2) has
// the last gamepad report in I/O ports 0x20-0x27, the 138K core reads 0xFF.
uint8_t detectFpgaBoard()
{
	uint8_t port40 = ~inportb(0x40);			// back up the current device
	outportb(0x40, 0x48);						// goa'uld config device
	bool found = (inportb(0x40) == 0xb7);
	outportb(0x40, port40);						// restore the original device
	if (!found) return FPGA_NONE;

	uint8_t version = inportb(0x2f);
	if (version == 0xff || version < 0x10) return FPGA_NONE;	// no version port
	if (version >= 0xa0) return FPGA_MSXIMUS_Z;
	if (version < 0x30) return FPGA_MSXNANO;

	for (uint8_t port = 0x20; port < 0x28; port++) {
		if (inportb(port) != 0xff) return FPGA_MSXIMUS_60K;
	}
	uint8_t patch = inportb(0x29);
	if (version > 0x37 || (version == 0x37 && patch >= 2 && patch <= 15)) {
		return FPGA_MSXIMUS_138K;				// a 60K core would have 0x20-0x27
	}
	return FPGA_MSXIMUS;						// older core: 60K or 138K
}
