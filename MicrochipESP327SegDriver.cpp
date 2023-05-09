/*
 * MicrochipESP327SegDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <SPI.h>
#include "MicrochipESP327SegDriver.h"
#ifdef ESP32
#include "esp32-hal-spi.h"
#include "soc/spi_struct.h"
#endif

#define ANIMATION_TIME 300

static SPIClass *pSPI;
#ifdef ESP32
static spi_t *spi;	// Need to get it out so we can access it from an IRAM_ATTR routine
hw_timer_t *MicrochipESP327SegDriver::timer = NULL;
#endif

MicrochipESP327SegDriver *MicrochipESP327SegDriver::_handler;

// Digit to segPins indices
DRAM_CONST const uint32_t MicrochipESP327SegDriver::digitShiftMap[6] = {
		0,
		10,
		21,
		32,
		42,
		53
};

// Digit to segPins indices
DRAM_CONST const uint32_t MicrochipESP327SegDriver::segMap[13] = {
	0x77,		// 0
	0x60,		// 1
	0x5D,		// 2
	0x79,		// 3
	0x6A,		// 4
	0x3B,		// 5
	0x3F,		// 6
	0x61,		// 7
	0x7F,		// 8
	0x7B,		// 9
	0x01,		// dp1
	0x08,		// dp2
	0x0, 		// Nothing
};

DRAM_CONST const uint64_t MicrochipESP327SegDriver::colonMap[6] = {
	0,							// none
	1ULL << 19 | 1ULL << 40,	// all
	1ULL << 19 | 1ULL << 40,	// top
	0,							// bottom
	1ULL << 19,					// left (maybe)
	1ULL << 40					// right (maybe)
};

uint32_t MicrochipESP327SegDriver::getMultiplexPins() { return 0; }
uint64_t MicrochipESP327SegDriver::getPins(byte mask) { return colonMap[mask]; }
uint32_t MicrochipESP327SegDriver::getPin(uint32_t digit) { return segMap[digit]; }
uint32_t MicrochipESP327SegDriver::convertPolarity(uint32_t pins) { return pins; }

void MicrochipESP327SegDriver::cacheColonMap() {
}

void MicrochipESP327SegDriver::setNumDigits(int numDigits) {
	this->numDigits = numDigits;
}

void MicrochipESP327SegDriver::setAnimation(Animation animation, int direction) {
	noInterrupts();
	this->animation = animation;
	if (animation != ANIMATION_NONE) {
		animatedDigit = direction > 0 ? 0 : 5;
		this->direction = direction;
		startAnimation = millis();
		animatorPWM.reset(brightness);
	}
	interrupts();
}

bool MicrochipESP327SegDriver::animationDone() {
	noInterrupts();
	bool done = animation == ANIMATION_NONE || 0 > animatedDigit || animatedDigit >= 6;
	interrupts();
	return done;
}

bool MicrochipESP327SegDriver::adjustAnimation(uint32_t nowMs) {
	bool animating = false;

	if (animation != ANIMATION_NONE) {
		if (nowMs - startAnimation > ANIMATION_TIME) {
			animatedDigit += direction;
			startAnimation = nowMs;
		}

		if (0 <= animatedDigit && animatedDigit < 6) {
			animating = true;

			long animationDutyCycle = (long) 100 * (ANIMATION_TIME + startAnimation - nowMs) / ANIMATION_TIME;

			if (animation == ANIMATION_FADE_IN) {
				animationDutyCycle = 100 - animationDutyCycle;
			}

			animationDutyCycle = animationDutyCycle * animationDutyCycle * brightness / 10000;
			if (animation == ANIMATION_FADE_IN && animationDutyCycle <= animatorPWM.quantum) {
				animationDutyCycle = animatorPWM.quantum + 1;
			}

			animatorPWM.onPercent = animationDutyCycle;
		}
	}

	return animating;
}

bool MicrochipESP327SegDriver::esp32CalculateFade(uint32_t nowMs) {
	displayPWM.onPercent = brightness;
	fadeOutPWM.onPercent = 100;
	fadeInPWM.onPercent = 0;

	uint32_t fadeTime = FADE_TIME;
	uint32_t effectTime = FADE_TIME;
	if (displayMode == FADE_OUT_IN) {
		fadeTime = FADE_FAST_TIME;
	}
	if (displayMode == CROSS_FADE_FAST) {
		fadeTime = effectTime = FADE_FAST_TIME;
	}

	if (nowMs - startFade > effectTime) {
		// Fading lasts at most FADE_TIME ms
		return false;
	}

	long fadeOutDutyCycle = (long) 100 * (fadeTime + startFade - nowMs) / fadeTime;
	fadeOutDutyCycle = fadeOutDutyCycle * fadeOutDutyCycle / 100;
	long fadeInDutyCycle = (long) 100 * (nowMs - startFade) / fadeTime;
	fadeInDutyCycle = fadeInDutyCycle * fadeInDutyCycle / 100;

	// Apparently switch statements are flaky in ISRs, so if then else
	if (displayMode == NO_FADE || displayMode == NO_FADE_DELAY) {
		return false;
	} else if (displayMode == FADE_OUT) {
		fadeOutPWM.onPercent = fadeOutDutyCycle;
	} else if (displayMode == FADE_IN) {
		fadeOutPWM.onPercent = 0;
		fadeInPWM.onPercent = fadeInDutyCycle;
	} else if (displayMode == CROSS_FADE || displayMode == CROSS_FADE_FAST) {
		fadeOutPWM.onPercent = fadeOutDutyCycle;
		fadeInPWM.onPercent = fadeInDutyCycle;
	} else if (displayMode == FADE_OUT_IN) {
		fadeInDutyCycle = (long) 100 * (nowMs - startFade - fadeTime) / fadeTime;
		fadeInDutyCycle = fadeInDutyCycle * fadeInDutyCycle / 100;

		if (nowMs - startFade <= fadeTime) {
			fadeOutPWM.onPercent = fadeOutDutyCycle;
			fadeInPWM.onPercent = 0;
		} else {
			fadeInPWM.onPercent = fadeInDutyCycle;
			fadeOutPWM.onPercent = 0;
		}
	}
	return true;
}

void MicrochipESP327SegDriver::init() {
	displayMode = NO_FADE;
	nextDigit = 0;

	// Initialize HV chip
#ifdef ESP8266
	pSPI = &SPI;
#elif ESP32
	pSPI = new SPIClass(HSPI);
	spi = pSPI->bus();
#endif
	/*
	 * SCK = 4
	 * MISO = 12
	 * MOSI = 13
	 * SS = 15
	 */

