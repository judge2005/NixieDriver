/*
 * NixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <NixieDriver.h>
#include <SPI.h>

bool NIXIE_DRIVER_ISR_FLAG SoftPWM::off() {
	count = (count + quantum) % 100;
	return count >= onPercent;
}

/*
 * Every microsecond we get 80 instruction cycles (80MHz). If we set the
 * callCycleCount to 10240, that means that we delay 10240/80 micro seconds
 * or 128 micro seconds. This is the period. So the frequency is 7.8KHz.
 */
volatile uint32_t NixieDriver::callCycleCount = ESP.getCpuFreqMHz() * 1024 / 8;
volatile int NixieDriver::_guard = 0;
NixieDriver *NixieDriver::_handler;
#ifdef ESP32
hw_timer_t *NixieDriver::timer = NULL;
#endif

// Maps a digit to an index into the pin map.
byte NixieDriver::digitMap[13] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
};

void NixieDriver::init() {
	guard(true);

	if (!_handler) {
		// First time through, set up interrupt handlers
#ifdef ESP8266
		timer0_isr_init();
		timer0_attachInterrupt(isr);
		timer0_write(ESP.getCycleCount() + ESP.getCpuFreqMHz() * 1024);	// Wait 2 seconds before displaying
#elif ESP32
		timer = timerBegin(0, 80, true); // timer_id = 0; divider=80; countUp = true; So at 80MHz, we have a granularity of 1MHz
		timerAttachInterrupt(timer, isr, true); // edge = true
		timerAlarmWrite(timer, 100, true); // Period = 100 micro seconds
		timerAlarmEnable(timer);
#endif
	}
	_handler = this;
	guard(false);
}

// Can't be using any system calls in case some other
// weird system calls are being called, because they can do weird
// things with flash. For the same reason, this function needs to be
// in RAM.
void NIXIE_DRIVER_ISR_FLAG NixieDriver::isr() {
#ifdef ESP8266
    uint32_t ccount;
    __asm__ __volatile__("esync; rsr %0,ccount":"=a" (ccount));

	timer0_write(ccount + callCycleCount);
#elif ESP32
//	timerAlarmWrite(timer, 1000, false);	// Once per ms
#endif

	if (_guard == 0) {
		if (_handler) {
			_handler->interruptHandler();
		}
	}
}

void NIXIE_DRIVER_ISR_FLAG NixieDriver::guard(bool flag) {
	noInterrupts();
	flag ? _guard++ : _guard--;
	interrupts();
}

void NixieDriver::cacheColonMap() {}
