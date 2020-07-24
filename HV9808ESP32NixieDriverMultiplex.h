/*
 * HV5523NixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_HV9808ESP32NixieDriverMultiplex_H_
#define LIBRARIES_NIXIEDRIVER_HV9808ESP32NixieDriverMultiplex_H_

#include <NixieDriver.h>
#include <SPI.h>

class HV9808ESP32NixieDriverMultiplex : public NixieDriver {
public:
	HV9808ESP32NixieDriverMultiplex(int LEpin) : LEpin(LEpin) {}
	~HV9808ESP32NixieDriverMultiplex() {}

	void setDate(uint32_t date) { this->date = date; }
	void setColons(uint64_t colons) { this->colons = colons; }

	virtual void setAnimation(Animation animation, int direction);
	virtual bool supportsAnimation() { return true; }
	virtual bool animationDone();
	virtual void init();

protected:
	int LEpin;

	static DRAM_CONST const uint32_t nixieDigitMap[13];
	static DRAM_CONST const uint32_t colonMap[4];
	static uint32_t currentColonMap[4];

	static DRAM_CONST const uint32_t dp1 = 0x100000;
	static DRAM_CONST const uint32_t dp2 = 0x200000;

	uint8_t getSPIMode() { return SPI_MODE0; }
	virtual void cacheColonMap();

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
	volatile uint64_t colons;
	volatile uint32_t date;
	volatile Animation animation = ANIMATION_NONE;
	volatile int animatedDigit = 0;
	volatile int direction = 0;
	volatile uint32_t startAnimation = 0;
	SoftPWM animatorPWM = SoftPWM(0, 3);

	static volatile uint32_t callCycleCount;
	static HV9808ESP32NixieDriverMultiplex *_handler;
#ifdef ESP32
	static hw_timer_t *timer;
#endif


	// Can't be using any system calls in case some other
	// weird system calls are being called, because they can do weird
	// things with flash. For the same reason, this function needs to be
	// in RAM.
	static NIXIE_DRIVER_ISR_FLAG void isr();
};

#endif /* LIBRARIES_NIXIEDRIVER_HV9808ESP32NixieDriverMultiplex_H_ */
