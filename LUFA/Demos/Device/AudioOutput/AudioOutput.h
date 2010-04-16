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
 *  Header file for AudioOutput.c.
 */

#ifndef _AUDIO_OUTPUT_H_
#define _AUDIO_OUTPUT_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/power.h>

		#include "Descriptors.h"
		
		#include <LUFA/Version.h>                    // Library Version Information
		#include <LUFA/Drivers/USB/USB.h>            // USB Functionality
		#include <LUFA/Drivers/Board/LEDs.h>         // LEDs driver
		#include <LUFA/Scheduler/Scheduler.h>        // Simple scheduler for task management
	
	/* Macros: */
		#if defined(USB_FULL_CONTROLLER) || defined(USB_MODIFIED_FULL_CONTROLLER)
			#define TCCRxA          TCCR3A
			#define TCCRxB          TCCR3B
			#define OCRxA           OCR3A
			#define OCRxB           OCR3B
			#define WGMx0           WGM30
			#define WGMx2           WGM32
			#define COMxA1          COM3A1
			#define COMxA0          COM3A0
			#define COMxB1          COM3B1
			#define COMxB0          COM3B0
			#define CSx0            CS30
		#else
			/** Timer count register used for left channel PWM audio output (or mixed output in mono output mode) */
			#define TCCRxA          TCCR1A

			/** Timer count register used for right channel PWM audio output */
			#define TCCRxB          TCCR1B

			/** Timer compare register used for left channel PWM audio output (or mixed output in mono output mode) */
			#define OCRxA           OCR1A

			/** Timer compare register used for right channel PWM audio output */
			#define OCRxB           OCR1B

			/** Timer control register mask used to select PWM mode */
			#define WGMx0           WGM10

			/** Timer control register mask used to select PWM mode */
			#define WGMx2           WGM12

			/** Timer control register mask used to set, clear or toggle channel output pin on match */
			#define COMxA1          COM1A1

			/** Timer control register mask used to set, clear or toggle channel output pin on match */
			#define COMxA0          COM1A0

			/** Timer control register mask used to set, clear or toggle channel output pin on match */
			#define COMxB1          COM1B1

			/** Timer control register mask used to set, clear or toggle channel output pin on match */
			#define COMxB0          COM1B0

			/** Timer control register mask used to start the timer at Fcpu clock rate */
			#define CSx0            CS10
		#endif
		
	/* Enums: */
		/** Enum for the possible status codes for passing to the UpdateStatus() function. */
		enum AudioOutput_StatusCodes_t
		{
			Status_USBNotReady    = 0, /**< USB is not ready (disconnected from a USB host) */
			Status_USBEnumerating = 1, /**< USB interface is enumerating */
			Status_USBReady       = 2, /**< USB interface is connected and ready */
		};

	/* Task Definitions: */
		TASK(USB_Audio_Task);

	/* Event Handlers: */
		/** Indicates that this module will catch the USB_Connect event when thrown by the library. */
		HANDLES_EVENT(USB_Connect);

		/** Indicates that this module will catch the USB_Disconnect event when thrown by the library. */
		HANDLES_EVENT(USB_Disconnect);

		/** Indicates that this module will catch the USB_ConfigurationChanged event when thrown by the library. */
		HANDLES_EVENT(USB_ConfigurationChanged);

		/** Indicates that this module will catch the USB_UnhandledControlPacket event when thrown by the library. */
		HANDLES_EVENT(USB_UnhandledControlPacket);
	
	/* Function Prototypes: */
		void UpdateStatus(uint8_t CurrentStatus);

#endif
