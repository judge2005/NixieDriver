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

	static DRAM_CONST const uint32_t nixieDigitMap[11];
	static DRAM_CONST const uint32_t colonMap[4];

	static DRAM_CONST const uint32_t dp1 = 0x100000;
	static DRAM_CONST const uint32_t dp2 = 0x200000;

	virtual uint8_t getSPIMode() { return SPI_MODE1; }
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask) { return colonMap[mask]; }
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit) { return transition == 1 ? 0 : nixieDigitMap[digit]; }
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins) { return pins; }
};

#endif /* LIBRARIES_NIXIEDRIVER_HV5523NIXIEDRIVER_H_ */
