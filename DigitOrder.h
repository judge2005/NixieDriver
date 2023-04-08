/*
 * DigitOrder.h
 *
 *  Created on: Apr 8, 2023
 *      Author: Nemesis
 */

#ifndef LIBRARIES_NIXIEDRIVER_DIGITORDER_H_
#define LIBRARIES_NIXIEDRIVER_DIGITORDER_H_

#include <NixieDriver.h>

/*
 * Order that digits are shifted in to the register
 */
class DigitOrder {
public:
	typedef uint64_t (*pShiftFn)(uint64_t, int);

	DigitOrder(pShiftFn shiftFn) : shiftDigit(shiftFn) {}
	DigitOrder(const DigitOrder& other) : shiftDigit(other.shiftDigit) {}

	uint64_t operator()(const uint64_t mask, int shift) { return shiftDigit(mask, shift); }

private:
	// Poor man's vtable
	pShiftFn shiftDigit;
};


class LRDigitOrder : public DigitOrder {
public:
	LRDigitOrder() : DigitOrder(_shiftDigit) {}
	LRDigitOrder(const LRDigitOrder& other) : DigitOrder(other) {}

private:
	static NIXIE_DRIVER_ISR_FLAG uint64_t _shiftDigit(uint64_t cMask, int digit);
};

class RLDigitOrder : public DigitOrder {
public:
	RLDigitOrder() : DigitOrder(_shiftDigit) {}
	RLDigitOrder(const RLDigitOrder& other) : DigitOrder(other) {}

private:
	static NIXIE_DRIVER_ISR_FLAG uint64_t _shiftDigit(uint64_t cMask, int digit);
};

class DriverDigitOrder : public DigitOrder {
public:
	DriverDigitOrder() : DigitOrder(_shiftDigit) {}
	DriverDigitOrder(const DriverDigitOrder& other) : DigitOrder(other) {}

private:
	static NIXIE_DRIVER_ISR_FLAG uint64_t _shiftDigit(uint64_t cMask, int shift);
};


#endif /* LIBRARIES_NIXIEDRIVER_DIGITORDER_H_ */
