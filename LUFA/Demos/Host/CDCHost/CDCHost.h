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
 *  Header file for CDCHost.c.
 */

#ifndef _CDC_HOST_H_
#define _CDC_HOST_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/pgmspace.h>
		#include <avr/power.h>
		#include <stdio.h>

		#include <LUFA/Version.h>                                // Library Version Information
		#include <LUFA/Drivers/Misc/TerminalCodes.h>             // ANSI Terminal Escape Codes
		#include <LUFA/Drivers/USB/USB.h>                        // USB Functionality
		#include <LUFA/Drivers/Peripheral/SerialStream.h>        // Serial stream driver
		#include <LUFA/Drivers/Board/LEDs.h>                     // LEDs driver
		#include <LUFA/Scheduler/Scheduler.h>                    // Simple scheduler for task management
		
		#include "ConfigDescriptor.h"
		
	/* Macros: */
		/** Pipe number for the CDC data IN pipe */
		#define CDC_DATAPIPE_IN                1

		/** Pipe number for the CDC data OUT pipe */
		#define CDC_DATAPIPE_OUT               2

		/** Pipe number for the CDC notification pipe */
		#define CDC_NOTIFICATIONPIPE           3

	/* Enums: */
		/** Enum for the possible status codes for passing to the UpdateStatus() function. */
		enum CDCHost_StatusCodes_t
		{
			Status_USBNotReady      = 0, /**< USB is not ready (disconnected from a USB device) */
			Status_USBEnumerating   = 1, /**< USB interface is enumerating */
			Status_USBReady         = 2, /**< USB interface is connected and ready */
			Status_EnumerationError = 3, /**< Software error while enumerating the attached USB device */
			Status_HardwareError    = 4, /**< Hardware error while enumerating the attached USB device */
		};

	/* Task Definitions: */
		TASK(USB_CDC_Host);

	/* Event Handlers: */
		HANDLES_EVENT(USB_DeviceAttached);
		HANDLES_EVENT(USB_DeviceUnattached);
		HANDLES_EVENT(USB_DeviceEnumerationComplete);
		HANDLES_EVENT(USB_HostError);
		HANDLES_EVENT(USB_DeviceEnumerationFailed);

	/* Function Prototypes: */
		void UpdateStatus(uint8_t CurrentStatus);
		
#endif
