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

static SPIClass *pSPI;
#ifdef ESP32
static spi_t *spi;	// Need to get it out so we can access it from an IRAM_ATTR routine
hw_timer_t *HV5523ESP32NixieDriver::timer = NULL;
#endif

HV5523ESP32NixieDriver *HV5523ESP32NixieDriver::_handler;

DRAM_CONST const uint32_t HV5523ESP32NixieDriver::nixieDigitMap[13] = {
	1,	// 0
	2,		// 1
	4,		// 2
	8,		// 3
	0x10,		// 4
	0x20,		// 5
	0x40,		// 6
	0x80,	// 7
	0x100,	// 8
	0x200,	// 9
	0x400,	// dp1
	0x800,	// dp2
	0x0, 		// Nothing
};

/*
 * 012 = 1 + 800  + 400000 = 400801
 * 345 = 8 + 4000 + 2000000 = 2004008
 */
DRAM_CONST const uint32_t HV5523ESP32NixieDriver::colonMap[4] = {
	0,		// none
	0xf,	// all
	0x5,	// top
	0xa		// bottom
};

uint32_t HV5523ESP32NixieDriver::currentColonMap[4] = {
	0,	// none
	0,	// none
	0,	// none
	0	// none
};

uint32_t NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::getMultiplexPins() { return 0; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::getPins(byte mask) { return colonMap[mask]; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::getPin(uint32_t digit) { return transition == 1 ? 0 : nixieDigitMap[digit]; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::convertPolarity(uint32_t pins) { return pins ^ 0xffffffff; }

void NIXIE_DRIVER_ISR_FLAG myPause(uint64_t delay) {
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

void NIXIE_DRIVER_ISR_FLAG spiWrite64NL(spi_t * spi, const uint64_t data) {
	uint8_t *d = (uint8_t *)&(data);
	spi->dev->data_buf[1] = d[3] | (d[2] << 8) | (d[1] << 16) | (d[0] << 24);
	spi->dev->data_buf[0] = d[7] | (d[6] << 8) | (d[5] << 16) | (d[4] << 24);

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

bool NIXIE_DRIVER_ISR_FLAG HV5523ESP32NixieDriver::esp32CalculateFade(uint32_t nowMs) {
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

	// Apparently switch statements are flaky in ISRs too...
#ifdef SWITCH_IN_ISR_WORKS
	switch (displayMode) {
	case NO_FADE:
	case NO_FADE_DELAY:
		return false;
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
#endif
	if (displayMode == NO_FADE || displayMode == NO_FADE_DELAY) {
		return false;
	} else if (displayMode == FADE_OUT) {
		fadeOutPWM.onPercent = fadeOutDutyCycle;
	} else if (displayMode == FADE_IN) {
		fadeOutPWM.onPercent = 0;
		fadeInPWM.onPercent = fadeInDutyCycle;
	} else if (displayMode == CROSS_FADE) {
		fadeOutPWM.onPercent = fadeOutDutyCycle;
		fadeInPWM.onPercent = fadeInDutyCycle;
	} else if (displayMode == FADE_OUT_IN) {
		fadeInDutyCycle = (long) 100 * (nowMs - startFade - FADE_TIME) / FADE_TIME;
		fadeInDutyCycle = fadeInDutyCycle * fadeInDutyCycle * brightness / 10000;

		if (nowMs - startFade <= FADE_TIME) {
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

	uint64_t pinMask = 0;
	bool stillFading = esp32CalculateFade(millis());
	displayPWM.onPercent = brightness;
	bool displayOff = displayPWM.off();

	uint32_t d = digit;
	uint32_t n = nextDigit;

	// Seconds are in the LSB
	for (int i=0; i<6; i++) {
		byte cd = d & 0xf;
		byte nd = n & 0xf;
		uint32_t cMask = 0;
		if (cd != nd) {
			// If we have a full clock, we only want to fade if an individual digit has changed,
			// otherwise we want to fade whenever fade has been set.
			if (stillFading) {
				if (!fadeOutPWM.off()) {
					cMask = getPin(cd);
				}

				if (!fadeInPWM.off()) {
					cMask |= getPin(nd);
				}
			} else {
				if (!displayOff) {
					cMask = getPin(nd);
				}
			}
		} else if (!displayOff) {
			cMask = getPin(cd);
		}

		pinMask |= ((uint64_t)cMask) << (10*i);

		d = d >> 4;
		n = n >> 4;
	}

	if (!stillFading) {
		digit = nextDigit;
	}

	if (!displayOff) {
		pinMask |= ((uint64_t)getPins(colonMask)) << 60;
	}

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
	uint64_t cMask = pinMask ^ 0xffffffffffffffffULL;
	spiWrite64NL(spi, cMask);
//	spiWriteLongNL(spi, convertPolarity((pinMask >> 32) & 0xffffffff));	// This is already flagged with IRAM_ATTR
//	spiWriteLongNL(spi, convertPolarity(pinMask & 0xffffffff));	// This is already flagged with IRAM_ATTR
#endif
//	myPause(2);
	digitalWrite(LEpin, HIGH);
	digitalWrite(LEpin, LOW);
//	digitalWrite(LEpin, HIGH);
}
