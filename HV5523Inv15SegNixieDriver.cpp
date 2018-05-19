/*
 * HV5523NixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HV5523Inv15SegNixieDriver.h>

// For eventually making our own characters
const unsigned long kMap[16] = {
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

// Circular wipe
const unsigned long HV5523Inv15SegNixieDriver::circularWipeMap[8] = {
	0x2000,		// K8 - HV4
	0x0200,		// K9 - HV15
	0x200000,	// K10 - HV12
	0x8000,		// K11 - HV6
	0x40000,	// K12 - HV9
	0x1000,		// K13 - HV3
	0x80,		// K14 - HV13
	0x800,		// K7 - HV2
};

const unsigned long HV5523Inv15SegNixieDriver::circularWipeEraseMap[8] = {
	0xffffffff ^ 0x0,		// Erase nothing
	0xffffffff ^ 0x102000,	// Erase K8 + K1
	0xffffffff ^ 0x112200,	// Erase K8 + K1 + K9 + K2
	0xffffffff ^ 0x332200,	// Erase K8 + K1 + K9 + K2 + K10 + K3
	0xffffffff ^ 0x33a200,	// Erase K8 + K1 + K9 + K2 + K10 + K3 + K11
	0xffffffff ^ 0x3fa200,	// Erase K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12 + K4
	0xffffffff ^ 0x3fb300,	// Erase K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12 + K4 + K13 + K5
	0xffffffff ^ 0x3f9780,	// Erase K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12 + K4 + K13 + K5 + K14 + K6
};

const unsigned long HV5523Inv15SegNixieDriver::circularWipeRevealMap[8] = {
	0x2000,		// Erase everything but K8
	0x102200,	// Erase everything but K8 + K1 + K9
	0x312200,	// Erase everything but K8 + K1 + K9 + K2 + K10
	0x33a200,	// Erase everything but K8 + K1 + K9 + K2 + K10 + K3 + K11
	0x37a200,	// Erase everything but K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12
	0x3fb200,	// Erase everything but K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12 + K4 + K13
	0x3fb380,	// Erase everything but K8 + K1 + K9 + K2 + K10 + K3 + K11 + K12 + K4 + K13 + K5 + K14
	0xffffffff,	// Erase nothing
};

const unsigned long HV5523Inv15SegNixieDriver::nixieDigitMap[13] = {
	0x1B1700,	// 0
	0x42000,	// 1
	0x390180,	// 2
	0x3A0200,	// 3
	0x230480,	// 4
	0x188480,	// 5
	0x3A0580,	// 6
	0x101200,	// 7
	0x3B0580,	// 8
	0x3B0480,	// 9
	0x24BA80,	// *
	0x4000,		// _
	0x0, 		// Nothing
};

