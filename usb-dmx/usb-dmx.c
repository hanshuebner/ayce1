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

#include "usb-dmx.h"

/* Scheduler Task List */
TASK_LIST
{
    { .Task = USB_USBTask          , .TaskStatus = TASK_STOP },
    { .Task = CDC_Task             , .TaskStatus = TASK_STOP },
};

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

/** Function to manage CDC data transmission and reception to and from the host. */
TASK(CDC_Task)
{
  // Input buffer
  static uint8_t input_buffer[CDC_RX_EPSIZE];
  static uint8_t input_pointer = 0;

  /* Select the Serial Rx Endpoint */
  Endpoint_SelectEndpoint(CDC_RX_EPNUM);

  input_pointer = 0;
  if (Endpoint_IsOUTReceived()) {
    while (Endpoint_BytesInEndpoint()) {
      input_buffer[input_pointer++] = Endpoint_Read_Byte();
    }
    Endpoint_ClearOUT();

		dmx_decode(input_buffer, input_pointer);
  }
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