#ifdef ESP8266
	pSPI->begin();
#elif ESP32
	pSPI->begin(4, 12, 13, 15);
#endif
	pSPI->setFrequency(10000000L);
	pSPI->setBitOrder(bitOrder());
	pSPI->setDataMode(getSPIMode());

#ifdef ESP32
	spi = pSPI->bus();
#endif

	pinMode(LEpin, OUTPUT);
	digitalWrite(LEpin, LOW);

	guard(true);

	if (!_handler) {
		// First time through, set up interrupt handlers
#ifdef ESP8266
		timer0_isr_init();
		timer0_attachInterrupt(isr);
		timer0_write(ESP.getCycleCount() + ESP.getCpuFreqMHz() * 1024);	// Wait 2 seconds before displaying
#elif ESP32
		timer = timerBegin(0, 80, true); // timer_id = 0; divider=80; countUp = true; So at 80MHz, we have a granularity of 1MHz
		timerAttachInterrupt(timer, isr, true); // edge = true
		timerAlarmWrite(timer, 200, true); // Period = 100 micro seconds
		timerAlarmEnable(timer);
#endif
	}
	_handler = this;
	guard(false);
}

// Can't be using any system calls in case some other
// weird system calls are being called, because they can do weird
// things with flash. For the same reason, this function needs to be
// in RAM.
void MicrochipESP327SegDriver::isr() {
#ifdef ESP8266
    uint32_t ccount;
    __asm__ __volatile__("esync; rsr %0,ccount":"=a" (ccount));

	timer0_write(ccount + callCycleCount);
#elif ESP32
//	timerAlarmWrite(timer, 1000, false);	// Once per ms
#endif

	if (_guard == 0) {
		if (_handler) {
			_handler->esp32InterruptHandler();
		}
	}
}

