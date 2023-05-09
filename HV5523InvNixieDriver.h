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

	static DRAM_CONST const uint32_t nixieDigitMap[13];
	static DRAM_CONST const uint32_t colonMap[4];
	static uint32_t currentColonMap[4];

	static DRAM_CONST const uint32_t dp1 = 0x100000;
	static DRAM_CONST const uint32_t dp2 = 0x200000;

	virtual uint8_t getSPIMode() { return SPI_MODE1; }
	virtual void cacheColonMap();
	virtual uint64_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask);
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit);
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins);
};

#endif /* LIBRARIES_NIXIEDRIVER_HV5523INVNIXIEDRIVER_H_ */
