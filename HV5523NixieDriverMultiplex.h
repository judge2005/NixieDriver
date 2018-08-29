/*
 * HV5523NixieDriverMultiplex.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_HV5523NIXIEDRIVERMULTIPLEX_H_
#define LIBRARIES_NIXIEDRIVER_HV5523NIXIEDRIVERMULTIPLEX_H_

#include <HVNixieDriver.h>
#include <SPI.h>

class HV5523NixieDriverMultiplex : public HVNixieDriver {
public:
	HV5523NixieDriverMultiplex(int LEpin, int numMultiplex) : HVNixieDriver(LEpin), numMultiplex(numMultiplex) {}
	virtual ~HV5523NixieDriverMultiplex() {}

protected:

	static const unsigned long nixieDigitMap[11];
	static const unsigned long colonMap[4];
	static const unsigned long multiplexMap[6];

	static const unsigned long dp1 = 0x100000;
	static const unsigned long dp2 = 0x200000;

	byte numMultiplex = 1;
	byte multiplexCount = 0;
	byte cycleCount = 0;
	virtual uint8_t getSPIMode() { return SPI_MODE1; }
	virtual unsigned long ICACHE_RAM_ATTR getPins(byte mask) { return colonMap[mask]; }
	virtual unsigned long ICACHE_RAM_ATTR getPin(uint32_t digit) {
		return nixieDigitMap[(digit >> (multiplexCount << 2)) & 0xf];
	}
	virtual unsigned long ICACHE_RAM_ATTR getMultiplexPins() {
		unsigned long pins = 0;
		if (cycleCount >= 1) {
			pins = multiplexMap[multiplexCount];
		}

		cycleCount = (cycleCount + 1) % 6;
		if (cycleCount == 0) {
			multiplexCount = (multiplexCount + 1) % numMultiplex;
		}

		return pins;
	}
	virtual unsigned long ICACHE_RAM_ATTR convertPolarity(unsigned long pins) { return pins; }
};

#endif /* LIBRARIES_NIXIEDRIVER_HV5523NIXIEDRIVERMULTIPLEX_H_ */
