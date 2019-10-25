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

	static DRAM_CONST const uint32_t nixieDigitMap[11];
	static DRAM_CONST const uint32_t colonMap[4];

	static DRAM_CONST const uint32_t dp1 = 0x200;
	static DRAM_CONST const uint32_t dp2 = 0x100;

	virtual uint8_t getSPIMode() { return SPI_MODE3; }
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask) { return colonMap[mask]; }
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit) { return transition == 1 ? 0 : nixieDigitMap[digit]; }
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins) { return pins ^ 0xffffffff; }
};

#endif /* LIBRARIES_NIXIEDRIVER_HV5812NIXIEDRIVER_H_ */
