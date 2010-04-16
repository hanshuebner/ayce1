/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2009  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the CDC demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#include "usb-dmx.h"

/* Scheduler Task List */
TASK_LIST
{
    { .Task = USB_USBTask          , .TaskStatus = TASK_STOP },
    { .Task = CDC_Task             , .TaskStatus = TASK_STOP },
};

/* Globals: */
/** Contains the current baud rate and other settings of the virtual serial port. While this demo does not use
 *  the physical USART and thus does not use these settings, they must still be retained and returned to the host
 *  upon request or the host will assume the device is non-functional.
 *
 *  These values are set by the host via a class-specific request, however they are not required to be used accurately.
 *  It is possible to completely ignore these value or use other settings as the host is completely unaware of the physical
 *  serial link characteristics and instead sends and receives data in endpoint streams.
 */
CDC_Line_Coding_t LineCoding = { .BaudRateBPS = 9600,
                                 .CharFormat  = OneStopBit,
                                 .ParityType  = Parity_None,
                                 .DataBits    = 8            };

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
EVENT_HANDLER(USB_Connect)
{
    /* Start USB management task */
    Scheduler_SetTaskMode(USB_USBTask, TASK_RUN);
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs and stops the USB management and CDC management tasks.
 */
EVENT_HANDLER(USB_Disconnect)
{
    /* Stop running tasks */
    Scheduler_SetTaskMode(CDC_Task, TASK_STOP);
    Scheduler_SetTaskMode(USB_USBTask, TASK_STOP);
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host set the current configuration
 *  of the USB device after enumeration - the device endpoints are configured and the CDC management task started.
 */
EVENT_HANDLER(USB_ConfigurationChanged)
{
    /* Setup CDC Notification, Rx and Tx Endpoints */
    Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPNUM, EP_TYPE_INTERRUPT,
                               ENDPOINT_DIR_IN, CDC_NOTIFICATION_EPSIZE,
                               ENDPOINT_BANK_SINGLE);

    Endpoint_ConfigureEndpoint(CDC_TX_EPNUM, EP_TYPE_BULK,
                               ENDPOINT_DIR_IN, CDC_TX_EPSIZE,
                               ENDPOINT_BANK_SINGLE);

    Endpoint_ConfigureEndpoint(CDC_RX_EPNUM, EP_TYPE_BULK,
                               ENDPOINT_DIR_OUT, CDC_RX_EPSIZE,
                               ENDPOINT_BANK_DOUBLE);
    
    /* Start tasks */
    Scheduler_SetTaskMode(CDC_Task, TASK_RUN);
}

/** Event handler for the USB_UnhandledControlPacket event. This is used to catch standard and class specific
 *  control requests that are not handled internally by the USB library (including the CDC control commands,
 *  which are all issued via the control endpoint), so that they can be handled appropriately for the application.
 */
EVENT_HANDLER(USB_UnhandledControlPacket)
{
    uint8_t* LineCodingData = (uint8_t*)&LineCoding;

    /* Process CDC specific control requests */
    switch (USB_ControlRequest.bRequest)
    {
        case REQ_GetLineEncoding:
            if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
            {   
                /* Acknowledge the SETUP packet, ready for data transfer */
                Endpoint_ClearSETUP();

                /* Write the line coding data to the control endpoint */
                Endpoint_Write_Control_Stream_LE(LineCodingData, sizeof(CDC_Line_Coding_t));
                
                /* Finalize the stream transfer to send the last packet or clear the host abort */
                Endpoint_ClearOUT();
            }
            
            break;
        case REQ_SetLineEncoding:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                /* Acknowledge the SETUP packet, ready for data transfer */
                Endpoint_ClearSETUP();

                /* Read the line coding data in from the host into the global struct */
                Endpoint_Read_Control_Stream_LE(LineCodingData, sizeof(CDC_Line_Coding_t));

                /* Finalize the stream transfer to clear the last packet from the host */
                Endpoint_ClearIN();
            }
    
            break;
        case REQ_SetControlLineState:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                /* Acknowledge the SETUP packet, ready for data transfer */
                Endpoint_ClearSETUP();
                
                /* NOTE: Here you can read in the line state mask from the host, to get the current state of the output handshake
                         lines. The mask is read in from the wValue parameter in USB_ControlRequest, and can be masked against the
                         CONTROL_LINE_OUT_* masks to determine the RTS and DTR line states using the following code:
                */
                
                /* Acknowledge status stage */
                while (!(Endpoint_IsINReady()));
                Endpoint_ClearIN();
            }
    
            break;
    }
}

// Note that the logic levels are inverted here.  The host driver must
// also invert the bits so that a 0 is sent as high and a 1 is sent as
// low logic level.

#define ZRO_LEVEL 0xFF
#define ONE_LEVEL 0x00

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

void
transmit(uint8_t* pointer, uint8_t length)
{
  PORTC |= 0x40;
  cli();
  while (length > 0) {
    PORTD = *pointer++;
    PORTB = *pointer++;
    length -= 2;
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
    asm volatile ("nop\n");
  }
  sei();
  PORTC &= ~0x40;
}

#define DLE 0x55

#define CMD_START 0

static bool mark_seen = false;
static uint8_t tx_pos = 2;

/** Function to manage CDC data transmission and reception to and from the host. */
TASK(CDC_Task)
{
  PORTC |= 0x20;
  /* Select the Serial Rx Endpoint */
  Endpoint_SelectEndpoint(CDC_RX_EPNUM);

  if (Endpoint_IsOUTReceived()) {
    while (Endpoint_BytesInEndpoint()) {
      uint8_t byte = Endpoint_Read_Byte();
      if (mark_seen) {
        mark_seen = false;
        switch (byte) {
        case DLE:
          goto stuff_byte;
        case CMD_START:
          transmit(reset_buf, 48);
          tx_pos = 2;
          break;
        }
      } else {
        if (byte == DLE) {
          mark_seen = true;
        } else {
        stuff_byte:
          data_buf[tx_pos++] = byte;
          if (tx_pos == 18) {
            tx_pos = 2;
            transmit(data_buf, 22);
            break;
          }
        }
      }
    }
    if (!Endpoint_BytesInEndpoint()) {
      Endpoint_ClearOUT();
    }
  }
  PORTC &= ~0x20;
}
/** Main program entry point. This routine configures the hardware required by the application, then
 *  starts the scheduler to run the application tasks.
 */

int main(void)
{
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    /* Hardware Initialization */

    DDRB = 0xFF;
    DDRD = 0xFF;
    DDRC = 0xF0;

    /* Initialize Scheduler so that it can be used */
    Scheduler_Init();

    /* Initialize USB Subsystem */
    USB_Init();

    /* Scheduling - routine never returns, so put this last in the main function */
    Scheduler_Start();
}

