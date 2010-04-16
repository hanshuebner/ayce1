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
 *  Main source file for the RNDISEthernet demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#include "RNDISEthernet.h"

/* Scheduler Task List */
TASK_LIST
{
	{ .Task = USB_USBTask          , .TaskStatus = TASK_STOP },
	{ .Task = Ethernet_Task        , .TaskStatus = TASK_STOP },
	{ .Task = TCP_Task             , .TaskStatus = TASK_STOP },
	{ .Task = RNDIS_Task           , .TaskStatus = TASK_STOP },
};

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
	LEDs_Init();
	SerialStream_Init(9600, false);
	
	/* Webserver Initialization */
	TCP_Init();
	Webserver_Init();

	printf_P(PSTR("\r\n\r\n****** RNDIS Demo running. ******\r\n"));

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
 *  the status LEDs and stops all the relevant tasks.
 */
EVENT_HANDLER(USB_Disconnect)
{
	/* Stop running TCP/IP and USB management tasks */
	Scheduler_SetTaskMode(RNDIS_Task, TASK_STOP);
	Scheduler_SetTaskMode(Ethernet_Task, TASK_STOP);
	Scheduler_SetTaskMode(TCP_Task, TASK_STOP);
	Scheduler_SetTaskMode(USB_USBTask, TASK_STOP);

	/* Indicate USB not ready */
	UpdateStatus(Status_USBNotReady);
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host sets the current configuration
 *  of the USB device after enumeration, and configures the RNDIS device endpoints and starts the relevant tasks.
 */
EVENT_HANDLER(USB_ConfigurationChanged)
{
	/* Setup CDC Notification, Rx and Tx Endpoints */
	Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPNUM, EP_TYPE_INTERRUPT,
		                       ENDPOINT_DIR_IN, CDC_NOTIFICATION_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	Endpoint_ConfigureEndpoint(CDC_TX_EPNUM, EP_TYPE_BULK,
		                       ENDPOINT_DIR_IN, CDC_TXRX_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	Endpoint_ConfigureEndpoint(CDC_RX_EPNUM, EP_TYPE_BULK,
		                       ENDPOINT_DIR_OUT, CDC_TXRX_EPSIZE,
	                           ENDPOINT_BANK_SINGLE);

	/* Indicate USB connected and ready */
	UpdateStatus(Status_USBReady);

	/* Start TCP/IP tasks */
	Scheduler_SetTaskMode(RNDIS_Task, TASK_RUN);
	Scheduler_SetTaskMode(Ethernet_Task, TASK_RUN);
	Scheduler_SetTaskMode(TCP_Task, TASK_RUN);
}

/** Event handler for the USB_UnhandledControlPacket event. This is used to catch standard and class specific
 *  control requests that are not handled internally by the USB library (including the RNDIS control commands,
 *  which set up the USB RNDIS network adapter), so that they can be handled appropriately for the application.
 */
EVENT_HANDLER(USB_UnhandledControlPacket)
{
	/* Discard the unused wValue parameter */
	Endpoint_Discard_Word();

	/* Discard the unused wIndex parameter */
	Endpoint_Discard_Word();

	/* Read in the wLength parameter */
	uint16_t wLength = Endpoint_Read_Word_LE();

	/* Process RNDIS class commands */
	switch (USB_ControlRequest.bRequest)
	{
		case REQ_SendEncapsulatedCommand:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				/* Clear the SETUP packet, ready for data transfer */
				Endpoint_ClearSETUP();
				
				/* Read in the RNDIS message into the message buffer */
				Endpoint_Read_Control_Stream_LE(RNDISMessageBuffer, wLength);

				/* Finalize the stream transfer to clear the last packet from the host */
				Endpoint_ClearIN();

				/* Process the RNDIS message */
				ProcessRNDISControlMessage();
			}
			
			break;
		case REQ_GetEncapsulatedResponse:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				/* Clear the SETUP packet, ready for data transfer */
				Endpoint_ClearSETUP();
				
				/* Check if a response to the last message is ready */
				if (!(MessageHeader->MessageLength))
				{
					/* Set the response to a single 0x00 byte to indicate that no response is ready */
					RNDISMessageBuffer[0] = 0;
					MessageHeader->MessageLength = 1;
				}

				/* Write the message response data to the endpoint */
				Endpoint_Write_Control_Stream_LE(RNDISMessageBuffer, MessageHeader->MessageLength);
				
				/* Finalize the stream transfer to send the last packet or clear the host abort */
				Endpoint_ClearOUT();

				/* Reset the message header once again after transmission */
				MessageHeader->MessageLength = 0;
			}
	
			break;
	}
}

/** Function to manage status updates to the user. This is done via LEDs on the given board, if available, but may be changed to
 *  log to a serial port, or anything else that is suitable for status updates.
 *
 *  \param CurrentStatus  Current status of the system, from the RNDISEthernet_StatusCodes_t enum
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
		case Status_ProcessingEthernetFrame:
			LEDMask = (LEDS_LED2 | LEDS_LED3);
			break;		
	}
	
	/* Set the board LEDs to the new LED mask */
	LEDs_SetAllLEDs(LEDMask);
}

