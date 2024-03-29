/*
 * MicrochipESP327SegDriver.h
 *
 *  Created on: May 22, 2021
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_MicrochipESP327SegDriver_H_
#define LIBRARIES_NIXIEDRIVER_MicrochipESP327SegDriver_H_

#include <NixieDriver.h>
#include <SPI.h>
#include <BitOrder.h>
#include <DigitOrder.h>
#include <Polarity.h>

class MicrochipESP327SegDriver : public NixieDriver {
public:
	MicrochipESP327SegDriver(int LEpin, DigitOrder digitOrder, BitOrder bitOrder, Polarity polarity) :
		LEpin(LEpin), digitShift(digitOrder), bitOrder(bitOrder), invert(polarity)
	{

	}

	~MicrochipESP327SegDriver() {}

	virtual void setNumDigits(int numDigits);
	virtual void setAnimation(Animation animation, int direction);
	virtual bool supportsAnimation() { return true; }
	virtual bool animationDone();
	virtual void init();

protected:
	int LEpin;
	DigitOrder digitShift;
	BitOrder bitOrder;
	Polarity invert;

	static DRAM_CONST const uint32_t digitShiftMap[6];

	static DRAM_CONST const uint32_t segMap[];
	static DRAM_CONST const uint64_t colonMap[6];

	static DRAM_CONST const uint32_t dp1 = 0x100000;
	static DRAM_CONST const uint32_t dp2 = 0x200000;

	uint8_t getSPIMode() { return SPI_MODE1; }
	virtual void cacheColonMap();

	uint32_t NIXIE_DRIVER_ISR_FLAG getMultiplexPins();
	uint64_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask);
	uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit);
	uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins);
	void NIXIE_DRIVER_ISR_FLAG esp32InterruptHandler();
	bool NIXIE_DRIVER_ISR_FLAG esp32CalculateFade(uint32_t nowMs);
	bool NIXIE_DRIVER_ISR_FLAG adjustAnimation(uint32_t nowMs);

	// Never called, but we need to define it
	virtual bool NIXIE_DRIVER_ISR_FLAG calculateFade(uint32_t nowMs);
	virtual void NIXIE_DRIVER_ISR_FLAG interruptHandler();

private:
	volatile Animation animation = ANIMATION_NONE;
	volatile int animatedDigit = 0;
	volatile int numDigits = 6;
	volatile int direction = 0;
	volatile uint32_t startAnimation = 0;
	SoftPWM animatorPWM = SoftPWM(0, 3);

	static volatile uint32_t callCycleCount;
	static MicrochipESP327SegDriver *_handler;
#ifdef ESP32
	static hw_timer_t *timer;
#endif


	// Can't be using any system calls in case some other
	// weird system calls are being called, because they can do weird
	// things with flash. For the same reason, this function needs to be
	// in RAM.
	static NIXIE_DRIVER_ISR_FLAG void isr();
};

#endif /* LIBRARIES_NIXIEDRIVER_MicrochipESP327SegDriver_H_ */
