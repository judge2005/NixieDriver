/*
 * HV5523NixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HV5523InvNixieDriver.h>
#include <SPI.h>

const unsigned long HV5523InvNixieDriver::nixieDigitMap[13] = {
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
	0x100000,	// dp1
	0x200000,	// dp2
	0x0, 		// Nothing
};

const unsigned long HV5523InvNixieDriver::colonMap[4] = {
	0,			// none
	0x800000,	// left
	0x400000,	// right
	0xc00000	// both
};
