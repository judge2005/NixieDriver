/*
 * SN74HC595_7SegNixieDriver.cpp
 *
 *  Created on: Apr 24, 2020
 *      Author: Paul Andrews
 */

#include <SN74HC595_7SegNixieDriver.h>
#define USE_SPI
#define SYNC_DELAY_MS 0

DRAM_CONST const uint32_t kMap[] = {
		0,
		0x1,
		0x2,
		0x4,
		0x8,
		0x10,
		0x20,
		0x40
};

// Circular wipe
DRAM_CONST const uint32_t SN74HC595_7SegNixieDriver::circularWipeMap[8] = {
	kMap[6],		// K8 - HV4
	kMap[1],		// K9 - HV15
	kMap[2],	// K10 - HV12
	kMap[3],		// K11 - HV6
	kMap[4],	// K12 - HV9
	kMap[5],		// K13 - HV3
	kMap[7],		// K14 - HV13
	0,		// K7 - HV2
};

DRAM_CONST const uint32_t SN74HC595_7SegNixieDriver::circularWipeEraseMap[8] = {
	0xffffffff ^ 0x0,		// Erase nothing
	0xffffffff ^ (kMap[6]),
	0xffffffff ^ (kMap[6] | kMap[1]),
	0xffffffff ^ (kMap[6] | kMap[1] | kMap[2]),
	0xffffffff ^ (kMap[6] | kMap[1] | kMap[2] | kMap[3]),
	0xffffffff ^ (kMap[6] | kMap[1] | kMap[2] | kMap[3] | kMap[4]),
	0xffffffff ^ (kMap[6] | kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[5]),
	0xffffffff ^ (kMap[6] | kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[5] | kMap[7]),
};

DRAM_CONST const uint32_t SN74HC595_7SegNixieDriver::circularWipeRevealMap[8] = {
	(kMap[6]),
	(kMap[6] | kMap[1]),
	(kMap[6] | kMap[1] | kMap[2]),
	(kMap[6] | kMap[1] | kMap[2] | kMap[3]),
	(kMap[6] | kMap[1] | kMap[2] | kMap[3] | kMap[4]),
	(kMap[6] | kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[5]),
	(kMap[6] | kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[5] | kMap[7]),
	0xffffffff,	// Erase nothing
};

DRAM_CONST const uint32_t SN74HC595_7SegNixieDriver::nixieDigitMap[13] = {
	kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[5] | kMap[6],
	kMap[2] | kMap[3],
	kMap[1] | kMap[2] | kMap[4] | kMap[5] | kMap[7],
	kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[7],
	kMap[2] | kMap[3] | kMap[6] | kMap[7],
	kMap[1] | kMap[3] | kMap[4] | kMap[6] | kMap[7],
	kMap[3] | kMap[4] | kMap[5] | kMap[6] | kMap[7],
	kMap[1] | kMap[2] | kMap[3],
	kMap[1] | kMap[2] | kMap[3] | kMap[4] | kMap[5] | kMap[6] | kMap[7],
	kMap[1] | kMap[2] | kMap[3] | kMap[6] | kMap[7],
	kMap[1] | kMap[4] | kMap[7],	// horizontal bars
	kMap[2] | kMap[3] | kMap[5] | kMap[6], // vertical bars
	0x0 		// Nothing
};

static SPIClass *pSPI;
#ifdef ESP32
static spi_t *spi;	// Need to get it out so we can access it from an IRAM_ATTR routine
#endif

void NIXIE_DRIVER_ISR_FLAG SN74HC595_7SegNixieDriver::syncHandler() {
	if (_guard == 0) {
		if (_handler) {
			((SN74HC595_7SegNixieDriver*)_handler)->setSync(1);
		}
	}
}
void SN74HC595_7SegNixieDriver::setSync(int sync) {
	syncHigh = sync;
	syncMs = millis();
}

uint64_t SN74HC595_7SegNixieDriver::getPins(byte mask) { return 0; }

uint32_t SN74HC595_7SegNixieDriver::getPin(uint32_t digit) {
	switch (transition) {
	default:
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

void SN74HC595_7SegNixieDriver::init() {
	displayMode = NO_FADE;
	nextDigit = 0;
	fadeInPWM = SoftPWM(0, 50);
	fadeOutPWM = SoftPWM(100, 50);
	displayPWM = SoftPWM(100, 50);

#ifdef USE_SPI
	// Initialize HV chip
#ifdef ESP8266
	pSPI = &SPI;
#elif ESP32
	pSPI = new SPIClass(HSPI);
	spi = pSPI->bus();
#endif

	pSPI->begin();
	pSPI->setFrequency(1000000L);
	pSPI->setBitOrder(MSBFIRST);
	pSPI->setDataMode(getSPIMode());

#ifdef ESP32
	spi = pSPI->bus();
#endif
#else
	pinMode(13, OUTPUT);
	pinMode(14, OUTPUT);
#endif

	pinMode(LEpin, OUTPUT);

	NixieDriver::init();
}

uint32_t SN74HC595_7SegNixieDriver::convertPolarity(uint32_t pins) { return pins; }

void SN74HC595_7SegNixieDriver::interruptHandler() {
	static uint32_t prevPinMask = 0;

	unsigned long nowMs = millis();

	if (!syncHigh) {
		return;
	}

	if (nowMs - syncMs < SYNC_DELAY_MS) {
		return;
	}

	syncHigh = 0;

	uint32_t pinMask = 0;

	bool stillFading = calculateFade(nowMs);
	displayPWM.onPercent = brightness;
	bool displayOff = displayPWM.off();
	bool fadeOutOff = fadeOutPWM.off();
	bool fadeInOff = fadeInPWM.off();

//	static int cycle = 0;
//
//	cycle = (cycle + 1) % 4;
//
//	if (brightness / 50 < cycle) {
//		displayOff = true;
//	} else {
//		displayOff = false;
//	}

	uint32_t d = digit;
	uint32_t n = nextDigit;

	// Minutes are in the MSB
	for (int i=0; i<4; i++) {
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
		} else if (!displayOff) {
			cMask = getPin(cd);
		}

		cMask |= onMask;

		pinMask |= ((uint64_t)cMask) << (8*(3-i));

		d = d >> 4;
		n = n >> 4;
	}

	if (!stillFading) {
		digit = nextDigit;
	}

	pinMask = convertPolarity(pinMask);

	if (pinMask == prevPinMask) {
		return;
	}

	prevPinMask = pinMask;

	digitalWrite(LEpin, LOW);
//	    unsigned long nowMic = micros();
//	    while (micros() - nowMic < 100L) {}
#ifdef ESP8266
#ifdef USE_SPI
	while(SPI1CMD & SPIBUSY) {}
    // Set Bits to transfer
    const uint32_t mask = ~((SPIMMOSI << SPILMOSI) | (SPIMMISO << SPILMISO));
    SPI1U1 = ((SPI1U1 & mask) | ((31 << SPILMOSI) | (31 << SPILMISO)));

    SPI1W0 = pinMask;
    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}
#else
    for (int pos=0; pos < 4; pos++) {
		for(uint8_t i = 0; i < 8; i++) {
			digitalWrite(13, !!(((uint8_t)pinMask) & (1 << (7 - i))));

			digitalWrite(14, HIGH);
			digitalWrite(14, LOW);
		}
    }
#endif
#elif ESP32
    spiWriteLongNL(spi, pinMask);	// This is already flagged with IRAM_ATTR
#endif

	digitalWrite(LEpin, HIGH);
}

bool NIXIE_DRIVER_ISR_FLAG SN74HC595_7SegNixieDriver::calculateFade(uint32_t nowMs) {
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
