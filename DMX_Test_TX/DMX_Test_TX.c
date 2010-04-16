/* DMX_Test_TX.c
**
** Example avr-gcc code to demonstrate the USART on a Tiny2313 by
** rt'ing 8 bit charter after and Break and MAP.
** Covers initialization, rx, and tx.  Settings were determined
** from the table in the Tiny2313 datasheet.  This
** example assumes the Tiny uses an external 12M crystal 
** and sets the USART to 2500 baud, 8 data bits,
** no parity, 2 stop bit. Each byte is about 44us long.  
** THE DMX512 "PACKET"
** The DMX512 Packet is the heart of the Standard, 
it consists of a collection of frames "wrapped up" with further 
synchronization information known as a 'Break' and a 'Mark-after-break'. 
It is this information that enables a receiver to detect 
the start of a frame and hence make some sense of the data it is receiving.
Examination of the frame timing reveals that the maximum time 
that the line can be 'low' is 
4탎 (Start bit) + (8 x 4탎 (Data bits)) = 36탎, 
however a "Break" consists of a 'low' of at least 88탎, 
the difference between these two is easily detectable and 
is used for dimmer synchronization.
The "Mark-after-break" is a 'high' state on the line and 
is at least 8탎 long, the "Mark-after-break" is required so 
the end of the "Break" can be detected otherwise the "Break" would 
run into the Start bit of a frame and everything would get very confused! 
The "Start Code" is a frame (like the others) except that it is 
the first frame after the break and is used as a flag to indicate the 
type of data that follows. A value of '0' indicates that the following 
frames contain dimmer level information. The other 255 codes are not 
defined in the Specification but some manufacturers use other codes 
to send product specific information. A dimmer receiving a non-zero 
start code 'should' ignore the rest of the packet, 
but be careful - it isn't always checked!
The "Inter-frame-time" is used to slow down the data rate - some 
dimmers cannot cope with the data running flat out, or, more usually 
to 'pad' the transmission while the console gets on with some other 
task. It can have any value from 0 to 1s. */

/*
I am developing a DMX RX module, and wanted to a module that would TX a known DMX code.  Thus the following was born.  It was also good for checking the actual BR setting ( as so many have sugested to do first - Thanx)
While the code may not be the most glamerous, it seems to work
I hope it is useful for others.
All comments are welcome.
Cheers
Matt
*/

/* io.h contains the defines for USART registers */
#include <avr/io.h>
#include "delay_x.h"


/* Defines to calculate the UBBR value based on 12Mhz clock speed
** and 250 000 baud. */
#define F_CPU 12000000L
#define FOSC 12000000L
#define BAUD 250000
#define MYUBRR FOSC/16/BAUD-1

#define LED_PORT	PORTD
#define LED_IO		PD6		// indicates program initialisation status
#define LED_USART	PD5		// Indicates USART initialieds
#define LED_TRIG	PD4		// Really for testing purposes only.  toogles with start frame
#define LED_DATA	PD3		// 
#define IO_TRIG		PD2		// Really for testing purposes only.  toogles with start frame
#define IO_TX		PD1		// Tx Pin of USART 

#define DMX_break 90
#define DMX_MAB 20
#define DMX_ch 512


/* buffer size for incoming strings */

/* Declarations for utillity functions. Note that the rx/tx
** functions are blocking. A later section in the tutorial
** demonstrates a non-blocking system. */
void USART_init(unsigned int ubrr);
void USART_tx(unsigned char data);


/* global buffer for incoming strings */


int main()
{
   int count;
   unsigned char x;
	DDRD = 0b01111001;  
	LED_PORT &= 0x00;

//	init_io();
	LED_PORT |= (1<<LED_IO);		// set bit 7

	USART_init( MYUBRR );
	LED_PORT |= (1<<LED_USART);		// set bit 6

	x = 1;

   while(1)
   {
		LED_PORT  &=~(1<<IO_TX);	// Drive the TX Pin Low
		LED_PORT |=(1<<IO_TRIG);	// Set trigger high for testing only good for trigging a scope
		LED_PORT |=(1<<LED_TRIG);	// turn Trigger LED on

		_delay_us(DMX_break);
		LED_PORT |=(1<<IO_TX);
		LED_PORT  &=~(1<<IO_TRIG);	// Set the trigger pulse low  
		_delay_us(DMX_MAB);
		
		UCSRA = (1<<TXC);			// Clear out any 	
	   	UCSRB = (1<<TXEN);			// Enable tx portion of the USART
		LED_PORT  &=~(1<<LED_TRIG);	// Turn Trigger LED off  
		USART_tx(0x00);				// the first byte is always 0
//		USART_tx(0xff);				// 

		for (count=1; count <= DMX_ch; count++)	{							
//			USART_tx(x);
//			x++;
//			if (x==0xff) x=2;
			USART_tx(0x80);
			USART_tx(0x7f);
		}
		while ( !(UCSRA & (1<<TXC)))
		;
		UCSRB &=~(1<<TXEN) ;

   }
   return(0);
}



/*********** USART_tx ******************************************/
/* waits until the UDRE bit is set in UCSRA (which means the
** outgoing tx buffer is clear and we can write to it). Then puts
** a char into the UDR to be sent. */
void USART_tx(unsigned char data)
{
   while( !( UCSRA & (1<<UDRE)) )
      ;
   UDR = data;
}







/*********** USART_init ***************/
/* Typical USART init function, taken mostly from
** the datasheet example. ubrr is a number that
** represent the speed the UART will run at.  There
** is a table in the datasheet that shows typical chip
** clock settings, baud rates, and error %.  Find a combo
** that works for the hardware you have and your
** desired clock setting and baud. The ubrr value is calculated
** in a macro at the top of this file.
**
** Note: the datasheet example does not set URSEL in UCSRC.  This must
** be done for timer0 and timer2, for timer1 it would be clear (URSEL specifies
** if you are reading UCSRC or UBRRH(high baud rate byte for 16 bit timer2 -
** they share the same memory address). */
void USART_init(unsigned int ubrr)
{
   	/* set baud rate, UBRRH is not used for this example */
   	UBRRH = (unsigned char)(ubrr>>8);
   	UBRRL = (unsigned char)ubrr;

   	/* Don't enable rx and tx YET*/
//   	UCSRB = (1<<RXEN) | (1<<TXEN);
		UCSRB &= ~(1 << RXEN) ;
		UCSRB &= ~(1 << TXEN) ;


   	/* set frame settings to 8 data, 2 stop bit */
	UCSRC = (1<<USBS) | (3<<UCSZ0);
}
