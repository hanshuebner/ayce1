/* DMX interface routines */

#include "usb-dmx.h"

/* send the buffer out to the DMX ports, bitbanging */
void
dmx_transmit(uint8_t* pointer, uint8_t length)
{
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

  PORTC &= ~0x40;
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
