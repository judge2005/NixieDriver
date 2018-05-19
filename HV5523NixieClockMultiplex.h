/*
 * HV5523NixieClockMultiplex.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIECLOCK_HV5523NIXIECLOCKMULTIPLEX_H_
#define LIBRARIES_NIXIECLOCK_HV5523NIXIECLOCKMULTIPLEX_H_

#include <HV5523InvNixieDriver.h>
//#include <SPI.h>

class HV5523NixieClockMultiplex : public HV5523InvNixieDriver {
public:
	HV5523NixieClockMultiplex(int LEpin, int numMultiplex) : HV5523InvNixieDriver(LEpin) {
		displayPWM.quantum = 7;
		fadeInPWM.quantum = 6;
		fadeOutPWM.quantum = 6;
		this->numMultiplex = numMultiplex;
	}
	virtual ~HV5523NixieClockMultiplex() {}

protected:
	static const unsigned long multiplexMap[6];

	virtual unsigned long ICACHE_RAM_ATTR getPins(byte mask) {
		if (colonMask & (1 << multiplexCount)) {
			return 0x100000;
		} else {
			return 0;
		}
	}

	virtual unsigned long ICACHE_RAM_ATTR getMultiplexPins() {
		if (cycleCount >= 2) {
			return multiplexMap[multiplexCount];
		} else {
			return 0;
		}
	}
};

#endif /* LIBRARIES_NIXIECLOCK_HV5523NIXIECLOCKMULTIPLEX_H_ */
