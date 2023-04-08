/*
 * BitOrder.h
 *
 *  Created on: Apr 8, 2023
 *      Author: Nemesis
 */

#ifndef LIBRARIES_NIXIEDRIVER_BITORDER_H_
#define LIBRARIES_NIXIEDRIVER_BITORDER_H_

#include <NixieDriver.h>
#include <SPI.h>

/*
 * Order that bits are shifted in to the register
 */
class BitOrder {
public:
	typedef int (*pOrderFn)();
#ifdef ESP32
	typedef void (*pWriteFn)(spi_t *, const uint64_t);
#else
	typedef void (*pWriteFn)(const uint64_t);
#endif

	BitOrder(pOrderFn orderFn, pWriteFn writeFn) : bitOrder(orderFn), spiWrite64(writeFn) {}
	BitOrder(const BitOrder& other) : bitOrder(other.bitOrder), spiWrite64(other.spiWrite64) {}

	int operator()() { return bitOrder(); }

	// Poor man's vtable
	pWriteFn spiWrite64 = 0;
private:
	pOrderFn bitOrder = 0;
};

class ClockwiseBitOrder : public BitOrder {
public:
	ClockwiseBitOrder() : BitOrder(&_bitOrder, _spiWrite64) {}
	ClockwiseBitOrder(const ClockwiseBitOrder& other) : BitOrder(other) {}

private:
	static int _bitOrder() { return LSBFIRST; }
#ifdef ESP32
	static NIXIE_DRIVER_ISR_FLAG void _spiWrite64(spi_t *spi, const uint64_t data);
#else
	static NIXIE_DRIVER_ISR_FLAG void _spiWrite64(const uint64_t data);
#endif
};

class AntiClockwiseBitOrder : public BitOrder {
public:
	AntiClockwiseBitOrder() : BitOrder(&_bitOrder, _spiWrite64) {}
	AntiClockwiseBitOrder(const AntiClockwiseBitOrder& other) : BitOrder(other) {}

private:
	static int _bitOrder() { return MSBFIRST; }
#ifdef ESP32
	static NIXIE_DRIVER_ISR_FLAG void _spiWrite64(spi_t *spi, const uint64_t data);
#else
	static NIXIE_DRIVER_ISR_FLAG void _spiWrite64(const uint64_t data);
#endif
};

#endif /* LIBRARIES_NIXIEDRIVER_BITORDER_H_ */