/** Task to manage the sending and receiving of encapsulated RNDIS data and notifications. This removes the RNDIS
 *  wrapper from received Ethernet frames and places them in the FrameIN global buffer, or adds the RNDIS wrapper
 *  to a frame in the FrameOUT global before sending the buffer contents to the host.
 */
TASK(RNDIS_Task)
{
	/* Select the notification endpoint */
	Endpoint_SelectEndpoint(CDC_NOTIFICATION_EPNUM);

	/* Check if a message response is ready for the host */
	if (Endpoint_IsINReady() && ResponseReady)
	{
		USB_Notification_t Notification = (USB_Notification_t)
			{
				.bmRequestType = (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE),
				.bNotification = NOTIF_RESPONSE_AVAILABLE,
				.wValue        = 0,
				.wIndex        = 0,
				.wLength       = 0,
			};
		
		/* Indicate that a message response is ready for the host */
		Endpoint_Write_Stream_LE(&Notification, sizeof(Notification));

		/* Finalize the stream transfer to send the last packet */
		Endpoint_ClearIN();

		/* Indicate a response is no longer ready */
		ResponseReady = false;
	}
	
	/* Don't process the data endpoints until the system is in the data initialized state, and the buffer is free */
	if ((CurrRNDISState == RNDIS_Data_Initialized) && !(MessageHeader->MessageLength))
	{
		/* Create a new packet header for reading/writing */
		RNDIS_PACKET_MSG_t RNDISPacketHeader;

		/* Select the data OUT endpoint */
		Endpoint_SelectEndpoint(CDC_RX_EPNUM);
		
		/* Check if the data OUT endpoint contains data, and that the IN buffer is empty */
		if (Endpoint_IsOUTReceived() && !(FrameIN.FrameInBuffer))
		{
			/* Read in the packet message header */
			Endpoint_Read_Stream_LE(&RNDISPacketHeader, sizeof(RNDIS_PACKET_MSG_t));

			/* Stall the request if the data is too large */
			if (RNDISPacketHeader.DataLength > ETHERNET_FRAME_SIZE_MAX)
			{
				Endpoint_StallTransaction();
				return;
			}
			
			/* Read in the Ethernet frame into the buffer */
			Endpoint_Read_Stream_LE(FrameIN.FrameData, RNDISPacketHeader.DataLength);

			/* Finalize the stream transfer to send the last packet */
			Endpoint_ClearOUT();
			
			/* Store the size of the Ethernet frame */
			FrameIN.FrameLength = RNDISPacketHeader.DataLength;

			/* Indicate Ethernet IN buffer full */
			FrameIN.FrameInBuffer = true;
		}
		
		/* Select the data IN endpoint */
		Endpoint_SelectEndpoint(CDC_TX_EPNUM);
		
		/* Check if the data IN endpoint is ready for more data, and that the IN buffer is full */
		if (Endpoint_IsINReady() && FrameOUT.FrameInBuffer)
		{
			/* Clear the packet header with all 0s so that the relevant fields can be filled */
			memset(&RNDISPacketHeader, 0, sizeof(RNDIS_PACKET_MSG_t));

			/* Construct the required packet header fields in the buffer */
			RNDISPacketHeader.MessageType   = REMOTE_NDIS_PACKET_MSG;
			RNDISPacketHeader.MessageLength = (sizeof(RNDIS_PACKET_MSG_t) + FrameOUT.FrameLength);
			RNDISPacketHeader.DataOffset    = (sizeof(RNDIS_PACKET_MSG_t) - sizeof(RNDIS_Message_Header_t));
			RNDISPacketHeader.DataLength    = FrameOUT.FrameLength;

			/* Send the packet header to the host */
			Endpoint_Write_Stream_LE(&RNDISPacketHeader, sizeof(RNDIS_PACKET_MSG_t));

			/* Send the Ethernet frame data to the host */
			Endpoint_Write_Stream_LE(FrameOUT.FrameData, RNDISPacketHeader.DataLength);
			
			/* Finalize the stream transfer to send the last packet */
			Endpoint_ClearIN();
			
			/* Indicate Ethernet OUT buffer no longer full */
			FrameOUT.FrameInBuffer = false;
		}
	}
}

/** Ethernet frame processing task. This task checks to see if a frame has been received, and if so hands off the processing
 *  of the frame to the Ethernet processing routines.
 */
TASK(Ethernet_Task)
{
	/* Task for Ethernet processing. Incoming ethernet frames are loaded into the FrameIN structure, and
	   outgoing frames should be loaded into the FrameOUT structure. Both structures can only hold a single
	   Ethernet frame at a time, so the FrameInBuffer bool is used to indicate when the buffers contain data. */

	/* Check if a frame has been written to the IN frame buffer */
	if (FrameIN.FrameInBuffer)
	{
		/* Indicate packet processing started */
		UpdateStatus(Status_ProcessingEthernetFrame);

		/* Process the ethernet frame - replace this with your own Ethernet handler code as desired */
		Ethernet_ProcessPacket();

		/* Indicate packet processing complete */
		UpdateStatus(Status_USBReady);
	}
}
