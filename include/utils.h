/*
	Copyright (c) 2024 Natalia Pujol Cremades
	info@abitwitches.com

	See LICENSE file.
*/
#pragma once

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdbool.h>


uint8_t getRomByte(uint16_t address) __sdcccall(1);
void click();
void die(const char *s, ...);
void exit(void);


void basic_play(void *parameters) __sdcccall(1);


char *formatFloat(float value, char *txt, int8_t decimals);


#define MODE_ANK		0
#define MODE_KANJI0		1
#define MODE_KANJI1		2
#define MODE_KANJI2		3
#define MODE_KANJI3		4
bool detectKanjiDriver() __z88dk_fastcall;
char getKanjiMode() __sdcccall(1);
void setKanjiMode(uint8_t mode) __z88dk_fastcall;

uint8_t detectVDP() __sdcccall(1);
bool detectR800() __sdcccall(0);
bool detectZ280() __sdcccall(0);
bool detectNMOS() __sdcccall(1);
bool detectTurboPana() __z88dk_fastcall;
bool setTurboPana(bool enabled) __sdcccall(1);
bool detectTurboR() __z88dk_fastcall;
void setCpuTurboR(uint8_t mode) __z88dk_fastcall;
uint8_t getCpuTurboR() __sdcccall(1);
void setNTSC(bool enabled) __sdcccall(1);

#define TIDES_3_57MHZ	0
#define TIDES_6_66MHZ	1
#define TIDES_10MHZ		2
#define TIDES_20MHZ		3
#define TIDES_SLOTS357	4
bool detectTidesRider() __sdcccall(1);
void setTidesSpeed(uint8_t speed) __z88dk_fastcall;

uint8_t getRegisterRTC(uint8_t reg, uint8_t block);
void setRegisterRTC(uint8_t reg, uint8_t block, uint8_t value);


#endif//__UTILS_H__
