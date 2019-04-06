/*
 * ITS1ANixieDriver.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: Paul Andrews
 */

#include <ITS1ANixieDriver.h>

static const byte ANODE_PIN = 6;
static const byte HV_PIN = 7;
static const byte IO_I2C_ADDRESS = 0x20;

#define SCROLL_HOLD_MS 100

static const byte gridPins[6] = {
	0, 1, 2, 3, 4, 5
};

static const byte tubeOrder[6] = {
	2, 0, 3, 4, 1, 5
};

// Digit to segPins indices
const unsigned long ITS1ANixieDriver::segMap[13] = {
	0x3F,		// 0
	0x06,		// 1
	0x5B,		// 2
	0x4F,		// 3
	0x66,		// 4
	0x6D,		// 5
	0x7D,		// 6
	0x07,		// 7
	0x7F,		// 8
	0x6F,		// 9
	0x01,		// dp1
	0x08,		// dp2
	0x0, 		// Nothing
};

// Digit to segPins indices
static const unsigned long down1SegMap[13] = {
	0x54,		// 0
	0x04,		// 1
	0x4C,		// 2
	0x4C,		// 3
	0x1C,		// 4
	0x58,		// 5
	0x58,		// 6
	0x44,		// 7
	0x5C,		// 8
	0x5C,		// 9
	0x40,		// dp1
	0x00,		// dp2
	0x0, 		// Nothing
};

// Digit to segPins indices
static const unsigned long down2SegMap[13] = {
	0x08,		// 0
	0x00,		// 1
	0x08,		// 2
	0x08,		// 3
	0x00,		// 4
	0x08,		// 5
	0x08,		// 6
	0x08,		// 7
	0x08,		// 8
	0x08,		// 9
	0x08,		// dp1
	0x00,		// dp2
	0x0, 		// Nothing
};

// Digit to segPins indices
static const unsigned long up1SegMap[13] = {
	0x62,		// 0
	0x02,		// 1
	0x61,		// 2
	0x43,		// 3
	0x03,		// 4
	0x43,		// 5
	0x63,		// 6
	0x02,		// 7
	0x63,		// 8
	0x43,		// 9
	0x00,		// dp1
	0x40,		// dp2
	0x0, 		// Nothing
};

// Digit to segPins indices
static const unsigned long up2SegMap[13] = {
	0x01,		// 0
	0x00,		// 1
	0x01,		// 2
	0x01,		// 3
	0x00,		// 4
	0x01,		// 5
	0x01,		// 6
	0x00,		// 7
	0x01,		// 8
	0x00,		// 9
	0x00,		// dp1
	0x01,		// dp2
	0x0, 		// Nothing
};

// Digit to segPins indices
static const unsigned long emptySegMap[13] = {
	0x00,		// 0
	0x00,		// 1
	0x00,		// 2
	0x00,		// 3
	0x00,		// 4
	0x00,		// 5
	0x00,		// 6
	0x00,		// 7
	0x00,		// 8
	0x00,		// 9
	0x00,		// dp1
	0x00,		// dp2
	0x0, 		// Nothing
};

static const unsigned long *scrollDownMaps[6] = {
		down1SegMap,
		down2SegMap,
		emptySegMap,
		up2SegMap,
		up1SegMap,
		ITS1ANixieDriver::segMap
};

static const unsigned long *scrollUpMaps[6] = {
		up1SegMap,
		up2SegMap,
		emptySegMap,
		down2SegMap,
		down1SegMap,
		ITS1ANixieDriver::segMap
};

static const unsigned long *activeSegMap = scrollDownMaps[5];
static const unsigned long *scrollSegMap = scrollDownMaps[5];

//unsigned char twiDcount = 19; // 100KHz
unsigned char twiDcount = 1;	// 400KHz
//static uint32_t twiClockStretchLimit = 230 * 3;
static uint32_t twiClockStretchLimit = 230 * 3;

// SDA and SCL pins
static unsigned char SDA_PIN = 0, SCL_PIN = 2;

#define SDA_LOW()   (GPES = (1 << SDA_PIN)) //Enable SDA (becomes output and since GPO is 0 for the pin, it will pull the line low)
#define SDA_HIGH()  (GPEC = (1 << SDA_PIN)) //Disable SDA (becomes input and since it has pullup it will go high)
#define SDA_READ()  ((GPI & (1 << SDA_PIN)) != 0)
#define SCL_LOW()   (GPES = (1 << SCL_PIN))
#define SCL_HIGH()  (GPEC = (1 << SCL_PIN))
#define SCL_READ()  ((GPI & (1 << SCL_PIN)) != 0)

