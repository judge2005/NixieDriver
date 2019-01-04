/*
 * HVNixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HVNixieDriver.h>
#include <SPI.h>

void HVNixieDriver::init() {
	displayMode = NO_FADE;
	nextDigit = 0;

	// Initialize HV chip
	SPI.begin();
	SPI.setFrequency(SPI_CLOCK_DIV2);
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(getSPIMode());
	pinMode(LEpin, OUTPUT);
	digitalWrite(LEpin, LOW);

	interruptHandler();

	NixieDriver::init();
}

unsigned long HVNixieDriver::getMultiplexPins() {
	return 0;
}

void HVNixieDriver::interruptHandler() {
	static const uint32_t digitMasks[] = {
		0xfff0,
		0xff0f,
		0xf0ff,
		0x0fff
	};
	static unsigned long prevPinMask = 0;
	unsigned long pinMask = 0;

	displayPWM.onPercent = brightness;
	bool displayOff = displayPWM.off();

	byte mxShift = multiplexCount << 2;
	byte cd = (digit >> mxShift) & 0xf;
	byte nd = (nextDigit >> mxShift) & 0xf;

	if (numMultiplex < 4 || cd != nd) {
		// If we have a full clock, we only want to fade if an individual digit has changed,
		// otherwise we want to fade whenever fade has been set.
		if (calculateFade(millis())) {
			if (!fadeOutPWM.off()) {
				pinMask = getPin(mapDigit(cd));
			}

			if (!fadeInPWM.off()) {
				pinMask |= getPin(mapDigit(nd));
			}
		} else {
			cd = nd;
			digit = (digit & digitMasks[multiplexCount]) | (((uint32_t)cd) << mxShift);
			if (!displayOff) {
				pinMask = getPin(mapDigit(cd));
			}
		}
	} else if (!displayOff) {
		pinMask = getPin(mapDigit(cd));
	}

	cycleCount = (cycleCount + 1) % 20;
	if (cycleCount == 0) {
		multiplexCount = (multiplexCount + 1) % numMultiplex;
	}

	pinMask |= getMultiplexPins();

	if (!displayOff) {
		pinMask |= getPins(colonMask);
	}

	pinMask = convertPolarity(pinMask);

	if (pinMask == prevPinMask) {
		return;
	}

	prevPinMask = pinMask;

	while(SPI1CMD & SPIBUSY) {}
    // Set Bits to transfer
    const uint32_t mask = ~((SPIMMOSI << SPILMOSI) | (SPIMMISO << SPILMISO));
    // 31 is (8*4-1)
    SPI1U1 = ((SPI1U1 & mask) | ((31 << SPILMOSI) | (31 << SPILMISO)));

    union {
            uint32_t l;
            uint8_t b[4];
    } data_;
    data_.l = pinMask;
    SPI1W0 = (data_.b[3] | (data_.b[2] << 8) | (data_.b[1] << 16) | (data_.b[0] << 24));

    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}

	digitalWrite(LEpin, HIGH);
	digitalWrite(LEpin, LOW);
}
