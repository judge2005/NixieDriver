/*
 * HV5812NixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_HV5812NIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_HV5812NIXIEDRIVER_H_

#include <HVNixieDriver.h>
#include <SPI.h>

class HV5812NixieDriver : public HVNixieDriver {
public:
	HV5812NixieDriver(int LEpin) : HVNixieDriver(LEpin) {}
	virtual ~HV5812NixieDriver() {}

protected:

	static const unsigned long nixieDigitMap[11];
	static const unsigned long colonMap[4];

	static const unsigned long dp1 = 0x200;
	static const unsigned long dp2 = 0x100;

	virtual uint8_t getSPIMode() { return SPI_MODE3; }
	virtual unsigned long ICACHE_RAM_ATTR getPins(byte mask) { return colonMap[mask]; }
	virtual unsigned long ICACHE_RAM_ATTR getPin(uint16_t digit) { return transition == 1 ? 0 : nixieDigitMap[digit]; }
	virtual unsigned long ICACHE_RAM_ATTR convertPolarity(unsigned long pins) { return pins ^ 0xffffffff; }
};

#endif /* LIBRARIES_NIXIEDRIVER_HV5812NIXIEDRIVER_H_ */
