/*
 * HVNixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_HVNIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_HVNIXIEDRIVER_H_

#include <NixieDriver.h>

class HVNixieDriver : public NixieDriver {
public:
	HVNixieDriver(int LEpin) : LEpin(LEpin) {}
	virtual void init();
	virtual ~HVNixieDriver() {}

protected:
	int LEpin;

	virtual uint8_t getSPIMode() = 0;
	virtual unsigned long ICACHE_RAM_ATTR getPins(byte mask) = 0;
	virtual unsigned long ICACHE_RAM_ATTR getPin(byte digit) = 0;
	virtual unsigned long ICACHE_RAM_ATTR convertPolarity(unsigned long pins) = 0;
	virtual void ICACHE_RAM_ATTR interruptHandler();
};

#endif /* LIBRARIES_NIXIEDRIVER_HVNIXIEDRIVER_H_ */
