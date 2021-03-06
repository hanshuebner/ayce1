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
 *  Main source file for the GenericHID demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#include "GenericHID.h"

/* Scheduler Task List */
TASK_LIST
{
	#if !defined(INTERRUPT_CONTROL_ENDPOINT)
	{ .Task = USB_USBTask          , .TaskStatus = TASK_STOP },
	#endif
	
	#if !defined(INTERRUPT_DATA_ENDPOINT)
	{ .Task = USB_HID_Report       , .TaskStatus = TASK_STOP },
	#endif
};

/** Static buffer to hold the last received report from the host, so that it can be echoed back in the next sent report */
static uint8_t LastReceived[GENERIC_REPORT_SIZE];


/** Main program entry point. This routine configures the hardware required by the application, then
 *  starts the scheduler to run the USB management task.
 */
int main(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Indicate USB not ready */
	UpdateStatus(Status_USBNotReady);

	/* Initialize Scheduler so that it can be used */
	Scheduler_Init();

	/* Initialize USB Subsystem */
	USB_Init();
	
	/* Scheduling - routine never returns, so put this last in the main function */
	Scheduler_Start();
}

/** Event handler for the USB_Reset event. This fires when the USB interface is reset by the USB host, before the
 *  enumeration process begins, and enables the control endpoint interrupt so that control requests can be handled
 *  asynchronously when they arrive rather than when the control endpoint is polled manually.
 */
