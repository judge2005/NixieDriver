/*
 * HV5523NixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_HV5523INV15SEGNIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_HV5523INV15SEGNIXIEDRIVER_H_

#include <HV5523InvNixieDriver.h>

class HV5523Inv15SegNixieDriver : public HV5523InvNixieDriver {
public:
	HV5523Inv15SegNixieDriver(int LEpin) : HV5523InvNixieDriver(LEpin) {}
	virtual ~HV5523Inv15SegNixieDriver() {}

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
	static const unsigned long nixieDigitMap[13];
	static const unsigned long circularWipeMap[8];
	static const unsigned long circularWipeEraseMap[8];
	static const unsigned long circularWipeRevealMap[8];

	virtual void cacheColonMap();

	virtual unsigned long ICACHE_RAM_ATTR getPin(uint32_t digit) {
		switch (transition) {
		case 0:	// Regular display
			return 	nixieDigitMap[digit];
		case 1:	// Blank
			return 0;
		case 2:	// Wipe
			uint32_t index = transitionCount % 8;
			if (transitionCount < 8) {
				return (nixieDigitMap[oldDigit] | circularWipeMap[index]) & circularWipeEraseMap[index];
			} else {
				return (nixieDigitMap[oldDigit] | circularWipeMap[index]) & circularWipeRevealMap[index];
			}
		}
	}
};

#endif /* LIBRARIES_NIXIEDRIVER_HV5523INV15SEGNIXIEDRIVER_H_ */
