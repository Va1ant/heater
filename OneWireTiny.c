// all timings explained in datasheet to ds18s20 by maximintegrated
// https://datasheets.maximintegrated.com/en/ds/DS18S20.pdf
// note that each cycle on 1MHz MCU freq is 1us
// and some instructions need more than 1 cycle to execute

#include <avr/interrupt.h>
#include "OneWireTiny.h"
#include <util/delay.h>
#include "Macro.h"

static void dsWriteBit(const _Bool bit, const uint8_t pin) {
	pinToLow(pin);
	pinAsOutput(pin);
	if (bit) {
		pinToHigh(pin);
		_delay_us(55);
	} else {
		_delay_us(57);
		pinToHigh(pin);
	}
}

static _Bool dsReadBit(const uint8_t pin) {
	pinAsOutput(pin);
	pinToLow(pin);
	_delay_loop_1(1);
	
	pinAsInput(pin);
	_delay_loop_1(3);
	
	_Bool bit = readPin(pin);
	// pull up here, if using internal pull up
	_delay_us(40);
	return bit;
}

static _Bool dsReset(void) {
	uint8_t retries = 100;
	pinAsInput(TERMO_PIN);
	while (!readPin(TERMO_PIN))
		if (--retries == 0) return 0;

	pinToLow(TERMO_PIN);
	pinAsOutput(TERMO_PIN);
	_delay_us(480);

	pinAsInput(TERMO_PIN);
	_delay_us(70);

	_Bool state = !readPin(TERMO_PIN);
	_delay_us(400);
	return state;
}

void dsWrite(const uint8_t byte, const uint8_t pin) {
	uint8_t cSREG = SREG;
	cli();
	for (uint8_t bitMask = 1; bitMask; bitMask <<= 1)
		dsWriteBit(((bitMask & byte) ? 1 : 0), pin);
	SREG = cSREG;
	
	// disable power
	pinAsInput(pin);
	pinToLow(pin);
}

uint8_t dsRead(const uint8_t pin) {
	uint8_t byte = 0;
	uint8_t cSREG = SREG;
	cli();
	for (uint8_t BitMask = 0x01; BitMask; BitMask <<= 1)
		if (dsReadBit(pin)) byte |= BitMask;
	SREG = cSREG;
	return byte;
}

uint8_t dsCrc(const uint8_t *addr, uint8_t len) {
	uint8_t crc = 0;
	while (len--) {
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}

_Bool dsTermo(const uint8_t cmd) {
	if (!dsReset()) return 0;
	dsWrite(SKIP_ROM, TERMO_PIN);
	dsWrite(cmd, TERMO_PIN);
	return 1;
}
