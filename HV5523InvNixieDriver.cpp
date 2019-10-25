/*
 * HV5523NixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HV5523InvNixieDriver.h>
#include <SPI.h>

DRAM_CONST const uint32_t HV5523InvNixieDriver::nixieDigitMap[13] = {
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

DRAM_CONST const uint32_t HV5523InvNixieDriver::colonMap[4] = {
	0,			// none
	0x800000,	// left
	0x400000,	// right
	0xc00000	// both
};

uint32_t HV5523InvNixieDriver::currentColonMap[4] = {
	0,	// none
	0,	// none
	0,	// none
	0	// none
};

uint32_t NIXIE_DRIVER_ISR_FLAG HV5523InvNixieDriver::getPins(byte mask) { return currentColonMap[mask]; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV5523InvNixieDriver::getPin(uint32_t digit) { return transition == 1 ? 0 : nixieDigitMap[digit]; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV5523InvNixieDriver::convertPolarity(uint32_t pins) { return pins ^ 0xffffffff; }

void HV5523InvNixieDriver::cacheColonMap() {
	if (indicator == 0) {
		for (int i=0; i<4; i++) {
			currentColonMap[i] = 0;
		}
	} else if (indicator == 1) {
		for (int i=0; i<4; i++) {
			currentColonMap[i] = colonMap[i];
		}
	} else {
		currentColonMap[0] = 0;
		currentColonMap[1] = nixieDigitMap[mapDigit(10)];
		currentColonMap[2] = nixieDigitMap[mapDigit(11)];
		currentColonMap[3] = nixieDigitMap[mapDigit(10)] | nixieDigitMap[mapDigit(11)];
	}
}
