/*
 * TLC5916IDR_7SegNixieDriver.cpp
 *
 *  Created on: May 6, 2023
 *      Author: Paul Andrews
 */
#include <Arduino.h>
#include <TLC5916IDR_7SegNixieDriver.h>
//#define DTF104B

#ifdef ESP8266
#include <esp8266_peri.h>
#endif

#define ANIMATION_TIME 300

#define SEG_A   0x01
#define SEG_B   0x02
#define SEG_C   0x04
#define SEG_D   0x08
#define SEG_E   0x10
#define SEG_F   0x20
#define SEG_G   0x40
#define SEG_IND 0x80

#define LATCHPin 14    // D5
#define CLOCKPin 13    // D7
#define DATAPin  12    // D6

// Digit to segPins indices
DRAM_CONST const uint32_t TLC5916IDR_7SegNixieDriver::digitShiftMap[6] = {
		0,
		8,
		16,
		24,
		32,
		40
};

// Digit to segPins indices
DRAM_CONST const uint32_t TLC5916IDR_7SegNixieDriver::segMap[13] = {
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,			// 0
	SEG_B | SEG_C,											// 1
	SEG_A | SEG_B | SEG_D | SEG_D | SEG_E | SEG_G,			// 2
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,					// 3
	SEG_B | SEG_C | SEG_F | SEG_G,							// 4
	SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,					// 5
	SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,			// 6
	SEG_A | SEG_B | SEG_C,									// 7
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,	// 8
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,			// 9
	0x01,		// dp1
	0x08,		// dp2
	0x0, 		// Nothing
};

DRAM_CONST const uint64_t TLC5916IDR_7SegNixieDriver::colonMap[6] = {
	0,							// none
	1ULL << 15 | 1ULL << 23 | 1ULL << 31 | 1ULL << 39,	// all
#ifdef DTF104B
	1ULL << 15 | 1ULL << 39,	// top (maybe)
	1ULL << 23 | 1ULL << 31,	// bottom (maybe)
#else
	1ULL << 15 | 1ULL << 31,	// top (maybe)
	1ULL << 23 | 1ULL << 39,	// bottom (maybe)
#endif
	1ULL << 15 | 1ULL << 23,	// left (maybe)
	1ULL << 31 | 1ULL << 39		// right (maybe)
};

uint32_t TLC5916IDR_7SegNixieDriver::getMultiplexPins() { return 0; }
uint64_t TLC5916IDR_7SegNixieDriver::getPins(uint8_t mask) { return colonMap[mask]; }
uint32_t TLC5916IDR_7SegNixieDriver::getPin(uint32_t digit) { return segMap[digit]; }
uint32_t TLC5916IDR_7SegNixieDriver::convertPolarity(uint32_t pins) { return pins; }

void TLC5916IDR_7SegNixieDriver::setNumDigits(int numDigits) {
	this->numDigits = numDigits;
}

