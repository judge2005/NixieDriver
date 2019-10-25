/*
 * ITS1ANixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_ITS1ANIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_ITS1ANIXIEDRIVER_H_
#ifdef ESP8266
#include <NixieDriver.h>


class ITS1ANixieDriver : public NixieDriver {
public:
	ITS1ANixieDriver() {}
	ITS1ANixieDriver(byte numTubes) : numTubes(numTubes) {}
	virtual void init();
	virtual ~ITS1ANixieDriver() {}

	void setResetTime(unsigned int resetTimeU) {
		this->resetTimeU = resetTimeU;
	}

	void setSetTime(unsigned int setTimeU) {
		this->setTimeU = setTimeU;
	}

	static const uint32_t segMap[];

	/*
	 * Overflow-safe timer that can also be enabled/disabled
	 */
	class Timer {
	public:
		Timer(unsigned int duration) : duration(duration), enabled(false), lastTick(0) {}
		Timer() : duration(0), enabled(false), lastTick(0) {}

		NIXIE_DRIVER_ISR_FLAG bool expired(uint32_t now);
		NIXIE_DRIVER_ISR_FLAG void init(uint32_t now, unsigned int duration);
		NIXIE_DRIVER_ISR_FLAG void clear();
	private:
		unsigned int duration;
		bool enabled = false;
		uint32_t lastTick;
	};

protected:
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask);
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit);
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getMultiplexPins();
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins);
	virtual bool NIXIE_DRIVER_ISR_FLAG calculateFade(uint32_t nowMs);
	virtual void NIXIE_DRIVER_ISR_FLAG interruptHandler();

private:
	byte numTubes = 1;
	byte portBMask = 0;

	unsigned int resetTimeU = 3500;
	unsigned int setTimeU = 150;
};
#endif /* ESP8266 */
#endif /* LIBRARIES_NIXIEDRIVER_ITS1ANIXIEDRIVER_H_ */
