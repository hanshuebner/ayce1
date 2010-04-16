/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2009  Dean Camera (dean [at] fourwalledcubicle [dot] com)
  Copyright 2009  Denver Gingerich (denver [at] ossguy [dot] com)
	  
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
 *  Main source file for the KeyboardMouse demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */
 
#include "KeyboardMouse.h"

/* Scheduler Task List */
TASK_LIST
{
	{ .Task = USB_USBTask               , .TaskStatus = TASK_STOP },
	{ .Task = USB_Mouse                 , .TaskStatus = TASK_RUN  },
	{ .Task = USB_Keyboard              , .TaskStatus = TASK_RUN  },
};

/* Global Variables */
/** Global structure to hold the current keyboard interface HID report, for transmission to the host */
USB_KeyboardReport_Data_t KeyboardReportData;

/** Global structure to hold the current mouse interface HID report, for transmission to the host */
USB_MouseReport_Data_t    MouseReportData;

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

	/* Hardware Initialization */
	Joystick_Init();
	LEDs_Init();
	
	/* Indicate USB not ready */
	UpdateStatus(Status_USBNotReady);
	
	/* Initialize Scheduler so that it can be used */
	Scheduler_Init();

	/* Initialize USB Subsystem */
	USB_Init();
	
	/* Scheduling - routine never returns, so put this last in the main function */
	Scheduler_Start();
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
EVENT_HANDLER(USB_Connect)
{
	/* Start USB management task */
	Scheduler_SetTaskMode(USB_USBTask, TASK_RUN);

	/* Indicate USB enumerating */
	UpdateStatus(Status_USBEnumerating);
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs and stops the USB management task.
 */
EVENT_HANDLER(USB_Disconnect)
{
	/* Stop running HID reporting and USB management tasks */
	Scheduler_SetTaskMode(USB_USBTask, TASK_STOP);

	/* Indicate USB not ready */
	UpdateStatus(Status_USBNotReady);
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host sets the current configuration
 *  of the USB device after enumeration, and configures the keyboard and mouse device endpoints.
 */
EVENT_HANDLER(USB_ConfigurationChanged)
{
	/* Setup Keyboard Report Endpoint */
	Endpoint_ConfigureEndpoint(KEYBOARD_IN_EPNUM, EP_TYPE_INTERRUPT,
		                       ENDPOINT_DIR_IN, HID_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	/* Setup Keyboard LED Report Endpoint */
	Endpoint_ConfigureEndpoint(KEYBOARD_OUT_EPNUM, EP_TYPE_INTERRUPT,
		                       ENDPOINT_DIR_OUT, HID_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	/* Setup Mouse Report Endpoint */
	Endpoint_ConfigureEndpoint(MOUSE_IN_EPNUM, EP_TYPE_INTERRUPT,
		                       ENDPOINT_DIR_IN, HID_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	/* Indicate USB connected and ready */
	UpdateStatus(Status_USBReady);
}

/** Event handler for the USB_UnhandledControlPacket event. This is used to catch standard and class specific
 *  control requests that are not handled internally by the USB library (including the HID commands, which are
 *  all issued via the control endpoint), so that they can be handled appropriately for the application.
 */
EVENT_HANDLER(USB_UnhandledControlPacket)
{
	uint8_t* ReportData;
	uint8_t  ReportSize;

	/* Handle HID Class specific requests */
	switch (USB_ControlRequest.bRequest)
	{
		case REQ_GetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
	
				/* Determine if it is the mouse or the keyboard data that is being requested */
				if (!(USB_ControlRequest.wIndex))
				{
					ReportData = (uint8_t*)&KeyboardReportData;
					ReportSize = sizeof(KeyboardReportData);
				}
				else
				{
					ReportData = (uint8_t*)&MouseReportData;
					ReportSize = sizeof(MouseReportData);
				}

				/* Write the report data to the control endpoint */
				Endpoint_Write_Control_Stream_LE(ReportData, ReportSize);

				/* Clear the report data afterwards */
				memset(ReportData, 0, ReportSize);
				
				/* Finalize the stream transfer to send the last packet or clear the host abort */
				Endpoint_ClearOUT();
			}
		
			break;
		case REQ_SetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				
				/* Wait until the LED report has been sent by the host */
				while (!(Endpoint_IsOUTReceived()));

				/* Read in the LED report from the host */
				uint8_t LEDStatus = Endpoint_Read_Byte();
				uint8_t LEDMask   = LEDS_LED2;
				
				if (LEDStatus & 0x01) // NUM Lock
				  LEDMask |= LEDS_LED1;
				
				if (LEDStatus & 0x02) // CAPS Lock
				  LEDMask |= LEDS_LED3;

				if (LEDStatus & 0x04) // SCROLL Lock
				  LEDMask |= LEDS_LED4;

				/* Set the status LEDs to the current HID LED status */
				LEDs_SetAllLEDs(LEDMask);

				/* Clear the endpoint data */
				Endpoint_ClearOUT();

				/* Acknowledge status stage */
				while (!(Endpoint_IsINReady()));
				Endpoint_ClearIN();
			}
			
			break;
	}
}

/** Function to manage status updates to the user. This is done via LEDs on the given board, if available, but may be changed to
 *  log to a serial port, or anything else that is suitable for status updates.
 *
 *  \param CurrentStatus  Current status of the system, from the KeyboardMouse_StatusCodes_t enum
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

/** Keyboard task. This generates the next keyboard HID report for the host, and transmits it via the
 *  keyboard IN endpoint when the host is ready for more data. Additionally, it processes host LED status
 *  reports sent to the device via the keyboard OUT reporting endpoint.
 */
TASK(USB_Keyboard)
{
	uint8_t JoyStatus_LCL = Joystick_GetStatus();

	/* Check if board button is not pressed, if so mouse mode enabled */
	if (!(Buttons_GetStatus() & BUTTONS_BUTTON1))
	{
		if (JoyStatus_LCL & JOY_UP)
		  KeyboardReportData.KeyCode[0] = 0x04; // A
		else if (JoyStatus_LCL & JOY_DOWN)
		  KeyboardReportData.KeyCode[0] = 0x05; // B

		if (JoyStatus_LCL & JOY_LEFT)
		  KeyboardReportData.KeyCode[0] = 0x06; // C
		else if (JoyStatus_LCL & JOY_RIGHT)
		  KeyboardReportData.KeyCode[0] = 0x07; // D

		if (JoyStatus_LCL & JOY_PRESS)
		  KeyboardReportData.KeyCode[0] = 0x08; // E
	}
	
	/* Check if the USB system is connected to a host and report protocol mode is enabled */
	if (USB_IsConnected)
	{
		/* Select the Keyboard Report Endpoint */
		Endpoint_SelectEndpoint(KEYBOARD_IN_EPNUM);

		/* Check if Keyboard Endpoint Ready for Read/Write */
		if (Endpoint_IsReadWriteAllowed())
		{
			/* Write Keyboard Report Data */
			Endpoint_Write_Stream_LE(&KeyboardReportData, sizeof(KeyboardReportData));

			/* Finalize the stream transfer to send the last packet */
			Endpoint_ClearIN();

			/* Clear the report data afterwards */
			memset(&KeyboardReportData, 0, sizeof(KeyboardReportData));
		}

		/* Select the Keyboard LED Report Endpoint */
		Endpoint_SelectEndpoint(KEYBOARD_OUT_EPNUM);

		/* Check if Keyboard LED Endpoint Ready for Read/Write */
		if (Endpoint_IsReadWriteAllowed())
		{		
			/* Read in the LED report from the host */
			uint8_t LEDStatus = Endpoint_Read_Byte();
			uint8_t LEDMask   = LEDS_LED2;
			
			if (LEDStatus & 0x01) // NUM Lock
			  LEDMask |= LEDS_LED1;
			
			if (LEDStatus & 0x02) // CAPS Lock
			  LEDMask |= LEDS_LED3;

			if (LEDStatus & 0x04) // SCROLL Lock
			  LEDMask |= LEDS_LED4;

			/* Set the status LEDs to the current Keyboard LED status */
			LEDs_SetAllLEDs(LEDMask);

			/* Handshake the OUT Endpoint - clear endpoint and ready for next report */
			Endpoint_ClearOUT();
		}
	}
}

/** Mouse task. This generates the next mouse HID report for the host, and transmits it via the
 *  mouse IN endpoint when the host is ready for more data.
 */
TASK(USB_Mouse)
{
	uint8_t JoyStatus_LCL = Joystick_GetStatus();

	/* Check if board button is pressed, if so mouse mode enabled */
	if (Buttons_GetStatus() & BUTTONS_BUTTON1)
	{
		if (JoyStatus_LCL & JOY_UP)
		  MouseReportData.Y =  1;
		else if (JoyStatus_LCL & JOY_DOWN)
		  MouseReportData.Y = -1;

		if (JoyStatus_LCL & JOY_RIGHT)
		  MouseReportData.X =  1;
		else if (JoyStatus_LCL & JOY_LEFT)
		  MouseReportData.X = -1;

		if (JoyStatus_LCL & JOY_PRESS)
		  MouseReportData.Button  = (1 << 0);
	}

	/* Check if the USB system is connected to a host and report protocol mode is enabled */
	if (USB_IsConnected)
	{
		/* Select the Mouse Report Endpoint */
		Endpoint_SelectEndpoint(MOUSE_IN_EPNUM);

		/* Check if Mouse Endpoint Ready for Read/Write */
		if (Endpoint_IsReadWriteAllowed())
		{
			/* Write Mouse Report Data */
			Endpoint_Write_Stream_LE(&MouseReportData, sizeof(MouseReportData));

			/* Finalize the stream transfer to send the last packet */
			Endpoint_ClearIN();

			/* Clear the report data afterwards */
			memset(&MouseReportData, 0, sizeof(MouseReportData));
		}
	}
}
