#pragma once
#include <stdint.h>
#include <stdbool.h>


void msx1_drawCpuSpeed();
void msx1_drawPanel();
void msx1_showCPUtype();
void msx1_showVDPtype();

void msx1_textattr(uint16_t attr) __z88dk_fastcall;
void msx1_textblink(uint8_t x, uint8_t y, uint16_t length, bool enabled);
