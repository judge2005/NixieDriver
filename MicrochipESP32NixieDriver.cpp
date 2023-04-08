/*
 * MicrochipESP32NixieDriver.cpp
 *
 *  Created on: Feb 10, 2023
 *      Author: Paul Andrews
 */

#include <MicrochipESP32NixieDriver.h>
#include <SPI.h>
#ifdef ESP32
#include "esp32-hal-spi.h"
#include "soc/spi_struct.h"
#endif

#define ANIMATION_TIME 300

static SPIClass *pSPI;
#ifdef ESP32
static spi_t *spi;	// Need to get it out so we can access it from an IRAM_ATTR routine


hw_timer_t *MicrochipESP32NixieDriver::timer = NULL;
#endif


MicrochipESP32NixieDriver *MicrochipESP32NixieDriver::_handler;

DRAM_CONST const uint32_t MicrochipESP32NixieDriver::defaultDigitMap[16] = {
	1,		// 0
	2,		// 1
	4,		// 2
	8,		// 3
	0x10,	// 4
	0x20,	// 5
	0x40,	// 6
	0x80,	// 7
	0x100,	// 8
	0x200,	// 9
	0x101,	// 10 - For CD11 clock
	0x102,	// 11 - For CD11 clock
	0x0, 	// 12 - Nothing
	0x201,	// 13 - For CD11 clock
	0x202,	// 14 - For CD11 clock
	0x0		// 15 - Nothing
};

DRAM_CONST const uint32_t MicrochipESP32NixieDriver::cd27DigitMap[16] = {
	1,		// 0
	0x200,	// 1
	0x100,	// 2
	0x80,	// 3
	0x40,	// 4
	0x20,	// 5
	0x10,	// 6
	8,		// 7
	4,		// 8
	2,		// 9
	0x101,	// 10 - For CD11 clock
	0x102,	// 11 - For CD11 clock
	0x0, 	// 12 - Nothing
	0x201,	// 13 - For CD11 clock
	0x202,	// 14 - For CD11 clock
	0x0		// 15 - Nothing
};

uint32_t MicrochipESP32NixieDriver::currentDigitMap[16] = {
	1,		// 0
	2,		// 1
	4,		// 2
	8,		// 3
	0x10,	// 4
	0x20,	// 5
	0x40,	// 6
	0x80,	// 7
	0x100,	// 8
	0x200,	// 9
	0x101,	// 10 - For CD11 clock
	0x102,	// 11 - For CD11 clock
	0x0, 	// 12 - Nothing
	0x201,	// 13 - For CD11 clock
	0x202,	// 14 - For CD11 clock
	0x0		// 15 - Nothing
};

/*
 * 012 = 1 + 800  + 400000 = 400801
 * 345 = 8 + 4000 + 2000000 = 2004008
 */

DRAM_CONST const uint32_t MicrochipESP32NixieDriver::colonMap[6] = {
	0,		// none
	0xf,	// all
	0x5,	// top
	0xa,	// bottom
	0x3,	// left (maybe)
	0xc		// right (maybe)
};

uint32_t MicrochipESP32NixieDriver::currentColonMap[6] = {
	0,	// none
	0,	// none
	0,	// none
	0,	// none
	0,	// none
	0	// none
};

uint32_t NIXIE_DRIVER_ISR_FLAG MicrochipESP32NixieDriver::getMultiplexPins() { return 0; }
uint32_t NIXIE_DRIVER_ISR_FLAG MicrochipESP32NixieDriver::getPins(byte mask) { return colonMap[mask]; }
uint32_t NIXIE_DRIVER_ISR_FLAG MicrochipESP32NixieDriver::getPin(uint32_t digit) { return transition == 1 ? 0 : currentDigitMap[digit]; }
uint32_t NIXIE_DRIVER_ISR_FLAG MicrochipESP32NixieDriver::convertPolarity(uint32_t pins) { return pins ^ 0xffffffff; }


void MicrochipESP32NixieDriver::cacheColonMap() {
	if (indicator == 0) {
		for (int i=0; i<4; i++) {
			currentColonMap[i] = 0;
		}
	} else if (indicator == 1) {
		for (int i=0; i<4; i++) {
			currentColonMap[i] = colonMap[i];
		}
	} else {
		currentColonMap[0] = 0;
		currentColonMap[1] = currentDigitMap[mapDigit(10)];
		currentColonMap[2] = currentDigitMap[mapDigit(11)];
		currentColonMap[3] = currentDigitMap[mapDigit(10)] | currentDigitMap[mapDigit(11)];
	}
}

void MicrochipESP32NixieDriver::setAnimation(Animation animation, int direction) {
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

bool MicrochipESP32NixieDriver::animationDone() {
	noInterrupts();
	bool done = animation == ANIMATION_NONE || 0 > animatedDigit || animatedDigit >= 6;
	interrupts();
	return done;
}

bool NIXIE_DRIVER_ISR_FLAG MicrochipESP32NixieDriver::adjustAnimation(uint32_t nowMs) {
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


bool NIXIE_DRIVER_ISR_FLAG MicrochipESP32NixieDriver::esp32CalculateFade(uint32_t nowMs) {
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


void MicrochipESP32NixieDriver::init() {
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
void NIXIE_DRIVER_ISR_FLAG MicrochipESP32NixieDriver::isr() {
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

bool MicrochipESP32NixieDriver::calculateFade(uint32_t nowMs) {
	return true;
}

void MicrochipESP32NixieDriver::interruptHandler() {
}

//#define TEST_DRIVER
void NIXIE_DRIVER_ISR_FLAG MicrochipESP32NixieDriver::esp32InterruptHandler() {
#ifndef TEST_DRIVER
	static uint64_t prevPinMask = 0;
	static byte prevColonMask;

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

	// Seconds are in the MSB
	for (int i=0; i<6; i++) {
		byte cd = d & 0xf;
		byte nd = n & 0xf;
		uint32_t cMask = 0;
		if (cd != nd) {
			if (stillFading) {
				if (!displayOff) {
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

		pinMask |= digitShift(cMask, i);

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

	pinMask |= cMask << 60;

	if (pinMask == prevPinMask) {
		return;
	}

	prevPinMask = pinMask;

#else
	static uint32_t c = 0;

	uint64_t pinMask = 0;

	for (int i=0; i<6; i++) {
		pinMask |= ((uint64_t)getPin(6)) << (10*i);
	}

	if ((c/2) % 2 == 0) {
		pinMask = 0;
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
