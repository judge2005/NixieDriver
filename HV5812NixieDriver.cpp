/*
 * HV5812NixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HV5812NixieDriver.h>
#include <SPI.h>

DRAM_CONST const uint32_t HV5812NixieDriver::nixieDigitMap[11] = {
		0x80000,	// 0
		0x400,		// 1
		0x800,		// 2
		0x1000,		// 3
		0x2000,		// 4
		0x4000,		// 5
		0x8000,		// 6
		0x10000,	// 7
		0x20000,	// 8
		0x40000,	// 9
		0x0			// nothing
};

DRAM_CONST const uint32_t HV5812NixieDriver::colonMap[4] = {
	0,		// 0
	0x80,	// 1
	0x40,	// 2
	0xc0	// 3
};
