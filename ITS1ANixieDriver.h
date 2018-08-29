/*
 * ITS1ANixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_ITS1ANIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_ITS1ANIXIEDRIVER_H_

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

	static const unsigned long segMap[];

	/*
	 * Overflow-safe timer that can also be enabled/disabled
	 */
	class Timer {
	public:
		Timer(unsigned int duration) : duration(duration), enabled(false), lastTick(0) {}
		Timer() : duration(0), enabled(false), lastTick(0) {}

		ICACHE_RAM_ATTR bool expired(uint32_t now);
		ICACHE_RAM_ATTR void init(uint32_t now, unsigned int duration);
		ICACHE_RAM_ATTR void clear();
	private:
		bool enabled = false;
		unsigned int duration;
		uint32_t lastTick;
	};

protected:
	virtual unsigned long ICACHE_RAM_ATTR getPins(byte mask);
	virtual unsigned long ICACHE_RAM_ATTR getPin(uint32_t digit);
	virtual unsigned long ICACHE_RAM_ATTR getMultiplexPins();
	virtual unsigned long ICACHE_RAM_ATTR convertPolarity(unsigned long pins);
	virtual bool ICACHE_RAM_ATTR calculateFade(unsigned long nowMs);
	virtual void ICACHE_RAM_ATTR interruptHandler();

private:
	byte numTubes = 1;
	byte portBMask = 0;

	unsigned int resetTimeU = 3500;
	unsigned int setTimeU = 150;
};

#endif /* LIBRARIES_NIXIEDRIVER_ITS1ANIXIEDRIVER_H_ */
