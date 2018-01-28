/*
 * HV9808NixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_HV9808NIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_HV9808NIXIEDRIVER_H_

#include <HVNixieDriver.h>
#include <SPI.h>

class HV9808NixieDriver : public HVNixieDriver {
public:
	HV9808NixieDriver(int LEpin) : HVNixieDriver(LEpin) {}
	virtual ~HV9808NixieDriver() {}

protected:

	static const unsigned long nixieDigitMap[11];
	static const unsigned long colonMap[4];

	static const unsigned long dp1 = 0x2000000;
	static const unsigned long dp2 = 0x4000000;

	virtual uint8_t getSPIMode() { return SPI_MODE0; }
	virtual unsigned long ICACHE_RAM_ATTR getPins(byte mask) { return colonMap[mask]; }
	virtual unsigned long ICACHE_RAM_ATTR getPin(byte digit) { return nixieDigitMap[digit]; }
	virtual unsigned long ICACHE_RAM_ATTR convertPolarity(unsigned long pins) { return pins; }
};

#endif /* LIBRARIES_NIXIEDRIVER_HV9808NIXIEDRIVER_H_ */
