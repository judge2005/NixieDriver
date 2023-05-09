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
	virtual uint64_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask) = 0;
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit) = 0;
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getMultiplexPins();
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins) = 0;
	virtual bool NIXIE_DRIVER_ISR_FLAG calculateFade(uint32_t nowMs);
	virtual void NIXIE_DRIVER_ISR_FLAG interruptHandler();
};

#endif /* LIBRARIES_NIXIEDRIVER_HVNIXIEDRIVER_H_ */
