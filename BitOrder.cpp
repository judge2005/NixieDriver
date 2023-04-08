/*
 * BitOrder.cpp
 *
 *  Created on: Apr 8, 2023
 *      Author: Nemesis
 */

#include <BitOrder.h>
#include <SPI.h>
#ifdef ESP32
#include "esp32-hal-spi.h"
#include "soc/spi_struct.h"
#endif

#ifdef ESP32
struct spi_struct_t {
    spi_dev_t * dev;
#if !CONFIG_DISABLE_HAL_LOCKS
    xSemaphoreHandle lock;
#endif
    uint8_t num;
};

void NIXIE_DRIVER_ISR_FLAG AntiClockwiseBitOrder::_spiWrite64(spi_t * spi, const uint64_t data) {
	uint8_t *d = (uint8_t *)&(data);
	spi->dev->data_buf[1] = d[3] | (d[2] << 8) | (d[1] << 16) | (d[0] << 24);
	spi->dev->data_buf[0] = d[7] | (d[6] << 8) | (d[5] << 16) | (d[4] << 24);

	spi->dev->mosi_dlen.usr_mosi_dbitlen = 63;
	spi->dev->miso_dlen.usr_miso_dbitlen = 0;
	spi->dev->cmd.usr = 1;
	while(spi->dev->cmd.usr);
}

void NIXIE_DRIVER_ISR_FLAG ClockwiseBitOrder::_spiWrite64(spi_t * spi, const uint64_t data) {
	uint32_t *d = (uint32_t *)&(data);
	spi->dev->data_buf[0] = d[1];
	spi->dev->data_buf[1] = d[0];

	spi->dev->mosi_dlen.usr_mosi_dbitlen = 63;
	spi->dev->miso_dlen.usr_miso_dbitlen = 0;
	spi->dev->cmd.usr = 1;
	while(spi->dev->cmd.usr);
}
#else
void NIXIE_DRIVER_ISR_FLAG AntiClockwiseBitOrder::_spiWrite64(const uint64_t data) {
    union {
            uint32_t l;
            uint8_t b[4];
    } data_;

    while(SPI1CMD & SPIBUSY) {}

    // Set Bits to transfer
    const uint32_t mask = ~((SPIMMOSI << SPILMOSI) | (SPIMMISO << SPILMISO));
    // 31 is (8*4-1)
    SPI1U1 = ((SPI1U1 & mask) | ((31 << SPILMOSI) | (31 << SPILMISO)));

    data_.l = (data >> 32) & 0xffffffff;
    SPI1W0 = data_.l;

    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}

    // 31 is (8*4-1)
    SPI1U1 = ((SPI1U1 & mask) | ((31 << SPILMOSI) | (31 << SPILMISO)));

    data_.l = data & 0xffffffff;
    SPI1W0 = data_.l;

    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}
}

void NIXIE_DRIVER_ISR_FLAG ClockwiseBitOrder::_spiWrite64(const uint64_t data) {
    union {
            uint32_t l;
            uint8_t b[4];
    } data_;

    while(SPI1CMD & SPIBUSY) {}

    // Set Bits to transfer
    const uint32_t mask = ~((SPIMMOSI << SPILMOSI) | (SPIMMISO << SPILMISO));
    // 31 is (8*4-1)
    SPI1U1 = ((SPI1U1 & mask) | ((31 << SPILMOSI) | (31 << SPILMISO)));

    data_.l = (data >> 32) & 0xffffffff;
    SPI1W0 = (data_.b[3] | (data_.b[2] << 8) | (data_.b[1] << 16) | (data_.b[0] << 24));

    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}

    // 31 is (8*4-1)
    SPI1U1 = ((SPI1U1 & mask) | ((31 << SPILMOSI) | (31 << SPILMISO)));

    data_.l = data & 0xffffffff;
    SPI1W0 = (data_.b[3] | (data_.b[2] << 8) | (data_.b[1] << 16) | (data_.b[0] << 24));

    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}
}
#endif
