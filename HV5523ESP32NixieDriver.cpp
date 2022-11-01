/*
 * HV5523NixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HV5523ESP32NixieDriver.h>
#include <SPI.h>
#ifdef ESP32
#include "esp32-hal-spi.h"
#include "soc/spi_struct.h"
#endif

#define ANIMATION_TIME 300

static SPIClass *pSPI;
#ifdef ESP32
static spi_t *spi;	// Need to get it out so we can access it from an IRAM_ATTR routine
hw_timer_t *HV5523ESP32NixieDriver::timer = NULL;
#endif

HV5523ESP32NixieDriver *HV5523ESP32NixieDriver::_handler;

DRAM_CONST const uint32_t HV5523ESP32NixieDriver::nixieDigitMap[16] = {
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
DRAM_CONST const uint32_t HV5523ESP32NixieDriver::colonMap[6] = {
	0,		// none
	0xf,	// all
	0x5,	// top
	0xa,	// bottom
	0x3,	// left (maybe)
	0xc		// right (maybe)
};

uint32_t HV5523ESP32NixieDriver::currentColonMap[6] = {
	0,	// none
	0,	// none
	0,	// none
	0,	// none
	0,	// none
	0	// none
};

uint32_t NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::getMultiplexPins() { return 0; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::getPins(byte mask) { return colonMap[mask]; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::getPin(uint32_t digit) { return transition == 1 ? 0 : nixieDigitMap[digit]; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::convertPolarity(uint32_t pins) { return pins ^ 0xffffffff; }

static void NIXIE_DRIVER_ISR_FLAG myPause(uint64_t delay) {
	uint64_t enter = micros();
	while (micros() - enter < delay);
}

#ifdef ESP32
struct spi_struct_t {
    spi_dev_t * dev;
#if !CONFIG_DISABLE_HAL_LOCKS
    xSemaphoreHandle lock;
#endif
    uint8_t num;
};

static void NIXIE_DRIVER_ISR_FLAG spiWrite64NL(spi_t * spi, const uint64_t data) {
	uint8_t *d = (uint8_t *)&(data);
	spi->dev->data_buf[1] = d[3] | (d[2] << 8) | (d[1] << 16) | (d[0] << 24);
	spi->dev->data_buf[0] = d[7] | (d[6] << 8) | (d[5] << 16) | (d[4] << 24);

	spi->dev->mosi_dlen.usr_mosi_dbitlen = 63;
	spi->dev->miso_dlen.usr_miso_dbitlen = 0;
	spi->dev->cmd.usr = 1;
	while(spi->dev->cmd.usr);
}

static void NIXIE_DRIVER_ISR_FLAG spiWrite64RL(spi_t * spi, const uint64_t data) {
	uint32_t *d = (uint32_t *)&(data);
	spi->dev->data_buf[1] = d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24);
	spi->dev->data_buf[0] = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);

	spi->dev->mosi_dlen.usr_mosi_dbitlen = 63;
	spi->dev->miso_dlen.usr_miso_dbitlen = 0;
	spi->dev->cmd.usr = 1;
	while(spi->dev->cmd.usr);
}
#endif

void HV5523ESP32NixieDriver::cacheColonMap() {
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
		currentColonMap[1] = nixieDigitMap[mapDigit(10)];
		currentColonMap[2] = nixieDigitMap[mapDigit(11)];
		currentColonMap[3] = nixieDigitMap[mapDigit(10)] | nixieDigitMap[mapDigit(11)];
	}
}

void HV5523ESP32NixieDriver::setAnimation(Animation animation, int direction) {
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

bool HV5523ESP32NixieDriver::animationDone() {
	noInterrupts();
	bool done = animation == ANIMATION_NONE || 0 > animatedDigit || animatedDigit >= 6;
	interrupts();
	return done;
}

bool NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::adjustAnimation(uint32_t nowMs) {
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

bool NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::esp32CalculateFade(uint32_t nowMs) {
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

void HV5523ESP32NixieDriver::init() {
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
	pSPI->setBitOrder(MSBFIRST);
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
void NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::isr() {
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

bool HV5523ESP32NixieDriver::calculateFade(uint32_t nowMs) {
	return true;
}

void HV5523ESP32NixieDriver::interruptHandler() {
}

//#define TEST_DRIVER
void NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::esp32InterruptHandler() {
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

		if (small) {
			pinMask |= ((uint64_t)cMask) << (10*(5-i));
		} else {
			pinMask |= ((uint64_t)cMask) << (10*i);
		}

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

    //	myPause(2);
#ifdef ESP8266
    union {
            uint32_t l;
            uint8_t b[4];
    } data_;

    while(SPI1CMD & SPIBUSY) {}

    // Set Bits to transfer
    const uint32_t mask = ~((SPIMMOSI << SPILMOSI) | (SPIMMISO << SPILMISO));
    // 31 is (8*4-1)
    SPI1U1 = ((SPI1U1 & mask) | ((31 << SPILMOSI) | (31 << SPILMISO)));

    data_.l = (pinMask >> 32) & 0xffffffff;
    SPI1W0 = (data_.b[3] | (data_.b[2] << 8) | (data_.b[1] << 16) | (data_.b[0] << 24));

    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}

    // 31 is (8*4-1)
    SPI1U1 = ((SPI1U1 & mask) | ((31 << SPILMOSI) | (31 << SPILMISO)));

    data_.l = pinMask & 0xffffffff;
    SPI1W0 = (data_.b[3] | (data_.b[2] << 8) | (data_.b[1] << 16) | (data_.b[0] << 24));

    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}
#elif ESP32
	spiWrite64NL(spi, pinMask ^ 0xffffffffffffffffULL);
//	spiWriteLongNL(spi, convertPolarity((pinMask >> 32) & 0xffffffff));	// This is already flagged with IRAM_ATTR
//	spiWriteLongNL(spi, convertPolarity(pinMask & 0xffffffff));	// This is already flagged with IRAM_ATTR
#endif
//	myPause(2);
	digitalWrite(LEpin, HIGH);
	digitalWrite(LEpin, LOW);
//	digitalWrite(LEpin, HIGH);
}