bool MicrochipESP327SegDriver::calculateFade(uint32_t nowMs) {
	return true;
}

void MicrochipESP327SegDriver::interruptHandler() {
}

//#define TEST_DRIVER
void MicrochipESP327SegDriver::esp32InterruptHandler() {
#ifndef TEST_DRIVER
	static uint64_t prevPinMask = 0;
	static byte prevColonMask = 0;

	uint64_t pinMask = 0;
	unsigned long nowMs = millis();
	bool stillFading = esp32CalculateFade(nowMs);
	bool displayOff = displayPWM.off();
	bool fadeOutOff = fadeOutPWM.off();
	bool fadeInOff = fadeInPWM.off();

	if (animation != ANIMATION_NONE) {
		digit = nextDigit;
	}

	uint32_t d = digit;
	uint32_t n = nextDigit;

	// Seconds are in the LSB
	for (int i=0; i<numDigits; i++) {
		byte cd = d & 0xf;
		byte nd = n & 0xf;
		uint32_t cMask = 0;
		uint32_t onMask = 0;
		if (cd != nd) {
			if (stillFading) {
				if (!displayOff) {
					if (displayMode != FADE_OUT_IN) {
						onMask = getPin(cd) & getPin(nd);
					}

					if (!fadeOutOff) {
						cMask = getPin(cd);
					}

					if (!fadeInOff) {
						cMask |= getPin(nd);
					}
				}
			} else {
				if (!displayOff) {
					cMask = getPin(nd);
				}
			}
		} else if (animation != ANIMATION_NONE) {
			adjustAnimation(nowMs);

			if (i == animatedDigit) {
				if (!animatorPWM.off()) {
					cMask = getPin(cd);
				}
			} else if (!displayOff) {
				if (animation == ANIMATION_FADE_OUT) {
					if (i > animatedDigit && direction > 0) {
						cMask = getPin(cd);
					} else if (i < animatedDigit && direction < 0) {
						cMask = getPin(cd);
					}
				} else {
					if (i > animatedDigit && direction < 0) {
						cMask = getPin(cd);
					} else if (i < animatedDigit && direction > 0) {
						cMask = getPin(cd);
					}
				}
			}
		} else if (!displayOff) {
			cMask = getPin(cd);
		}

		cMask |= onMask;

		pinMask |= digitShift(cMask, digitShiftMap[i]);

		d = d >> 4;
		n = n >> 4;
	}

	uint64_t cMask = 0;

	if (colonMask != prevColonMask) {
		if (stillFading) {
			if (!displayOff) {
				if (!fadeOutOff) {
					cMask = getPins(prevColonMask);
				}

				if (!fadeInOff) {
					cMask |= getPins(colonMask);
				}
			}
		} else {
			if (!displayOff) {
				cMask = getPins(colonMask);
			}
		}
	} else if (!displayOff) {
		cMask = getPins(colonMask);
	}

	if (!stillFading) {
		digit = nextDigit;
		prevColonMask = colonMask;
	}

	pinMask |= cMask;

	if (pinMask == prevPinMask) {
		return;
	}

	prevPinMask = pinMask;

#else
	static uint32_t c = 0;

	uint64_t pinMask = 0;
	uint32_t d = nextDigit;
	for (int i=0; i<4; i++) {
		byte cd = d & 0xf;
		pinMask |= ((uint64_t)getPin(cd)) << (7*(3-i));
	}

	if ((c/2) % 2 == 0) {
//		pinMask = 0;
	}

	c++;
#endif

#ifdef ESP8266
    bitOrder.spiWrite64(invert(pinMask));
#elif ESP32
    bitOrder.spiWrite64(spi, invert(pinMask));
#endif
    digitalWrite(LEpin, HIGH);
	digitalWrite(LEpin, LOW);
}
