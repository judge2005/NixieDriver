/*
 * HVNixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <HVNixieDriver.h>
#include <SPI.h>

static SPIClass *pSPI;
#ifdef ESP32
static spi_t *spi;	// Need to get it out so we can access it from an IRAM_ATTR routine
#endif

void HVNixieDriver::init() {
	displayMode = NO_FADE;
	nextDigit = 0;

	// Initialize HV chip
#ifdef ESP8266
	pSPI = &SPI;
#elif ESP32
	pSPI = new SPIClass(HSPI);
	spi = pSPI->bus();
#endif

	pSPI->begin();
	pSPI->setFrequency(10000000L);
	pSPI->setBitOrder(MSBFIRST);
	pSPI->setDataMode(getSPIMode());

#ifdef ESP32
	spi = pSPI->bus();
#endif

	pinMode(LEpin, OUTPUT);
	digitalWrite(LEpin, LOW);

//	interruptHandler();

	NixieDriver::init();
}

uint32_t HVNixieDriver::getMultiplexPins() {
	return 0;
}

void HVNixieDriver::interruptHandler() {
	DRAM_CONST static const uint32_t digitMasks[] = {
		0xfff0,
		0xff0f,
		0xf0ff,
		0x0fff
	};
	static uint32_t prevPinMask = 0;
	uint32_t pinMask = 0;

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

#ifdef ESP8266
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
#elif ESP32
    spiWriteLongNL(spi, pinMask);	// This is already flagged with IRAM_ATTR
#endif

	digitalWrite(LEpin, HIGH);
	digitalWrite(LEpin, LOW);
}

bool NIXIE_DRIVER_ISR_FLAG HVNixieDriver::calculateFade(uint32_t nowMs) {
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
