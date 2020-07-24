/*
 * HV5523NixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HV5523Inv15SegNixieDriver.h>

// For eventually making our own characters
#ifdef B7971
DRAM_CONST const uint32_t kMap[17] = {
	0,			// So we are base 1
	0x100000,	// K1 - HV11
	0x10000,	// K2 - HV7
	0x20000,	// K3 - HV8
	0x80000,	// K4 - HV10
	0x100,		// K5 - HV14
	0x400,		// K6 - HV1
	0x800,		// K7 - HV2
	0x2000,		// K8 - HV4
	0x200,		// K9 - HV15
	0x200000,	// K10 - HV12
	0x8000,		// K11 - HV6
	0x40000,	// K12 - HV9
	0x1000,		// K13 - HV3
	0x80,		// K14 - HV13
	0x4000,		// K15 - HV5
	0			// No such segment
};
#else
DRAM_CONST const uint32_t kMap[17] = {
	0,			// So we are base 1
	0x800,		// K1 - HV2
	0x200,		// K2 - HV13
	0x4000,		// K3 - HV14
	0x80000,	// K4 - HV10
	0x100,		// K5 - HV5
	0x80,		// K6 - HV15
	0x10000,	// K7 - HV12
	0x40000,	// K8 - HV9
	0x200000,	// K9 - HV7
	0x8000,		// K10 - HV1
	0x100000,	// K11 - HV3
	0x20000,	// K12 - HV8
	0x1000,		// K13 - HV11
	0x400,		// K14 - HV6
	0x2000,		// K15 - HV4
	0			// No such segment
};
#endif
// Circular wipe
DRAM_CONST const uint32_t HV5523Inv15SegNixieDriver::circularWipeMap[8] = {
	kMap[8],		// K8 - HV4
	kMap[9],		// K9 - HV15
	kMap[10],	// K10 - HV12
	kMap[11],		// K11 - HV6
	kMap[12],	// K12 - HV9
	kMap[13],		// K13 - HV3
	kMap[14],		// K14 - HV13
	kMap[7],		// K7 - HV2
};

DRAM_CONST const uint32_t HV5523Inv15SegNixieDriver::circularWipeEraseMap[8] = {
	0xffffffff ^ 0x0,		// Erase nothing
	0xffffffff ^ (kMap[8] | kMap[1]),	// Erase K8 + K1
	0xffffffff ^ (kMap[8] | kMap[1] | kMap[9] | kMap[2]),	// Erase K8 + K1 + K9 + K2
	0xffffffff ^ (kMap[8] | kMap[1] | kMap[9] | kMap[2] | kMap[10] | kMap[3]),	// Erase K8 + K1 + K9 + K2 + K10 + K3
	0xffffffff ^ (kMap[8] | kMap[1] | kMap[9] | kMap[2] | kMap[10] | kMap[3] | kMap[11]),	// Erase K8 + K1 + K9 + K2 + K10 + K3 + K11
	0xffffffff ^ (kMap[8] | kMap[1] | kMap[9] | kMap[2] | kMap[10] | kMap[3] | kMap[11] | kMap[12] | kMap[4]),	// Erase K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12 + K4
	0xffffffff ^ (kMap[8] | kMap[1] | kMap[9] | kMap[2] | kMap[10] | kMap[3] | kMap[11] | kMap[12] | kMap[4] | kMap[13] | kMap[5]),	// Erase K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12 + K4 + K13 + K5
	0xffffffff ^ (kMap[8] | kMap[1] | kMap[9] | kMap[2] | kMap[10] | kMap[3] | kMap[11] | kMap[12] | kMap[4] | kMap[13] | kMap[5] | kMap[14] | kMap[6]),	// Erase K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12 + K4 + K13 + K5 + K14 + K6
};

DRAM_CONST const uint32_t HV5523Inv15SegNixieDriver::circularWipeRevealMap[8] = {
	kMap[8],		// Erase everything but K8
	kMap[8] | kMap[1] | kMap[9],	// Erase everything but K8 + K1 + K9
	kMap[8] | kMap[1] | kMap[9] | kMap[2] | kMap[10],	// Erase everything but K8 + K1 + K9 + K2 + K10
	kMap[8] | kMap[1] | kMap[9] | kMap[2] | kMap[10] | kMap[3] | kMap[11],	// Erase everything but K8 + K1 + K9 + K2 + K10 + K3 + K11
	kMap[8] | kMap[1] | kMap[9] | kMap[2] | kMap[10] | kMap[3] | kMap[11] | kMap[12],	// Erase everything but K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12
	kMap[8] | kMap[1] | kMap[9] | kMap[2] | kMap[10] | kMap[3] | kMap[11] | kMap[12] | kMap[4] | kMap[13],	// Erase everything but K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12 + K4 + K13
	kMap[8] | kMap[1] | kMap[9] | kMap[2] | kMap[10] | kMap[3] | kMap[11] | kMap[12] | kMap[4] | kMap[13] | kMap[5] | kMap[14],	// Erase everything but K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12 + K4 + K13 + K5 + K14
	0xffffffff,	// Erase nothing
};

DRAM_CONST const uint32_t HV5523Inv15SegNixieDriver::nixieDigitMap[13] = {
	kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[5] | kMap[6] | kMap[9] | kMap[13],	// 0
	kMap[8] | kMap[12],	// 1
	kMap[1] | kMap[2] | kMap[4] | kMap[10] | kMap[13],	// 2
	kMap[1] | kMap[3] | kMap[4] | kMap[9] | kMap[10],	// 3
	kMap[2] | kMap[3] | kMap[6] | kMap[10] | kMap[14],	// 4
	kMap[1] | kMap[4] | kMap[6] | kMap[11] | kMap[14],	// 5
	kMap[1] | kMap[3] | kMap[4] | kMap[5] | kMap[6] | kMap[10] | kMap[14],	// 6
	kMap[1] | kMap[9] | kMap[12],	// 7
	kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[5] | kMap[6] | kMap[10] | kMap[14],	// 8
	kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[6] | kMap[10] | kMap[14],	// 9
	kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[5] | kMap[6] | kMap[7] | kMap[8] | kMap[9] | kMap[10] | kMap[11] | kMap[12] | kMap[13] | kMap[14] | kMap[15] | kMap[10],	// Everything
	kMap[15],	// _
	0x0, 		// Nothing
};

void HV5523Inv15SegNixieDriver::cacheColonMap() {
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
		currentColonMap[1] = 0;
		currentColonMap[2] = nixieDigitMap[11];
		currentColonMap[3] = nixieDigitMap[11];
	}
}
