/*
 * NixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <NixieDriver.h>
#include <SPI.h>

bool ICACHE_RAM_ATTR SoftPWM::off() {
	count = (count + quantum) % 100;
	return count >= onPercent;
}

const uint32_t NixieDriver::callCycleCount = ESP.getCpuFreqMHz() * 1024 / 8;
volatile int NixieDriver::_guard = 0;
NixieDriver *NixieDriver::_handler;

// Maps a digit to an index into the pin map.
byte NixieDriver::digitMap[13] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
};

void NixieDriver::init() {
	guard(true);
	if (!_handler) {
		// First time through, set up interrupt handlers
		timer0_isr_init();
		timer0_attachInterrupt(isr);
		timer0_write(ESP.getCycleCount() + ESP.getCpuFreqMHz() * 1024);	// Wait 2 seconds before displaying
	}
	_handler = this;
	guard(false);
}

// Can't be using any system calls in case some other
// weird system calls are being called, because they can do weird
// things with flash. For the same reason, this function needs to be
// in RAM.
void ICACHE_RAM_ATTR NixieDriver::isr() {
    uint32_t ccount;
    __asm__ __volatile__("esync; rsr %0,ccount":"=a" (ccount));

	timer0_write(ccount + callCycleCount);

	if (_guard == 0) {
		if (_handler) {
			_handler->interruptHandler();
		}
	}
}

void ICACHE_RAM_ATTR NixieDriver::guard(bool flag) {
	noInterrupts();
	flag ? _guard++ : _guard--;
	interrupts();
}

bool ICACHE_RAM_ATTR NixieDriver::calculateFade(unsigned long nowMs) {
	displayPWM.onPercent = fadeOutPWM.onPercent = brightness;
	fadeInPWM.onPercent = 0;

	if ((displayMode == FADE_OUT_IN && nowMs - startFade > FADE_TIME2)
			|| (displayMode != FADE_OUT_IN && nowMs - startFade > FADE_TIME)) {
		// Fading lasts at most FADE_TIME ms
		return false;
	}

	long fadeOutDutyCycle = (long) 100 * (FADE_TIME + startFade - nowMs) / FADE_TIME;
	fadeOutDutyCycle = fadeOutDutyCycle * fadeOutDutyCycle * brightness / 10000;
	long fadeInDutyCycle = (long) 100 * (nowMs - startFade) / FADE_TIME;
	fadeInDutyCycle = fadeInDutyCycle * fadeInDutyCycle * brightness / 10000;

	switch (displayMode) {
	case NO_FADE:
		return false;
		break;
	case NO_FADE_DELAY:
		if (nowMs - startFade > FADE_TIME / 2) {
			return false;
		} else {
			fadeOutPWM.onPercent = 0;
		}
		break;
	case FADE_OUT:
		fadeOutPWM.onPercent = fadeOutDutyCycle;
		break;
	case FADE_IN:
		fadeOutPWM.onPercent = 0;
		fadeInPWM.onPercent = fadeInDutyCycle;
		break;
	case CROSS_FADE:
		fadeOutPWM.onPercent = fadeOutDutyCycle;
		fadeInPWM.onPercent = fadeInDutyCycle;
		break;
	case FADE_OUT_IN:
		fadeInDutyCycle = (long) 100 * (nowMs - startFade - FADE_TIME) / FADE_TIME;
		fadeInDutyCycle = fadeInDutyCycle * fadeInDutyCycle * brightness / 10000;

		if (nowMs - startFade <= FADE_TIME) {
			fadeOutPWM.onPercent = fadeOutDutyCycle;
			fadeInPWM.onPercent = 0;
		} else {
			fadeInPWM.onPercent = fadeInDutyCycle;
			fadeOutPWM.onPercent = 0;
		}
		break;
	}

	return true;
}

void NixieDriver::cacheColonMap() {}