void TLC5916IDR_7SegNixieDriver::setAnimation(Animation animation, int direction) {
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

bool TLC5916IDR_7SegNixieDriver::animationDone() {
	noInterrupts();
	bool done = animation == ANIMATION_NONE || 0 > animatedDigit || animatedDigit >= 6;
	interrupts();
	return done;
}


TLC5916IDR_7SegNixieDriver::~TLC5916IDR_7SegNixieDriver() {}

void TLC5916IDR_7SegNixieDriver::init() {
	displayMode = NO_FADE;
	nextDigit = 0;

	pinMode(DATAPin, OUTPUT);
	pinMode(CLOCKPin, OUTPUT);
	pinMode(LEpin, OUTPUT);

	digitalWrite(DATAPin, LOW);
	digitalWrite(CLOCKPin, LOW);
	digitalWrite(LEpin, LOW);

	NixieDriver::init();
}

void TLC5916IDR_7SegNixieDriver::setBrightness(const byte b) {
	brightness = b;
}

void TLC5916IDR_7SegNixieDriver::setWarming(byte val) {
	if (val < TLC5916IDR_7SegNixieDriverMAX_WARMING) {
		warmingPWM.onPercent = val * 5;
	}
}

void TLC5916IDR_7SegNixieDriver::interruptHandler() {
	static uint64_t prevPinMask = 0;
	static byte prevColonMask = 0;
	static unsigned int warmCounter = 0;
	uint64_t pinMask = 0;
	unsigned long nowMs = millis();
	bool stillFading = calculateFade(nowMs);
	bool displayOff = displayPWM.off();
	bool ledDisplayOff = ledDisplayPWM.off();
	bool fadeOutOff = fadeOutPWM.off();
	bool fadeInOff = fadeInPWM.off();

	if (animation != ANIMATION_NONE) {
		digit = nextDigit;
	}

	uint32_t d = digit;
	uint32_t n = nextDigit;
	bool warm = false;
	byte warmMask = 0;
	// Digit pre-heat
	warm = !warmingPWM.off();

	// Seconds are in the LSB
	for (int i=0; i<numDigits; i++) {
		byte cd = d & 0xf;
		byte nd = n & 0xf;
		uint32_t onMask = 0;
		uint32_t cMask = 0;
		warmMask = warm ? 0x7f : 0x00;
		if (cd != nd) {
			if (stillFading) {
				if (!displayOff) {
					if (displayMode != FADE_OUT_IN) {
						onMask = getPin(cd) & getPin(nd);
						warmMask &= ~onMask;
					}

					if (!fadeOutOff) {
						cMask = getPin(cd);
						warmMask &= ~cMask;
					}

					if (!fadeInOff) {
						cMask |= getPin(nd);
						warmMask &= ~cMask;
					}
				}
			} else {
				if (!displayOff) {
					cMask = getPin(nd);
					warmMask &= ~cMask;
				}
			}
		} else if (animation != ANIMATION_NONE) {
			adjustAnimation(nowMs);

			if (i == animatedDigit) {
				if (!animatorPWM.off()) {
					cMask = getPin(cd);
					warmMask &= ~cMask;
				}
			} else if (!displayOff) {
				if (animation == ANIMATION_FADE_OUT) {
					if (i > animatedDigit && direction > 0) {
						cMask = getPin(cd);
						warmMask &= ~cMask;
					} else if (i < animatedDigit && direction < 0) {
						cMask = getPin(cd);
						warmMask &= ~cMask;
					}
				} else {
					if (i > animatedDigit && direction < 0) {
						cMask = getPin(cd);
						warmMask &= ~cMask;
					} else if (i < animatedDigit && direction > 0) {
						cMask = getPin(cd);
						warmMask &= ~cMask;
					}
				}
			}
		} else if (!displayOff) {
			cMask = getPin(cd);
			warmMask &= ~cMask;
		}

		cMask |= onMask | warmMask;

		pinMask |= digitShift(cMask, digitShiftMap[i]);

		d = d >> 4;
		n = n >> 4;
	}

	uint64_t cMask = 0;

	if (colonMask != prevColonMask) {
		if (stillFading) {
			if (!ledDisplayOff) {
				if (!ledFadeOutPWM.off()) {
					cMask = getPins(prevColonMask);
				}

				if (!ledFadeInPWM.off()) {
					cMask |= getPins(colonMask);
				}
			}
		} else {
			if (!ledDisplayOff) {
				cMask = getPins(colonMask);
			}
		}
	} else if (!ledDisplayOff) {
		cMask = getPins(colonMask);
	}

	if (!stillFading) {
		digit = nextDigit;
		prevColonMask = colonMask;
	}

	pinMask |= cMask;

	if (leftLEDState) {
		pinMask |= 1ULL << 7;
	}

	if (rightLEDState) {
		pinMask |= 1ULL << 47;
	}

	if (pinMask == prevPinMask) {
		return;
	}

	prevPinMask = pinMask;

//	    unsigned long nowMic = micros();
//	    while (micros() - nowMic < 100L) {}
    for (int pos=0; pos < 6; pos++) {
		uint8_t mask = (pinMask >> pos*8) & 0xff;
		for(uint8_t i = 0; i < 8; i++) {
//			digitalWrite(DATAPin, !!(mask & (1 << (7 - i))));
#ifdef ESP8266
			if ((mask >> (7-i)) & 0x01) {
				GPOS = (1 << DATAPin);
			} else {
				GPOC = (1 << DATAPin);
			}

			GPOS = (1 << CLOCKPin);
			GPOC = (1 << CLOCKPin);
#else
			digitalWrite(DATAPin, (mask >> (7-i)) & 0x01);
			digitalWrite(CLOCKPin, HIGH);
			digitalWrite(CLOCKPin, LOW);
#endif
		}
    }
#ifdef ESP8266
	GPOS = (1 << LEpin);
	GPOC = (1 << LEpin);
#else
	digitalWrite(LEpin, HIGH);
	digitalWrite(LEpin, LOW);
#endif
}

bool TLC5916IDR_7SegNixieDriver::adjustAnimation(uint32_t nowMs) {
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

NIXIE_DRIVER_ISR_FLAG long TLC5916IDR_7SegNixieDriver_map(long x, long in_min, long in_max, long out_min, long out_max) {
    const long dividend = out_max - out_min;
    const long divisor = in_max - in_min;
    const long delta = x - in_min;

    return (delta * dividend + (divisor / 2)) / divisor + out_min;
}


bool TLC5916IDR_7SegNixieDriver::calculateFade(uint32_t nowMs) {
	static uint8_t minValue = 25;
	displayPWM.onPercent = brightness;
	ledDisplayPWM.onPercent = ((long)brightness)*brightness/100;
	fadeOutPWM.onPercent = 100;
	fadeInPWM.onPercent = minValue;
	ledFadeOutPWM.onPercent = 100;
	ledFadeInPWM.onPercent = 0;

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

	long numitronFadeOutDutyCycle = TLC5916IDR_7SegNixieDriver_map(fadeOutDutyCycle, 0, 100, minValue, 100);
	long numitronFadeInDutyCycle = TLC5916IDR_7SegNixieDriver_map(fadeInDutyCycle, 0, 100, minValue, 100);

	// Apparently switch statements are flaky in ISRs, so if then else
	if (displayMode == NO_FADE || displayMode == NO_FADE_DELAY) {
		return false;
	} else if (displayMode == FADE_OUT) {
		fadeOutPWM.onPercent = numitronFadeOutDutyCycle;
		ledFadeOutPWM.onPercent = fadeOutDutyCycle;
	} else if (displayMode == FADE_IN) {
		fadeOutPWM.onPercent = minValue;
		fadeInPWM.onPercent = numitronFadeInDutyCycle;
		ledFadeOutPWM.onPercent = 0;
		ledFadeInPWM.onPercent = fadeInDutyCycle;
	} else if (displayMode == CROSS_FADE || displayMode == CROSS_FADE_FAST) {
		fadeOutPWM.onPercent = numitronFadeOutDutyCycle;
		fadeInPWM.onPercent = numitronFadeInDutyCycle;
		ledFadeOutPWM.onPercent = fadeOutDutyCycle;
		ledFadeInPWM.onPercent = fadeInDutyCycle;
	} else if (displayMode == FADE_OUT_IN) {
		numitronFadeInDutyCycle = (long) 100 * (nowMs - startFade - fadeTime) / fadeTime;
		numitronFadeInDutyCycle = numitronFadeInDutyCycle * numitronFadeInDutyCycle / 100;

		if (nowMs - startFade <= fadeTime) {
			fadeOutPWM.onPercent = numitronFadeOutDutyCycle;
			fadeInPWM.onPercent = minValue;
			ledFadeOutPWM.onPercent = fadeOutDutyCycle;
			ledFadeInPWM.onPercent = 0;
		} else {
			fadeInPWM.onPercent = numitronFadeInDutyCycle;
			fadeOutPWM.onPercent = minValue;
			ledFadeInPWM.onPercent = fadeInDutyCycle;
			ledFadeOutPWM.onPercent = minValue;
		}
	}
	return true;
}
