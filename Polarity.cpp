/*
 * Polarity.cpp
 *
 *  Created on: Apr 8, 2023
 *      Author: Nemesis
 */

#include <Polarity.h>

uint64_t NIXIE_DRIVER_ISR_FLAG Identity::_transform(uint64_t mask) { return mask; }

uint64_t NIXIE_DRIVER_ISR_FLAG Invert::_transform(uint64_t mask) { return mask ^ 0xffffffffffffffffULL; }




