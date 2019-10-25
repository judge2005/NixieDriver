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

	static DRAM_CONST const uint32_t nixieDigitMap[11];
	static DRAM_CONST const uint32_t colonMap[4];
	static DRAM_CONST const uint32_t multiplexMap[6];

	static DRAM_CONST const uint32_t dp1 = 0x100000;
	static DRAM_CONST const uint32_t dp2 = 0x200000;

	byte numMultiplex = 1;
	byte multiplexCount = 0;
	byte cycleCount = 0;
	virtual uint8_t getSPIMode() { return SPI_MODE1; }
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask) { return colonMap[mask]; }
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit) {
		return nixieDigitMap[(digit >> (multiplexCount << 2)) & 0xf];
	}
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getMultiplexPins() {
		uint32_t pins = 0;
		if (cycleCount >= 1) {
			pins = multiplexMap[multiplexCount];
		}

		cycleCount = (cycleCount + 1) % 6;
		if (cycleCount == 0) {
			multiplexCount = (multiplexCount + 1) % numMultiplex;
		}

		return pins;
	}
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins) { return pins; }
};

#endif /* LIBRARIES_NIXIEDRIVER_HV5523NIXIEDRIVERMULTIPLEX_H_ */
