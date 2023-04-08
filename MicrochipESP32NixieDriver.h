/*
 * MicrochipESP32NixieDriver.h
 *
 *  Created on: Feb 10, 2023
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_MICROCHIPESP32NIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_MICROCHIPESP32NIXIEDRIVER_H_

#include <NixieDriver.h>
#include <BitOrder.h>
#include <DigitOrder.h>
#include <Polarity.h>
#include <SPI.h>

/*
 * The driver
 */
class MicrochipESP32NixieDriver : public NixieDriver {
public:
	MicrochipESP32NixieDriver(int LEpin, DigitOrder digitOrder, BitOrder bitOrder, Polarity polarity) :
		LEpin(LEpin), digitShift(digitOrder), bitOrder(bitOrder), invert(polarity)
	{
		copyDigitMap(defaultDigitMap);
	}

	MicrochipESP32NixieDriver(int LEpin, DigitOrder digitOrder, BitOrder bitOrder, Polarity polarity, const uint32_t digitMap[]) :
		LEpin(LEpin), digitShift(digitOrder), bitOrder(bitOrder), invert(polarity)
	{
		copyDigitMap(digitMap);
	}

	~MicrochipESP32NixieDriver() {}

	virtual void setAnimation(Animation animation, int direction);
	virtual bool supportsAnimation() { return true; }
	virtual bool animationDone();
	virtual void init();

	static const int DIGIT_MAP_SIZE = 16;
	static DRAM_CONST const uint32_t cd27DigitMap[DIGIT_MAP_SIZE];
	static DRAM_CONST const uint32_t defaultDigitMap[DIGIT_MAP_SIZE];
protected:
	int LEpin;
	DigitOrder digitShift;
	BitOrder bitOrder;
	Polarity invert;

	static uint32_t currentDigitMap[DIGIT_MAP_SIZE];
	static DRAM_CONST const uint32_t colonMap[6];
	static uint32_t currentColonMap[6];	// Not used

	static DRAM_CONST const uint32_t dp1 = 0x100000;
	static DRAM_CONST const uint32_t dp2 = 0x200000;

	uint8_t getSPIMode() { return SPI_MODE1; }
	virtual void cacheColonMap();
	void copyDigitMap(const uint32_t digitMap[DIGIT_MAP_SIZE]) {
		for (int i=0; i<DIGIT_MAP_SIZE; i++) {
			currentDigitMap[i] = digitMap[i];
		}
	}

	uint32_t NIXIE_DRIVER_ISR_FLAG getMultiplexPins();
	uint32_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask);
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
	volatile int direction = 0;
	volatile uint32_t startAnimation = 0;
	SoftPWM animatorPWM = SoftPWM(0, 3);

	static volatile uint32_t callCycleCount;
	static MicrochipESP32NixieDriver *_handler;
#ifdef ESP32
	static hw_timer_t *timer;
#endif


	// Can't be using any system calls in case some other
	// weird system calls are being called, because they can do weird
	// things with flash. For the same reason, this function needs to be
	// in RAM.
	static NIXIE_DRIVER_ISR_FLAG void isr();
};

#endif /* LIBRARIES_NIXIEDRIVER_MICROCHIPESP32NIXIEDRIVER_H_ */
