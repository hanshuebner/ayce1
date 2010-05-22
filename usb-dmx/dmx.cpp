/* DMX interface routines */

extern "C" {
#include "usb-dmx.h"
}

#include "RingBuffer.h"

CRingBuffer<uint8_t, 128> dmxRingBuffer;


// alle 64 clocks
ISR(TIMER1_COMPA_vect) {
	SET_BIT(PORTC, 4);

	asm("nop");
	asm("nop");
	asm("nop");

#if 0
	if (dmxRingBuffer.size() >= 2) {
		PORTD = dmxRingBuffer.get();
	  PORTB = dmxRingBuffer.get();
	}
#endif

	CLEAR_BIT(PORTC, 4);
}

/* set up the ring buffer and the timer to send out ringbuffer bytes */
extern "C" void
dmx_init()
{
	// pwm, phase and frequency correct mode
	TCCR1A = _BV(WGM10) | _BV(WGM11);
	TCCR1B = _BV(WGM13) | _BV(WGM12);
	// every 64 cycles (4 us)
	OCR1A = 63; 
	TCNT1 = 0;
	TIMSK1 |= _BV(OCIE1A); // irq on overflow
	TCCR1B |= _BV(CS10);
}

/* send the buffer out to the DMX ports, bitbanging */
extern "C" inline void
dmx_transmit(uint8_t* pointer, uint8_t length)
{
	for (uint8_t i = 0; i < length; i++) {
		continue;
		
#if 0
		// old version
		SET_BIT(PORTC, 6);

		cli();
		while (length > 0) {
			PORTD = *pointer++;
			PORTB = *pointer++;
			length -= 2;
			
			_delay_us(3.125);
		}
		sei();

	CLEAR_BIT(PORTC, 6);
#else
	// ring buffer version
	dmxRingBuffer.put(pointer[i]);
#endif
	
	}
}

/*
 *
 */
extern "C" void
dmx_decode1(uint8_t byte)
{
	static uint8_t tx_pos = 2;
  static bool mark_seen = false;

	if (mark_seen) {
		mark_seen = false;
		switch (byte) {
		case DLE:
			goto stuff_byte;
				
		case CMD_START:
			SET_BIT(PORTC, 5);
			PORTC |= 0x10;
			dmx_transmit(reset_buf, 48);
			tx_pos = 2;
			CLEAR_BIT(PORTC, 5);
			break;
		}
	} else {
		if (byte == DLE) {
			mark_seen = true;
		} else {
		stuff_byte:
			data_buf[tx_pos++] = byte;
			if (tx_pos == 18) {
				SET_BIT(PORTC, 5);
				dmx_transmit(data_buf, 22);
				CLEAR_BIT(PORTC, 5);
				tx_pos = 2;
			}
		}
	}
}

extern "C" void
dmx_decode(uint8_t *input_buffer, uint8_t input_pointer)
{
  static uint8_t tx_pos = 2;
  static bool mark_seen = false;

	for (int i = 0; i < input_pointer; i++) {
		uint8_t byte = input_buffer[i];
		if (mark_seen) {
			mark_seen = false;
			switch (byte) {
			case DLE:
				goto stuff_byte;
				
			case CMD_START:
				SET_BIT(PORTC, 4);
				PORTC |= 0x10;
				dmx_transmit(reset_buf, 48);
				tx_pos = 2;
				CLEAR_BIT(PORTC, 4);
				break;
			}
		} else {
			if (byte == DLE) {
				mark_seen = true;
			} else {
			stuff_byte:
				data_buf[tx_pos++] = byte;
				if (tx_pos == 18) {
					SET_BIT(PORTC, 5);
					dmx_transmit(data_buf, 22);
					CLEAR_BIT(PORTC, 5);
					tx_pos = 2;
				}
			}
		}
	}
}

/*
 * the format for the buffers is PORTD, PORTB
 */

// One byte, including start, data and stop bits
uint8_t data_buf[22] = { ONE_LEVEL, ONE_LEVEL, // start bit
                         ZRO_LEVEL, ZRO_LEVEL,
                         ZRO_LEVEL, ZRO_LEVEL,
                         ZRO_LEVEL, ZRO_LEVEL,
                         ZRO_LEVEL, ZRO_LEVEL,
                         ZRO_LEVEL, ZRO_LEVEL,
                         ZRO_LEVEL, ZRO_LEVEL,
                         ZRO_LEVEL, ZRO_LEVEL,
                         ZRO_LEVEL, ZRO_LEVEL,
                         ZRO_LEVEL, ZRO_LEVEL, // stop bit 1
                         ZRO_LEVEL, ZRO_LEVEL  // stop bit 2
};

// Reset frame, 88us break, 8us make
uint8_t reset_buf[48] = { ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ONE_LEVEL, ONE_LEVEL, ONE_LEVEL, ONE_LEVEL,
                          ZRO_LEVEL, ZRO_LEVEL, ZRO_LEVEL, ZRO_LEVEL };
