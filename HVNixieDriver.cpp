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

void HVNixieDriver::interruptHandler() {
#ifndef TEST_PATTERN
	unsigned long pinMask = 0;

	bool displayOff = displayPWM.off();

	if (calculateFade(millis())) {
		if (!fadeOutPWM.off()) {
			pinMask |= getPin(digit);
		}

		if (!fadeInPWM.off()) {
			pinMask |= getPin(nextDigit);
		}
	} else if (!displayOff) {
		pinMask |= getPin(digit);
	}

	if (!displayOff) {
		pinMask |= getPins(colonMask);
	}

	pinMask = convertPolarity(pinMask);

#define USE_WRITE
#ifdef USE_WRITE
	byte bytes[4];
	bytes[0] = (pinMask >> 24) & 0xff;
	bytes[1] = (pinMask >> 16) & 0xff;
	bytes[2] = (pinMask >> 8) & 0xff;
	bytes[3] = pinMask & 0xff;

	SPI.writeBytes(bytes, 4);

#else
	SPI.transfer((pinMask >> 24) & 0xff); // HVout 32 through 25 = pins 17 through 10
	SPI.transfer((pinMask >> 16) & 0xff);// HVout 24 through 17 = pins 9 through 2
	SPI.transfer((pinMask >> 8) & 0xff);// HVout 16 through 9 = pins 1 and 44 through 38
	SPI.transfer(pinMask & 0xff);// HVout 8 through 1 = pins 37 through 30
#endif

#else
	SPI.transfer(0xaa);
	SPI.transfer(0xaa);
	SPI.transfer(0xaa);
	SPI.transfer(0xaa);
#endif

	digitalWrite(LEpin, HIGH);
	digitalWrite(LEpin, LOW);
}
