/*
 * TLC5916IDR_7SegNixieDriver.h
 *
 *  Created on: May 6, 2023
 *      Author: Paul Andrews
 */

#ifndef LIBRARIES_NIXIEDRIVER_TLC5916IDR_7SEGNIXIEDRIVER_H_
#define LIBRARIES_NIXIEDRIVER_TLC5916IDR_7SEGNIXIEDRIVER_H_

#include <HVNixieDriver.h>
#include <DigitOrder.h>

#define TLC5916IDR_7SegNixieDriverMAX_WARMING 6

class TLC5916IDR_7SegNixieDriver : public NixieDriver {
public:
	TLC5916IDR_7SegNixieDriver(int LEpin, DigitOrder digitOrder) :
		LEpin(LEpin), digitShift(digitOrder)
	{}
	TLC5916IDR_7SegNixieDriver(int LEpin) :
		LEpin(LEpin), digitShift(LRDigitOrder())
	{}
	virtual ~TLC5916IDR_7SegNixieDriver();

	virtual void setNumDigits(int numDigits);
	virtual void setAnimation(Animation animation, int direction);
	virtual bool supportsAnimation() { return true; }
	virtual bool animationDone();
	virtual void init();
	void setWarming(byte val);
	void setLeftLED(bool state) { leftLEDState = state; }
	void setRightLED(bool state) { rightLEDState = state; }
	virtual void setBrightness(const byte b);

protected:
	int LEpin;
	volatile bool leftLEDState = false;
	volatile bool rightLEDState = false;
	DigitOrder digitShift;

	static DRAM_CONST const uint32_t digitShiftMap[6];
	static DRAM_CONST const uint32_t segMap[];
	static DRAM_CONST const uint64_t colonMap[6];
	static DRAM_CONST const uint32_t dp1 = 0x100000;
	static DRAM_CONST const uint32_t dp2 = 0x200000;


	virtual uint8_t getSPIMode() { return 0; }

	virtual void NIXIE_DRIVER_ISR_FLAG interruptHandler();
	virtual bool NIXIE_DRIVER_ISR_FLAG calculateFade(uint32_t nowMs);
	virtual bool NIXIE_DRIVER_ISR_FLAG adjustAnimation(uint32_t nowMs);

	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getMultiplexPins();
	virtual uint64_t NIXIE_DRIVER_ISR_FLAG getPins(uint8_t mask);
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG getPin(uint32_t digit);
	virtual uint32_t NIXIE_DRIVER_ISR_FLAG convertPolarity(uint32_t pins);

private:
	volatile Animation animation = ANIMATION_NONE;
	volatile int animatedDigit = 0;
	volatile int numDigits = 6;
	volatile int direction = 0;
	volatile uint32_t startAnimation = 0;
	SoftPWM animatorPWM = SoftPWM(0, 3);
	SoftPWM ledFadeInPWM = SoftPWM(0, 3);
	SoftPWM ledFadeOutPWM = SoftPWM(100, 3);
	SoftPWM ledDisplayPWM = SoftPWM(100, 2);
	SoftPWM warmingPWM = SoftPWM(100, 2);
};

#endif /* LIBRARIES_NIXIEDRIVER_TLC5916IDR_7SEGNIXIEDRIVER_H_ */