ICACHE_RAM_ATTR void twiDelay(int count) {
	unsigned int reg;
	for(int i=0;i<count;i++)
		reg = GPI;
}

ICACHE_RAM_ATTR void writeStop(void){
  uint32_t i = 0;
  SCL_LOW();
  SDA_LOW();
  twiDelay(twiDcount);
  SCL_HIGH();
  while (SCL_READ() == 0 && (i++) < twiClockStretchLimit); // Clock stretching
  twiDelay(twiDcount);
  SDA_HIGH();
  twiDelay(twiDcount);
}

ICACHE_RAM_ATTR bool writeBit(bool bit) {
  uint32_t i = 0;
  SCL_LOW();
  if (bit) SDA_HIGH();
  else SDA_LOW();
  twiDelay(twiDcount+1);
  SCL_HIGH();
  while (SCL_READ() == 0 && (i++) < twiClockStretchLimit);// Clock stretching
  twiDelay(twiDcount);
  return true;
}

ICACHE_RAM_ATTR bool readBit(void) {
  uint32_t i = 0;
  SCL_LOW();
  SDA_HIGH();
  twiDelay(twiDcount+2);
  SCL_HIGH();
  while (SCL_READ() == 0 && (i++) < twiClockStretchLimit);// Clock stretching
  bool bit = SDA_READ();
  twiDelay(twiDcount);
  return bit;
}

ICACHE_RAM_ATTR bool writeByte(unsigned char byte) {
  unsigned char bit;
  for (bit = 0; bit < 8; bit++) {
    writeBit(byte & 0x80);
    byte <<= 1;
  }
  return !readBit();//NACK/ACK
}

ICACHE_RAM_ATTR bool writeBuf(byte *buffer, byte length) {
	unsigned int i;

	// twi_write_start
	SCL_HIGH();
	SDA_HIGH();
	if (SDA_READ() == 0)
		return false;
	twiDelay(twiDcount);
	SDA_LOW();
	twiDelay(twiDcount);

	// Write address
	if (!writeByte(((IO_I2C_ADDRESS << 1) | 0) & 0xFF)) {
		writeStop();
		return false; //received NACK on transmit of address
	}

	// Write passed value, LSB first
    for (i=0; i<length; i++) {
		if (!writeByte(buffer[i] & 0xff)) {
			writeStop();
			return false; //received NACK on transmit of data
		}
    }

	writeStop();
	i = 0;

	while (SDA_READ() == 0 && (i++) < 10) {
		SCL_LOW();
		twiDelay(twiDcount);
		SCL_HIGH();
		twiDelay(twiDcount);
	}

	return true;
}

ICACHE_RAM_ATTR bool writeValue(unsigned long value) {
	byte buffer[2];

	buffer[0] = value & 0xff;
	buffer[1] = (value >> 8) & 0xff;

	return writeBuf(buffer, 2);
}

ICACHE_RAM_ATTR bool writePortA(unsigned long value) {
	return writeValue((value << 8) | 0x12);
}

ICACHE_RAM_ATTR bool writePortB(unsigned long value) {
	return writeValue((value << 8) | 0x13);
}

bool ITS1ANixieDriver::Timer::expired(uint32_t now) {
	bool ret = enabled && (now - lastTick >= duration);
	if (ret) {
		enabled = false;
	}

	return ret;
}

void ITS1ANixieDriver::Timer::init(uint32_t now, unsigned int duration) {
	this->duration = duration;
	lastTick = now;
	enabled = true;
}

void ITS1ANixieDriver::Timer::clear() {
	enabled = false;
}

#define MODE_TWO
void ITS1ANixieDriver::init() {
	displayMode = NO_FADE_DELAY;
	nextDigit = 0;

	pinMode(SDA_PIN, INPUT_PULLUP);
	pinMode(SCL_PIN, INPUT_PULLUP);

	writeValue(0 | 1); // LSB first: Set all of port B to outputs | IODIRB register
	writeValue(0 | 0); // LSB first: Set all of port A to outputs | IODIRA register

	portBMask = 0xff;
	writePortB(portBMask);	// Set initial state of anode and 2nd grid pins

	digit = 0x111111;
	nextDigit = 0;
	activeSegMap = segMap;

	interruptHandler();

	NixieDriver::init();
}

