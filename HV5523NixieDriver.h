/*
 * HV5523NixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_HV5523NIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_HV5523NIXIEDRIVER_H_

#include <HVNixieDriver.h>
#include <SPI.h>

class HV5523NixieDriver : public HVNixieDriver {
public:
	HV5523NixieDriver(int LEpin) : HVNixieDriver(LEpin) {}
	virtual ~HV5523NixieDriver() {}

protected:

	static const unsigned long nixieDigitMap[11];
	static const unsigned long colonMap[4];

	static const unsigned long dp1 = 0x100000;
	static const unsigned long dp2 = 0x200000;

	virtual uint8_t getSPIMode() { return SPI_MODE1; }
	virtual unsigned long ICACHE_RAM_ATTR getPins(byte mask) { return colonMap[mask]; }
	virtual unsigned long ICACHE_RAM_ATTR getPin(uint32_t digit) { return transition == 1 ? 0 : nixieDigitMap[digit]; }
	virtual unsigned long ICACHE_RAM_ATTR convertPolarity(unsigned long pins) { return pins; }
};

#endif /* LIBRARIES_NIXIEDRIVER_HV5523NIXIEDRIVER_H_ */
