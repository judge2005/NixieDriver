/*
 * HV9808NixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HV9808NixieDriver.h>
#include <SPI.h>

const unsigned long HV9808NixieDriver::nixieDigitMap[11] = {
	0x8000,	// 0
	0x1000000,	// 1
	0x800000,	// 2
	0x400000,	// 3
	0x200000,	// 4
	0x100000,	// 5
	0x80000,	// 6
	0x40000,	// 7
	0x20000,	// 8
	0x10000,	// 9
	0x0, // Nothing
};

const unsigned long HV9808NixieDriver::colonMap[4] = {
	0,			// 0
	0x8000000,	// 1
	0x10000000,	// 2
	0x18000000	// 3
};
