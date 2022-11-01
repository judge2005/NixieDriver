/*
 * HV5523NixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HV9808ESP32NixieDriverMultiplex.h>
#include <SPI.h>
#ifdef ESP32
#include "esp32-hal-spi.h"
#include "soc/spi_struct.h"
#else
#include <limits.h>
#endif

#define ANIMATION_TIME 300

static SPIClass *pSPI;
#ifdef ESP32
static spi_t *spi;	// Need to get it out so we can access it from an IRAM_ATTR routine
hw_timer_t *HV9808ESP32NixieDriverMultiplex::timer = NULL;
#endif

HV9808ESP32NixieDriverMultiplex *HV9808ESP32NixieDriverMultiplex::_handler;

DRAM_CONST const uint32_t HV9808ESP32NixieDriverMultiplex::nixieDigitMap[13] = {
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
DRAM_CONST const uint32_t HV9808ESP32NixieDriverMultiplex::colonMap[4] = {
	0,		// none
	0x400,	// all
	0,	// top
	0x400		// bottom
};

uint32_t HV9808ESP32NixieDriverMultiplex::currentColonMap[4] = {
	0,	// none
	0,	// none
	0,	// none
	0	// none
};

uint32_t NIXIE_DRIVER_ISR_FLAG HV9808ESP32NixieDriverMultiplex::getMultiplexPins() { return 0; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV9808ESP32NixieDriverMultiplex::getPins(byte mask) { return colonMap[mask]; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV9808ESP32NixieDriverMultiplex::getPin(uint32_t digit) { return transition == 1 ? 0 : nixieDigitMap[digit]; }
uint32_t NIXIE_DRIVER_ISR_FLAG HV9808ESP32NixieDriverMultiplex::convertPolarity(uint32_t pins) { return pins; }

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
#endif

void HV9808ESP32NixieDriverMultiplex::cacheColonMap() {
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

void HV9808ESP32NixieDriverMultiplex::setAnimation(Animation animation, int direction) {
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

bool HV9808ESP32NixieDriverMultiplex::animationDone() {
	noInterrupts();
	bool done = animation == ANIMATION_NONE || 0 > animatedDigit || animatedDigit >= 6;
	interrupts();
	return done;
}

bool NIXIE_DRIVER_ISR_FLAG HV9808ESP32NixieDriverMultiplex::adjustAnimation(uint32_t nowMs) {
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

bool NIXIE_DRIVER_ISR_FLAG HV9808ESP32NixieDriverMultiplex::esp32CalculateFade(uint32_t nowMs) {
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

void HV9808ESP32NixieDriverMultiplex::init() {
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
		timerAlarmWrite(timer, 130, true); // Period = 520 micro seconds, frequency = 1,923Hz
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
void NIXIE_DRIVER_ISR_FLAG HV9808ESP32NixieDriverMultiplex::isr() {
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

bool HV9808ESP32NixieDriverMultiplex::calculateFade(uint32_t nowMs) {
	return true;
}

void HV9808ESP32NixieDriverMultiplex::interruptHandler() {
}

static inline uint64_t rotl64 (uint64_t n, unsigned int c)
{
  const uint64_t mask = (CHAR_BIT*sizeof(n) - 1);  // assumes width is a power of 2.

  // assert ( (c<=mask) &&"rotate by type width or more");
  c &= mask;
  return (n<<c) | (n>>( (-c)&mask ));
}

#define ALL_BLANKS (0xccccccccccccccccULL)
#define TIME_MASK  (0x00ffffff)
#define DATE_MASK  (0x00ffffff00000000ULL)

//#define TEST_DRIVER
#define D_DELAY
void NIXIE_DRIVER_ISR_FLAG HV9808ESP32NixieDriverMultiplex::esp32InterruptHandler() {
	static uint32_t prevPinMask = 0;
	static uint32_t tubeMask = 1;
	static uint32_t digitMask = 1;
	static uint8_t digit = 0xc;
	static uint8_t colon = 0;
	static uint8_t tubeNo = 0;
	static uint32_t counter = 0;
	static uint64_t display = ALL_BLANKS;
	static uint64_t colonMask = 0;

#ifndef TEST_DRIVER

	uint32_t dMask = digitMask;
	uint32_t tMask = tubeMask;

#ifdef D_DELAY
	if (counter == 3 ) {
		dMask = 0;
	}
#endif

	if (counter == 0) {
		tubeMask = tubeMask << 1;
		tubeNo++;
		display = display >> 4;
		colonMask = colonMask >> 4;

		if (tubeMask == 0x10000) {
			tubeMask = 1;
			tubeNo = 0;
			display = nextDigit;
			display |= (date * 1ULL) << 32;
			display = rotl64(display, 4);
			colonMask = rotl64(colons, 4);
		}

		digit = display & 0xf;
		colon = colonMask & 0xf;
		digitMask = getPin(digit);
		digitMask |= getPins(colon);

		dMask = digitMask;
		tMask = tubeMask;
	}

	uint32_t pinMask = (((tMask << 16) | dMask) ^ 0xffffffff );

	counter = (counter + 1) % 4;

	if (pinMask == prevPinMask) {
		return;
	}

	prevPinMask = pinMask;

#else
	static uint8_t blanks = 0;
	static long lastMillis = 0;

	uint32_t dMask = digitMask;
	uint32_t tMask = tubeMask;

	if (millis() - lastMillis > 1000) {
		lastMillis = millis();
		blanks = (blanks + 1) % 16;
	}

//#define D_DELAY
#ifdef D_DELAY
	if (counter == 3 ) {
		dMask = 0;
	}
#endif

//#define T_DELAY
#ifdef T_DELAY
	if (counter < 3) {
		tMask = 0;
	}
#endif

	uint32_t pinMask = (((tMask << 16) | dMask) ^ 0xffffffff );

	counter = (counter + 1) % 4;
	if (counter == 0) {
		tubeMask = tubeMask << 1;
		digit = (digit + 1) % 10;
		if (tubeMask == 0x10000) {
			tubeMask = 1;
		}

		if (tubeMask << 1 == 1 << blanks) {
			digit = 0;
		}

		if (tubeMask << 1 < (1 << blanks)) {
			digitMask = 0;
		} else {
			digitMask = nixieDigitMap[digit];
		}
	}

	if (pinMask == prevPinMask) {
		return;
	}

	prevPinMask = pinMask;
#endif

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
	spiWriteLongNL(spi, pinMask);	// This is already flagged with IRAM_ATTR
#endif
	digitalWrite(LEpin, HIGH);
	digitalWrite(LEpin, LOW);
//	digitalWrite(LEpin, HIGH);
}
