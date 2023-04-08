/*
 * DigitOrder.cpp
 *
 *  Created on: Apr 8, 2023
 *      Author: Nemesis
 */

#include <DigitOrder.h>

NIXIE_DRIVER_ISR_FLAG uint64_t LRDigitOrder::_shiftDigit(uint64_t cMask, int digit) {
	return cMask << (10*digit);
}

NIXIE_DRIVER_ISR_FLAG uint64_t RLDigitOrder::_shiftDigit(uint64_t cMask, int digit) {
	return cMask << (10*(5-digit));
}

NIXIE_DRIVER_ISR_FLAG uint64_t DriverDigitOrder::_shiftDigit(uint64_t cMask, int shift) {
	return cMask << shift;
}