EVENT_HANDLER(USB_Reset)
{
	#if defined(INTERRUPT_CONTROL_ENDPOINT)
	/* Select the control endpoint */
	Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

	/* Enable the endpoint SETUP interrupt ISR for the control endpoint */
	USB_INT_Enable(ENDPOINT_INT_SETUP);
	#endif
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
EVENT_HANDLER(USB_Connect)
{
	#if !defined(INTERRUPT_CONTROL_ENDPOINT)
	/* Start USB management task */
	Scheduler_SetTaskMode(USB_USBTask, TASK_RUN);
	#endif

	/* Indicate USB enumerating */
	UpdateStatus(Status_USBEnumerating);
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs and stops the USB management task.
 */
EVENT_HANDLER(USB_Disconnect)
{
	/* Stop running HID reporting and USB management tasks */
	#if !defined(INTERRUPT_DATA_ENDPOINT)
	Scheduler_SetTaskMode(USB_HID_Report, TASK_STOP);
	#endif

	#if !defined(INTERRUPT_CONTROL_ENDPOINT)
	Scheduler_SetTaskMode(USB_USBTask, TASK_STOP);
	#endif

	/* Indicate USB not ready */
	UpdateStatus(Status_USBNotReady);
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host sets the current configuration
 *  of the USB device after enumeration, and configures the generic HID device endpoints.
 */
EVENT_HANDLER(USB_ConfigurationChanged)
{
	/* Setup Generic IN Report Endpoint */
	Endpoint_ConfigureEndpoint(GENERIC_IN_EPNUM, EP_TYPE_INTERRUPT,
		                       ENDPOINT_DIR_IN, GENERIC_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	#if defined(INTERRUPT_DATA_ENDPOINT)
	/* Enable the endpoint IN interrupt ISR for the report endpoint */
	USB_INT_Enable(ENDPOINT_INT_IN);
	#endif

	/* Setup Generic OUT Report Endpoint */
	Endpoint_ConfigureEndpoint(GENERIC_OUT_EPNUM, EP_TYPE_INTERRUPT,
		                       ENDPOINT_DIR_OUT, GENERIC_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	#if defined(INTERRUPT_DATA_ENDPOINT)
	/* Enable the endpoint OUT interrupt ISR for the report endpoint */
	USB_INT_Enable(ENDPOINT_INT_OUT);
	#endif

	/* Indicate USB connected and ready */
	UpdateStatus(Status_USBReady);
}

/** Event handler for the USB_UnhandledControlPacket event. This is used to catch standard and class specific
 *  control requests that are not handled internally by the USB library (including the HID commands, which are
 *  all issued via the control endpoint), so that they can be handled appropriately for the application.
 */
EVENT_HANDLER(USB_UnhandledControlPacket)
{
	/* Handle HID Class specific requests */
	switch (USB_ControlRequest.bRequest)
	{
		case REQ_GetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				uint8_t GenericData[GENERIC_REPORT_SIZE];

				Endpoint_ClearSETUP();
	
				CreateGenericHIDReport(GenericData);

				/* Write the report data to the control endpoint */
				Endpoint_Write_Control_Stream_LE(&GenericData, sizeof(GenericData));

				/* Finalize the stream transfer to send the last packet or clear the host abort */
				Endpoint_ClearOUT();
			}
		
			break;
		case REQ_SetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				uint8_t GenericData[GENERIC_REPORT_SIZE];

				Endpoint_ClearSETUP();
				
				/* Wait until the generic report has been sent by the host */
				while (!(Endpoint_IsOUTReceived()));

				Endpoint_Read_Control_Stream_LE(&GenericData, sizeof(GenericData));

				ProcessGenericHIDReport(GenericData);
			
				/* Clear the endpoint data */
				Endpoint_ClearOUT();

				/* Wait until the host is ready to receive the request confirmation */
				while (!(Endpoint_IsINReady()));
				
				/* Handshake the request by sending an empty IN packet */
				Endpoint_ClearIN();
			}
			
			break;
	}
}

/** Function to manage status updates to the user. This is done via LEDs on the given board, if available, but may be changed to
 *  log to a serial port, or anything else that is suitable for status updates.
 *
 *  \param CurrentStatus  Current status of the system, from the GenericHID_StatusCodes_t enum
 */
void UpdateStatus(uint8_t CurrentStatus)
{
	uint8_t LEDMask = LEDS_NO_LEDS;
	
	/* Set the LED mask to the appropriate LED mask based on the given status code */
	switch (CurrentStatus)
	{
		case Status_USBNotReady:
			LEDMask = (LEDS_LED1);
			break;
		case Status_USBEnumerating:
			LEDMask = (LEDS_LED1 | LEDS_LED2);
			break;
		case Status_USBReady:
			LEDMask = (LEDS_LED2 | LEDS_LED4);
			break;
	}
	
	/* Set the board LEDs to the new LED mask */
	LEDs_SetAllLEDs(LEDMask);
}

/** Function to process the lest received report from the host.
 *
 *  \param DataArray  Pointer to a buffer where the last report data is stored
 */
void ProcessGenericHIDReport(uint8_t* DataArray)
{
	/*
		This is where you need to process the reports being sent from the host to the device.
		DataArray is an array holding the last report from the host. This function is called
		each time the host has sent a report to the device.
	*/
	
	for (uint8_t i = 0; i < GENERIC_REPORT_SIZE; i++)
	  LastReceived[i] = DataArray[i];
}

/** Function to create the next report to send back to the host at the next reporting interval.
 *
 *  \param DataArray  Pointer to a buffer where the next report data should be stored
 */
void CreateGenericHIDReport(uint8_t* DataArray)
{
	/*
		This is where you need to create reports to be sent to the host from the device. This
		function is called each time the host is ready to accept a new report. DataArray is 
		an array to hold the report to the host.
	*/

	for (uint8_t i = 0; i < GENERIC_REPORT_SIZE; i++)
	  DataArray[i] = LastReceived[i];
}

#if !defined(INTERRUPT_DATA_ENDPOINT)
TASK(USB_HID_Report)
{
	/* Check if the USB system is connected to a host */
	if (USB_IsConnected)
	{
		Endpoint_SelectEndpoint(GENERIC_OUT_EPNUM);
		
		/* Check to see if a packet has been sent from the host */
		if (Endpoint_IsOUTReceived())
		{
			/* Check to see if the packet contains data */
			if (Endpoint_IsReadWriteAllowed())
			{
				/* Create a temporary buffer to hold the read in report from the host */
				uint8_t GenericData[GENERIC_REPORT_SIZE];
				
				/* Read Generic Report Data */
				Endpoint_Read_Stream_LE(&GenericData, sizeof(GenericData));
				
				/* Process Generic Report Data */
				ProcessGenericHIDReport(GenericData);
			}

			/* Finalize the stream transfer to send the last packet */
			Endpoint_ClearOUT();
		}	

		Endpoint_SelectEndpoint(GENERIC_IN_EPNUM);
		
		/* Check to see if the host is ready to accept another packet */
		if (Endpoint_IsINReady())
		{
			/* Create a temporary buffer to hold the report to send to the host */
			uint8_t GenericData[GENERIC_REPORT_SIZE];
			
			/* Create Generic Report Data */
			CreateGenericHIDReport(GenericData);

			/* Write Generic Report Data */
			Endpoint_Write_Stream_LE(&GenericData, sizeof(GenericData));

			/* Finalize the stream transfer to send the last packet */
			Endpoint_ClearIN();
		}
	}
}
#endif

/** ISR for the general Pipe/Endpoint interrupt vector. This ISR fires when an endpoint's status changes (such as
 *  a packet has been received) on an endpoint with its corresponding ISR enabling bits set. This is used to send
 *  HID packets to the host each time the HID interrupt endpoints polling period elapses, as managed by the USB
 *  controller.
 */
ISR(ENDPOINT_PIPE_vect, ISR_BLOCK)
{
	/* Save previously selected endpoint before selecting a new endpoint */
	uint8_t PrevSelectedEndpoint = Endpoint_GetCurrentEndpoint();

	#if defined(INTERRUPT_CONTROL_ENDPOINT)
	/* Check if the control endpoint has received a request */
	if (Endpoint_HasEndpointInterrupted(ENDPOINT_CONTROLEP))
	{
		/* Clear the endpoint interrupt */
		Endpoint_ClearEndpointInterrupt(ENDPOINT_CONTROLEP);

		/* Process the control request */
		USB_USBTask();

		/* Handshake the endpoint setup interrupt - must be after the call to USB_USBTask() */
		USB_INT_Clear(ENDPOINT_INT_SETUP);
	}
	#endif

	#if defined(INTERRUPT_DATA_ENDPOINT)
	/* Check if Generic IN endpoint has interrupted */
	if (Endpoint_HasEndpointInterrupted(GENERIC_IN_EPNUM))
	{
		/* Select the Generic IN Report Endpoint */
		Endpoint_SelectEndpoint(GENERIC_IN_EPNUM);

		/* Clear the endpoint IN interrupt flag */
		USB_INT_Clear(ENDPOINT_INT_IN);

		/* Clear the Generic IN Report endpoint interrupt and select the endpoint */
		Endpoint_ClearEndpointInterrupt(GENERIC_IN_EPNUM);

		/* Create a temporary buffer to hold the report to send to the host */
		uint8_t GenericData[GENERIC_REPORT_SIZE];
		
		/* Create Generic Report Data */
		CreateGenericHIDReport(GenericData);

		/* Write Generic Report Data */
		Endpoint_Write_Stream_LE(&GenericData, sizeof(GenericData));

		/* Finalize the stream transfer to send the last packet */
		Endpoint_ClearIN();
	}

	/* Check if Generic OUT endpoint has interrupted */
	if (Endpoint_HasEndpointInterrupted(GENERIC_OUT_EPNUM))
	{
		/* Select the Generic OUT Report Endpoint */
		Endpoint_SelectEndpoint(GENERIC_OUT_EPNUM);

		/* Clear the endpoint OUT Interrupt flag */
		USB_INT_Clear(ENDPOINT_INT_OUT);

		/* Clear the Generic OUT Report endpoint interrupt and select the endpoint */
		Endpoint_ClearEndpointInterrupt(GENERIC_OUT_EPNUM);

		/* Create a temporary buffer to hold the read in report from the host */
		uint8_t GenericData[GENERIC_REPORT_SIZE];
		
		/* Read Generic Report Data */
		Endpoint_Read_Stream_LE(&GenericData, sizeof(GenericData));
		
		/* Process Generic Report Data */
		ProcessGenericHIDReport(GenericData);

		/* Finalize the stream transfer to send the last packet */
		Endpoint_ClearOUT();
	}
	#endif

	/* Restore previously selected endpoint */
	Endpoint_SelectEndpoint(PrevSelectedEndpoint);
}
