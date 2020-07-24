/*
 * SN74HC595_7SegNixieDriver.h
 *
 *  Created on: Apr 28, 2020
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_SN74HC595_7SEGNIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_SN74HC595_7SEGNIXIEDRIVER_H_

#include <HVNixieDriver.h>
#include <SPI.h>

class SN74HC595_7SegNixieDriver : public HVNixieDriver {
public:
	SN74HC595_7SegNixieDriver(int LEpin) : HVNixieDriver(LEpin) {}
	virtual void init();
	virtual ~SN74HC595_7SegNixieDriver() {}

	virtual bool setTransition(const byte transition, uint32_t nextClockDigit) {
		bool ret = true;
		if (this->transition != transition) {
			oldDigit = digit;
			this->transition = transition;
			transitionCount = 0;
		} else if (transition != 0) {
			nextDigit = digit = nextClockDigit;
			transitionCount++;
			if (transitionCount == 16) {
				ret = false;
			}

			if (transitionCount >= 8) {
				oldDigit = nextClockDigit;
			}
		}

		return ret;
	}

protected:
	uint32_t oldDigit = 0;
	static DRAM_CONST const uint32_t nixieDigitMap[13];
	static DRAM_CONST const uint32_t circularWipeMap[8];
	static DRAM_CONST const uint32_t circularWipeEraseMap[8];
	static DRAM_CONST const uint32_t circularWipeRevealMap[8];

	virtual uint8_t getSPIMode() { return SPI_MODE0; }

	virtual void NIXIE_DRIVER_ISR_FLAG interruptHandler();
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins);
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask);
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit);
};

#endif /* LIBRARIES_NIXIEDRIVER_SN74HC595_7SEGNIXIEDRIVER_H_ */
