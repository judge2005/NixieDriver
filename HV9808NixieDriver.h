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

	static DRAM_CONST const uint32_t nixieDigitMap[11];
	static DRAM_CONST const uint32_t colonMap[4];

	static DRAM_CONST const uint32_t dp1 = 0x2000000;
	static DRAM_CONST const uint32_t dp2 = 0x4000000;

	virtual uint8_t getSPIMode() { return SPI_MODE0; }
	virtual uint64_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask) { return colonMap[mask]; }
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit) { return transition == 1 ? 0 : nixieDigitMap[digit]; }
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins) { return pins; }
};

#endif /* LIBRARIES_NIXIEDRIVER_HV9808NIXIEDRIVER_H_ */
