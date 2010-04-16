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
 *  Main source file for the MouseHostWithParser demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */
 
#include "MouseHostWithParser.h"

/* Scheduler Task List */
TASK_LIST
{
	{ .Task = USB_USBTask          , .TaskStatus = TASK_STOP },
	{ .Task = USB_Mouse_Host       , .TaskStatus = TASK_STOP },
};


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
	SerialStream_Init(9600, false);
	LEDs_Init();
	
	/* Indicate USB not ready */
	UpdateStatus(Status_USBNotReady);
	
	/* Initialize Scheduler so that it can be used */
	Scheduler_Init();

	/* Initialize USB Subsystem */
	USB_Init();

	/* Start-up message */
	puts_P(PSTR(ESC_RESET ESC_BG_WHITE ESC_INVERSE_ON ESC_ERASE_DISPLAY
	       "Mouse Host Demo running.\r\n" ESC_INVERSE_OFF));
		   
	/* Scheduling - routine never returns, so put this last in the main function */
	Scheduler_Start();
}

/** Event handler for the USB_DeviceAttached event. This indicates that a device has been attached to the host, and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
EVENT_HANDLER(USB_DeviceAttached)
{
	puts_P(PSTR("Device Attached.\r\n"));
	UpdateStatus(Status_USBEnumerating);

	/* Start USB management task to enumerate the device */
	Scheduler_SetTaskMode(USB_USBTask, TASK_RUN);
}

/** Event handler for the USB_DeviceUnattached event. This indicates that a device has been removed from the host, and
 *  stops the library USB task management process.
 */
EVENT_HANDLER(USB_DeviceUnattached)
{
	/* Stop mouse and USB management task */
	Scheduler_SetTaskMode(USB_USBTask, TASK_STOP);
	Scheduler_SetTaskMode(USB_Mouse_Host, TASK_STOP);

	puts_P(PSTR("Device Unattached.\r\n"));
	UpdateStatus(Status_USBNotReady);
}

/** Event handler for the USB_DeviceEnumerationComplete event. This indicates that a device has been successfully
 *  enumerated by the host and is now ready to be used by the application.
 */
EVENT_HANDLER(USB_DeviceEnumerationComplete)
{
	/* Start Mouse Host task */
	Scheduler_SetTaskMode(USB_Mouse_Host, TASK_RUN);
	
	/* Indicate device enumeration complete */
	UpdateStatus(Status_USBReady);
}

/** Event handler for the USB_HostError event. This indicates that a hardware error occurred while in host mode. */
EVENT_HANDLER(USB_HostError)
{
	USB_ShutDown();

	puts_P(PSTR(ESC_BG_RED "Host Mode Error\r\n"));
	printf_P(PSTR(" -- Error Code %d\r\n"), ErrorCode);

	UpdateStatus(Status_HardwareError);
	for(;;);
}

/** Event handler for the USB_DeviceEnumerationFailed event. This indicates that a problem occurred while
 *  enumerating an attached USB device.
 */
EVENT_HANDLER(USB_DeviceEnumerationFailed)
{
	puts_P(PSTR(ESC_BG_RED "Dev Enum Error\r\n"));
	printf_P(PSTR(" -- Error Code %d\r\n"), ErrorCode);
	printf_P(PSTR(" -- Sub Error Code %d\r\n"), SubErrorCode);
	printf_P(PSTR(" -- In State %d\r\n"), USB_HostState);
	
	UpdateStatus(Status_EnumerationError);
}

/** Function to manage status updates to the user. This is done via LEDs on the given board, if available, but may be changed to
 *  log to a serial port, or anything else that is suitable for status updates.
 *
 *  \param CurrentStatus  Current status of the system, from the MouseHostWithParser_StatusCodes_t enum
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
			LEDMask = (LEDS_LED2);
			break;
		case Status_EnumerationError:
		case Status_HardwareError:
			LEDMask = (LEDS_LED1 | LEDS_LED3);
			break;
		case Status_Busy:
			LEDMask = (LEDS_LED1 | LEDS_LED4);
			break;
	}
	
	/* Set the board LEDs to the new LED mask */
	LEDs_SetAllLEDs(LEDMask);
}

/** Task to set the configuration of the attached device after it has been enumerated, and to read and process
 *  the HID report descriptor and HID reports from the device and display the results onto the board LEDs.
 */
