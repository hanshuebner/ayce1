/* DMX interface routines */

extern "C" {
#include "usb-dmx.h"
}

uint8_t *irq_tx_ptr = NULL;
uint8_t irq_tx_length = 0;

// alle 64 clocks
ISR(TIMER1_COMPA_vect) {
	SET_BIT(PORTC, 4);

#define IRQ_DO_NOTHING 1
#ifdef IRQ_DO_NOTHING
	_delay_us(1.4); // -> 30 zyklen pro bit ungefaehr fuer alles. nunja
	irq_tx_length = 0;
#else
 	if (irq_tx_length > 0) {
		PORTD = *irq_tx_ptr++;
		PORTB = *irq_tx_ptr++;
		irq_tx_length -= 2;
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
	uint8_t _irq_tx_length = 1;
	// wait for irq to have sent out its buffer
	while (true) {
		cli();
		_irq_tx_length = irq_tx_length;
		sei();
		if (_irq_tx_length > 0) {
			_delay_us(3.125);
		} else {
			break;
		}
	}

	cli();
	irq_tx_ptr = pointer;
	irq_tx_length = length;
	sei();
}

void dmx_transmit_data() {
	if (data_buf == data_bufs[0]) {
		data_buf = data_bufs[1];
		dmx_transmit(data_bufs[0], 22);
	} else {
		data_buf = data_bufs[0];
		dmx_transmit(data_bufs[1], 22);
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

			return;

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
				dmx_transmit_data();
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
					dmx_transmit_data();
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
uint8_t *data_buf = data_bufs[0];

uint8_t data_bufs[2][22] =
	{
		{ ONE_LEVEL, ONE_LEVEL, // start bit
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
		},
		{ ONE_LEVEL, ONE_LEVEL, // start bit
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
		},
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
