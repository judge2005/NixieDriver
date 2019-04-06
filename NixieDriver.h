/*
 * NixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_NIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_NIXIEDRIVER_H_

#include <Arduino.h>

struct SoftPWM {
	byte quantum = 1;
	byte onPercent = 100;
	int count = 0;

	SoftPWM(byte onPercent) {
		reset(onPercent);
	}

	SoftPWM(byte onPercent, byte quantum) {
		this->quantum = quantum;
		reset(onPercent);
	}

	bool ICACHE_RAM_ATTR off();

	void reset(byte onPercent) {
		this->onPercent = onPercent;
		count = 0;
	}
};

class NixieDriver {
public:
	enum DisplayMode {
		NO_FADE_DELAY,
		FADE_OUT,
		FADE_IN,
		FADE_OUT_IN,
		CROSS_FADE,
		NO_FADE
	};

	virtual void init();
	virtual ~NixieDriver() {}
	static void guard(const bool flag);
	static void setPWMFreq(int pwm_freq) {
		callCycleCount = ESP.getCpuFreqMHz() * 1000000L / pwm_freq;
	}
	static int getPWMFreq(int pwm_freq) {
		return ESP.getCpuFreqMHz() * 1000000L / callCycleCount;
	}
	inline void setMode(const DisplayMode mode) {
		displayMode = mode;
	}
	inline void setBrightness(const byte b) {
		brightness = b;
	}
	inline void setNixieDigit(const uint32_t digit) {
		nextDigit = digit;
	}
	inline uint32_t getNixieDigit() {
		return nextDigit;
	}
	inline void setNewNixieDigit(const uint32_t digit) {
		startFade = millis();
		nextDigit = digit;
	}
	inline void setColons(const byte mask) {
		colonMask = mask;
	}
	inline void setDisplayOn(const bool displayOn) {
		this->displayOn = displayOn;
	}
	inline byte getIndicator() {
		return indicator;
	}
	inline void setIndicator(const byte indicator) {
		this->indicator = indicator;
		cacheColonMap();
	}
	virtual bool setTransition(const byte transition, uint32_t nextClockDigit) {
		bool ret = true;
		if (this->transition != transition) {
			this->transition = transition;
			transitionCount = 0;
			setNewNixieDigit((digit + 1) % 10);
		} else if (transition != 0) {
			transitionCount++;
			if (transitionCount >= 19) {
				setNewNixieDigit(nextClockDigit);
				if (transitionCount == 20) {
					ret = false;
				}
			} else {
				setNewNixieDigit((digit + 1) % 10);
			}
		}

		return ret;
	}
	virtual void setDigitMap(const char *map) {
		for (int i=0; i < strlen(map); i++) {
			byte digit = i;
			char cDigit = map[i];

			if (cDigit >= 'a' && cDigit <= 'f') {
				digit = cDigit - 'a' + 10;
			} else if (cDigit >= 'A' && cDigit <= 'F') {
				digit = cDigit - 'A' + 10;
			} else if (cDigit >= '0' && cDigit <= '9') {
				digit = cDigit - '0';
			} else {
				continue;
			}

			digitMap[digit] = i;
		}

		cacheColonMap();
	}

protected:
	virtual void ICACHE_RAM_ATTR interruptHandler() = 0;
	virtual bool ICACHE_RAM_ATTR calculateFade(unsigned long nowMs);
	virtual void cacheColonMap();
	byte ICACHE_RAM_ATTR mapDigit(byte digit) {
		return digitMap[digit];
	}

	byte numMultiplex = 1;
	byte multiplexCount = 0;
	byte cycleCount = 0;
	volatile byte transitionCount = 0;
	volatile uint32_t nextDigit = 0;
	volatile byte colonMask = 0;
	volatile byte transition = 0;
	volatile unsigned long startFade = 0;
	volatile bool displayOn = true;
	volatile DisplayMode displayMode = NO_FADE_DELAY;
	volatile byte indicator = 0;
	volatile byte brightness = 50;
	volatile uint32_t digit = -1;
	SoftPWM fadeInPWM = SoftPWM(0, 1);
	SoftPWM fadeOutPWM = SoftPWM(100, 1);
	SoftPWM displayPWM = SoftPWM(100, 1);
	#define FADE_CHANGE_QUANT 1
	#define FADE_TIME 400
	#define FADE_TIME2 800

private:
	static byte digitMap[13];

	static volatile uint32_t callCycleCount;
	static volatile int _guard;
	static NixieDriver *_handler;

	// Can't be using any system calls in case some other
	// weird system calls are being called, because they can do weird
	// things with flash. For the same reason, this function needs to be
	// in RAM.
	static void isr();
};

#endif /* LIBRARIES_NIXIEDRIVER_NIXIEDRIVER_H_ */
