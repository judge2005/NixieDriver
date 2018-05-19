/*
 * HV5523NixieClockMultiplex.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HV5523NixieClockMultiplex.h>
//#include <SPI.h>

const unsigned long HV5523NixieClockMultiplex::multiplexMap[6] = {
	0x80,	// 1 - HV13
	0x100,	// 2 - HV14
	0x200,	// 3 - HV15
	0x200000,	// 4 - HV12
	0,	// 5
	0	// 6
};