unsigned long ITS1ANixieDriver::getPins(byte mask) {
	if (mask != 0) {
		return 0x80;
	} else {
		return 0;
	}
}

unsigned long ITS1ANixieDriver::convertPolarity(unsigned long pins) {
	return pins ^ 0x7F;
}

unsigned long ITS1ANixieDriver::getMultiplexPins() {
	return 0;
}

unsigned long ITS1ANixieDriver::getPin(uint32_t digit) {
	return activeSegMap[digit];
}

static byte scrollIndex = 0;

bool ITS1ANixieDriver::calculateFade(unsigned long nowMs) {
	if (displayMode == NO_FADE_DELAY) {
		scrollIndex = 5;
		scrollSegMap = segMap;
		return false;
	}

	bool running = true;

	scrollIndex++;

	if (scrollIndex >= 6) {
		scrollIndex = 0;
		running = false;
	}

	if (displayMode == FADE_OUT) {
		scrollSegMap = scrollDownMaps[scrollIndex];
	} else {
		scrollSegMap = scrollUpMaps[scrollIndex];
	}

	return running;
}

/*
 * * Segments and grid are active-low 0V/5V signals.
 * * Segments turn on when segment AND grid inputs are low.
 * * Segments are latching (remain on), turn off by removing anode voltages (+100V and +40V) for at least 500 us.
 */
void ITS1ANixieDriver::interruptHandler() {
	static ITS1ANixieDriver::Timer anodeLowTimer;
	static ITS1ANixieDriver::Timer anodeHighTimer;
	static ITS1ANixieDriver::Timer gridHighTimer;
	static ITS1ANixieDriver::Timer segmentStartTimer;
	static ITS1ANixieDriver::Timer segmentHoldTimer;
	static volatile byte tube = 0;
	static volatile byte colonMask;
	static volatile bool updating = false;
	static volatile uint32_t _cd;
	static volatile uint32_t _nd;

	unsigned long int nowU = micros();

	if ((digit != nextDigit) && !updating) {
		updating = true;
		_cd = digit;
		_nd = nextDigit;
		if (displayOn) {
			colonMask = getPins(this->colonMask);
			bitClear(portBMask, HV_PIN);
		} else {
			colonMask = 0;
			bitSet(portBMask, HV_PIN);
		}

		anodeLowTimer.init(nowU, 0);
	}

	if (anodeLowTimer.expired(nowU)) {
		bitClear(portBMask, ANODE_PIN);
		writePortB(portBMask);

		anodeHighTimer.init(nowU, resetTimeU);
	}

	if (anodeHighTimer.expired(nowU)) {
		bitSet(portBMask, ANODE_PIN);
		writePortB(portBMask);
		segmentStartTimer.init(nowU, 0);
	}

	if (segmentStartTimer.expired(nowU)) {
		uint32_t cd = (_cd >> (tubeOrder[tube] * 4)) & 0x0f;
		uint32_t nd = (_nd >> (tubeOrder[tube] * 4)) & 0x0f;
		uint32_t displayDigit = nd;
		if (cd != nd) {
			if (scrollIndex <= 2) {
				displayDigit = cd;
			}
			activeSegMap = scrollSegMap;
		} else {
			activeSegMap = segMap;
		}
		unsigned long pinMask = colonMask | convertPolarity(getPin(displayDigit));
		writePortA(pinMask);
		segmentHoldTimer.init(nowU, 0);
	}

	if (segmentHoldTimer.expired(nowU)) {
		bitClear(portBMask, gridPins[tubeOrder[tube]]);
		writePortB(portBMask);
		gridHighTimer.init(nowU, setTimeU);
	}

	// Take the grid (set pin) high again
	if (gridHighTimer.expired(nowU)) {
		bitSet(portBMask, gridPins[tubeOrder[tube]]);
		writePortB(portBMask);

		tube++;
		if (tube < numTubes) {
			segmentStartTimer.init(nowU, 0);
		} else {
			// Check
			if (!calculateFade(millis())) {
				// All done
				digit = _nd;
				updating = false;
			} else {
				anodeLowTimer.init(nowU, SCROLL_HOLD_MS*1000L);
			}
			tube = 0;
		}
	}
}
