/*
 * HV5523NixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_HV5523INVNIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_HV5523INVNIXIEDRIVER_H_

#include <HVNixieDriver.h>
#include <SPI.h>

class HV5523InvNixieDriver : public HVNixieDriver {
public:
	HV5523InvNixieDriver(int LEpin) : HVNixieDriver(LEpin) {}
	virtual ~HV5523InvNixieDriver() {}

protected:

	static const unsigned long nixieDigitMap[13];
	static const unsigned long colonMap[4];
	static unsigned long currentColonMap[4];

	static const unsigned long dp1 = 0x100000;
	static const unsigned long dp2 = 0x200000;

	virtual uint8_t getSPIMode() { return SPI_MODE1; }
	virtual void cacheColonMap();
	virtual unsigned long ICACHE_RAM_ATTR getPins(byte mask) { return currentColonMap[mask]; }
	virtual unsigned long ICACHE_RAM_ATTR getPin(uint32_t digit) { return transition == 1 ? 0 : nixieDigitMap[digit]; }
	virtual unsigned long ICACHE_RAM_ATTR convertPolarity(unsigned long pins) { return pins ^ 0xffffffff; }
};

#endif /* LIBRARIES_NIXIEDRIVER_HV5523INVNIXIEDRIVER_H_ */