TASK(USB_Mouse_Host)
{
	uint8_t ErrorCode;

	/* Switch to determine what user-application handled host state the host state machine is in */
	switch (USB_HostState)
	{
		case HOST_STATE_Addressed:	
			/* Standard request to set the device configuration to configuration 1 */
			USB_ControlRequest = (USB_Request_Header_t)
				{
					.bmRequestType = (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_DEVICE),
					.bRequest      = REQ_SetConfiguration,
					.wValue        = 1,
					.wIndex        = 0,
					.wLength       = 0,
				};

			/* Select the control pipe for the request transfer */
			Pipe_SelectPipe(PIPE_CONTROLPIPE);

			/* Send the request, display error and wait for device detach if request fails */
			if ((ErrorCode = USB_Host_SendControlRequest(NULL)) != HOST_SENDCONTROL_Successful)
			{
				puts_P(PSTR("Control Error (Set Configuration).\r\n"));
				printf_P(PSTR(" -- Error Code: %d\r\n"), ErrorCode);

				/* Indicate error via status LEDs */
				UpdateStatus(Status_EnumerationError);

				/* Wait until USB device disconnected */
				while (USB_IsConnected);
				break;
			}
			
			USB_HostState = HOST_STATE_Configured;
			break;
		case HOST_STATE_Configured:
			puts_P(PSTR("Getting Config Data.\r\n"));
		
			/* Get and process the configuration descriptor data */
			if ((ErrorCode = ProcessConfigurationDescriptor()) != SuccessfulConfigRead)
			{
				if (ErrorCode == ControlError)
				  puts_P(PSTR("Control Error (Get Configuration).\r\n"));
				else
				  puts_P(PSTR("Invalid Device.\r\n"));

				printf_P(PSTR(" -- Error Code: %d\r\n"), ErrorCode);
				
				/* Indicate error via status LEDs */
				UpdateStatus(Status_EnumerationError);

				/* Wait until USB device disconnected */
				while (USB_IsConnected);
				break;
			}
		
			puts_P(PSTR("Processing HID Report.\r\n"));

			/* LEDs one and two on to indicate busy processing */
			UpdateStatus(Status_Busy);

			/* Get and process the device's first HID report descriptor */
			if ((ErrorCode = GetHIDReportData()) != ParseSuccessful)
			{
				puts_P(PSTR("Report Parse Error.\r\n"));
				printf_P(PSTR(" -- Error Code: %d\r\n"), ErrorCode);
			
				/* Indicate error via status LEDs */
				UpdateStatus(Status_EnumerationError);
				
				/* Wait until USB device disconnected */
				while (USB_IsConnected);
				break;			
			}
			
			/* All LEDs off - ready to indicate key presses */
			UpdateStatus(Status_USBReady);

			puts_P(PSTR("Mouse Enumerated.\r\n"));
				
			USB_HostState = HOST_STATE_Ready;
			break;
		case HOST_STATE_Ready:
			/* Select and unfreeze mouse data pipe */
			Pipe_SelectPipe(MOUSE_DATAPIPE);	
			Pipe_Unfreeze();

			/* Check to see if a packet has been received */
			if (Pipe_IsINReceived())
			{
				/* Check if data has been received from the attached mouse */
				if (Pipe_IsReadWriteAllowed())
				{
					/* Create buffer big enough for the report */
					uint8_t MouseReport[Pipe_BytesInPipe()];

					/* Load in the mouse report */
					Pipe_Read_Stream_LE(MouseReport, Pipe_BytesInPipe());
				
					/* Process the read in mouse report from the device */
					ProcessMouseReport(MouseReport);
				}
				
				/* Clear the IN endpoint, ready for next data packet */
				Pipe_ClearIN();
			}

			/* Freeze mouse data pipe */
			Pipe_Freeze();
			break;
	}
}

/** Processes a read HID report from an attached mouse, extracting out elements via the HID parser results
 *  as required and displays movement and button presses on the board LEDs.
 *
 *  \param MouseReport  Pointer to a HID report from an attached mouse device
 */
void ProcessMouseReport(uint8_t* MouseReport)
{
	uint8_t LEDMask = LEDS_NO_LEDS;

	/* Check each HID report item in turn, looking for mouse X/Y/button reports */
	for (uint8_t ReportNumber = 0; ReportNumber < HIDReportInfo.TotalReportItems; ReportNumber++)
	{
		/* Create a temporary item pointer to the next report item */
		HID_ReportItem_t* ReportItem = &HIDReportInfo.ReportItems[ReportNumber];
		
		bool FoundData;

		if ((ReportItem->Attributes.Usage.Page       == USAGE_PAGE_BUTTON) &&
			(ReportItem->ItemType                    == REPORT_ITEM_TYPE_In))
		{
			/* Get the mouse button value */
			FoundData = USB_GetHIDReportItemInfo(MouseReport, ReportItem);
			
			/* For multi-report devices - if the requested data was not in the issued report, continue */
			if (!(FoundData))
			  continue;

			/* If button is pressed, all LEDs are turned on */
			if (ReportItem->Value)
			  LEDMask = LEDS_ALL_LEDS;
		}
		else if ((ReportItem->Attributes.Usage.Page   == USAGE_PAGE_GENERIC_DCTRL) &&
				 ((ReportItem->Attributes.Usage.Usage == USAGE_X)                  ||
				  (ReportItem->Attributes.Usage.Usage == USAGE_Y))                 &&
				 (ReportItem->ItemType                == REPORT_ITEM_TYPE_In))
		{
			/* Get the mouse relative position value */
			FoundData = USB_GetHIDReportItemInfo(MouseReport, ReportItem);
			
			/* For multi-report devices - if the requested data was not in the issued report, continue */
			if (!(FoundData))
			  continue;
			  
			int16_t DeltaMovement;
			
			if (ReportItem->Attributes.BitSize > 8)
			  DeltaMovement = (int16_t)ReportItem->Value;
			else
			  DeltaMovement = (int8_t)ReportItem->Value;
			
			/* Determine if the report is for the X or Y delta movement */
			if (ReportItem->Attributes.Usage.Usage == USAGE_X)
			{
				/* Turn on the appropriate LED according to direction if the delta is non-zero */
				if (DeltaMovement)
				  LEDMask |= ((DeltaMovement > 0) ? LEDS_LED1 : LEDS_LED2);
			}
			else
			{
				/* Turn on the appropriate LED according to direction if the delta is non-zero */
				if (DeltaMovement)
				  LEDMask |= ((DeltaMovement > 0) ? LEDS_LED3 : LEDS_LED4);
			}
		}
	}
	
	/* Display the button information on the board LEDs */
	LEDs_SetAllLEDs(LEDMask);
}