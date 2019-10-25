/*
 * HV5523NixieDriver.h
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_HV5523ESP32NIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_HV5523ESP32NIXIEDRIVER_H_

#include <NixieDriver.h>
#include <SPI.h>

class HV5523ESP32NixieDriver : public NixieDriver {
public:
	HV5523ESP32NixieDriver(int LEpin) : LEpin(LEpin) {}
	~HV5523ESP32NixieDriver() {}

	virtual void init();

protected:
	int LEpin;

	static DRAM_CONST const uint32_t nixieDigitMap[13];
	static DRAM_CONST const uint32_t colonMap[4];
	static uint32_t currentColonMap[4];

	static DRAM_CONST const uint32_t dp1 = 0x100000;
	static DRAM_CONST const uint32_t dp2 = 0x200000;

	uint8_t getSPIMode() { return SPI_MODE1; }
	virtual void cacheColonMap();

	uint32_t NIXIE_DRIVER_ISR_FLAG getMultiplexPins();
	uint32_t NIXIE_DRIVER_ISR_FLAG getPins(byte mask);
	uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit);
	uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins);
	void NIXIE_DRIVER_ISR_FLAG esp32InterruptHandler();
	bool NIXIE_DRIVER_ISR_FLAG esp32CalculateFade(uint32_t nowMs);

	// Never called, but we need to define it
	virtual bool NIXIE_DRIVER_ISR_FLAG calculateFade(uint32_t nowMs);
	virtual void NIXIE_DRIVER_ISR_FLAG interruptHandler();

private:
	static volatile uint32_t callCycleCount;
	static HV5523ESP32NixieDriver *_handler;
#ifdef ESP32
	static hw_timer_t *timer;
#endif


	// Can't be using any system calls in case some other
	// weird system calls are being called, because they can do weird
	// things with flash. For the same reason, this function needs to be
	// in RAM.
	static NIXIE_DRIVER_ISR_FLAG void isr();
};

#endif /* LIBRARIES_NIXIEDRIVER_HV5523ESP32NIXIEDRIVER_H_ */
