/* DMX interface routines */

extern "C" {
#include "usb-dmx.h"
}

#include "RingBuffer.h"

CRingBuffer<uint8_t, 128> dmxRingBuffer;

/* send the buffer out to the DMX ports, bitbanging */
extern "C" void
dmx_transmit(uint8_t* pointer, uint8_t length)
{
	SET_BIT(PORTC, 6);
  PORTC |= 0x40;

#if 1
  cli();
  while (length > 0) {
    PORTD = *pointer++;
    PORTB = *pointer++;
    length -= 2;

		_delay_us(3.125);
  }
  sei();
#endif

	CLEAR_BIT(PORTC, 6);
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
