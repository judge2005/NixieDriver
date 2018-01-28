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

void NixieDriver::init() {
	guard(true);
	_handler = this;
	timer0_isr_init();
	timer0_attachInterrupt(isr);
	timer0_write(ESP.getCycleCount() + ESP.getCpuFreqMHz() * 1024);	// Wait 2 seconds before displaying
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
		_handler->interruptHandler();
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
		digit = nextDigit;
		return false;
	}

	long fadeOutDutyCycle = 0;
	long fadeInDutyCycle = 0;

	switch (displayMode) {
	case NO_FADE:
		digit = nextDigit;
		return false;
		break;
	case NO_FADE_DELAY:
		if (nowMs - startFade > FADE_TIME / 2) {
			digit = nextDigit;
			return false;
		} else {
			fadeOutPWM.onPercent = 0;
		}
		break;
	case FADE_OUT:
		fadeOutDutyCycle = (long) 100 * (FADE_TIME + startFade - nowMs) / FADE_TIME;
		fadeOutDutyCycle = fadeOutDutyCycle * fadeOutDutyCycle * brightness / 10000;

		fadeOutPWM.onPercent = fadeOutDutyCycle;
		break;
	case FADE_IN:
		fadeInDutyCycle = (long) 100 * (nowMs - startFade) / FADE_TIME;
		fadeInDutyCycle = fadeInDutyCycle * fadeInDutyCycle * brightness / 10000;

		fadeOutPWM.onPercent = 0;
		fadeInPWM.onPercent = fadeInDutyCycle;
		break;
	case CROSS_FADE:
		fadeOutDutyCycle = (long) 100 * (FADE_TIME + startFade - nowMs) / FADE_TIME;
		fadeOutDutyCycle = fadeOutDutyCycle * fadeOutDutyCycle * brightness / 10000;

		fadeInDutyCycle = (long) 100 * (nowMs - startFade) / FADE_TIME;
		fadeInDutyCycle = fadeInDutyCycle * fadeInDutyCycle * brightness / 10000;

		fadeOutPWM.onPercent = fadeOutDutyCycle;
		fadeInPWM.onPercent = fadeInDutyCycle;
		break;
	case FADE_OUT_IN:
		fadeOutDutyCycle = (long) 100 * (FADE_TIME + startFade - nowMs) / FADE_TIME;
		fadeOutDutyCycle = fadeOutDutyCycle * fadeOutDutyCycle * brightness / 10000;

		fadeInDutyCycle = (long) 100 * (nowMs - startFade - FADE_TIME) / FADE_TIME;
		fadeInDutyCycle = fadeInDutyCycle * fadeInDutyCycle * brightness / 10000;

		if (nowMs - startFade <= FADE_TIME) {
			fadeOutPWM.onPercent = fadeOutDutyCycle;
		} else {
			fadeOutPWM.onPercent = 0;
		}

		if (nowMs - startFade >= FADE_TIME) {
			fadeInPWM.onPercent = fadeInDutyCycle;
		} else {
			fadeInPWM.onPercent = 0;
		}
		break;
	}

	return true;
}
