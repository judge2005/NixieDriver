/*
 * Polarity.h
 *
 *  Created on: Apr 8, 2023
 *      Author: Nemesis
 */

#ifndef LIBRARIES_NIXIEDRIVER_POLARITY_H_
#define LIBRARIES_NIXIEDRIVER_POLARITY_H_

#include <NixieDriver.h>

/*
 * Polarity - i.e. is a 1 bit displayed as on or off?
 */
class Polarity {
public:
	typedef uint64_t (*pTransform)(const uint64_t);

	Polarity(pTransform transform) : transform(transform) {}
	Polarity(const Polarity& other) : transform(other.transform) {}

	uint64_t operator()(const uint64_t mask) { return transform(mask); }

private:
	pTransform transform;
};

class Identity : public Polarity {
public:
	Identity() : Polarity(_transform) {}
	Identity(const Identity& other) : Polarity(other) {}

private:
	static NIXIE_DRIVER_ISR_FLAG uint64_t _transform(const uint64_t mask);
};

class Invert : public Polarity {
public:
	Invert() : Polarity(_transform) {}
	Invert(const Invert& other) : Polarity(other) {}

private:
	static NIXIE_DRIVER_ISR_FLAG uint64_t _transform(const uint64_t mask);
};


#endif /* LIBRARIES_NIXIEDRIVER_POLARITY_H_ */
