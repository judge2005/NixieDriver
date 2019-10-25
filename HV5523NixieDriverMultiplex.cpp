/*
 * HV5523NixieDriverMultiplex.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HV5523NixieDriverMultiplex.h>
#include <SPI.h>

DRAM_CONST const uint32_t HV5523NixieDriverMultiplex::nixieDigitMap[11] = {
	0x400,		// 0
	0x80000,	// 1
	0x40000,	// 2
	0x20000,	// 3
	0x10000,	// 4
	0x8000,		// 5
	0x4000,		// 6
	0x2000,		// 7
	0x1000,		// 8
	0x800,		// 9
	0x0, 		// Nothing
};

DRAM_CONST const uint32_t HV5523NixieDriverMultiplex::colonMap[4] = {
	0,			// 0
	0x400000,	// 1
	0x800000,	// 2
	0xc00000	// 3
};

DRAM_CONST const uint32_t HV5523NixieDriverMultiplex::multiplexMap[6] = {
	0x1000000,	// 1
	0x2000000,	// 2
	0x4000000,	// 3
	0x8000000,	// 4
	0,	// 5
	0	// 6
};


