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
 
/** \ingroup Group_USB
 *  @defgroup Group_Device Device Management
 *
 *  USB Device mode related macros and enums. This module contains macros and enums which are used when
 *  the USB controller is initialized in device mode.
 *
 *  @{
 */

#ifndef __USBDEVICE_H__
#define __USBDEVICE_H__

	/* Includes: */
		#include <avr/pgmspace.h>
		#include <avr/eeprom.h>

		#include "../../../Common/Common.h"	
		#include "../HighLevel/StdDescriptors.h"
		#include "Endpoint.h"

	/* Public Interface - May be used in end-application: */
		/* Macros: */
			#if defined(USB_FULL_CONTROLLER) || defined(USB_MODIFIED_FULL_CONTROLLER) || defined(__DOXYGEN__)
				/** Mask for the Options parameter of the \ref USB_Init() function. This indicates that the
				 *  USB interface should be initialized in low speed (1.5Mb/s) mode.
				 *
				 *  \note Low Speed mode is not available on all USB AVR models.
				 *
				 *  \note Restrictions apply on the number, size and type of endpoints which can be used
				 *        when running in low speed mode -- refer to the USB 2.0 standard.
				 */
				#define USB_DEVICE_OPT_LOWSPEED            (1 << 0)
			#endif
			
			/** Mask for the Options parameter of the USB_Init() function. This indicates that the
			 *  USB interface should be initialized in full speed (12Mb/s) mode.
			 */
			#define USB_DEVICE_OPT_FULLSPEED               (0 << 0)
			
		/* Pseudo-Function Macros: */
			#if defined(__DOXYGEN__)
				/** Sends a Remote Wakeup request to the host. This signals to the host that the device should
				 *  be taken out of suspended mode, and communications should resume.
				 *
				 *  Typically, this is implemented so that HID devices (mice, keyboards, etc.) can wake up the
				 *  host computer when the host has suspended all USB devices to enter a low power state.
				 *
				 *  \note This macro should only be used if the device has indicated to the host that it
				 *        supports the Remote Wakeup feature in the device descriptors, and should only be
				 *        issued if the host is currently allowing remote wakeup events from the device (i.e.,
				 *        the \ref USB_RemoteWakeupEnabled flag is set).
				 *
				 *  \see \ref Group_Descriptors for more information on the RMWAKEUP feature and device descriptors.
				 */
				static inline void USB_Device_SendRemoteWakeup(void);
				
				/** Indicates if a Remote Wakeup request is being sent to the host. This returns true if a
				 *  remote wakeup is currently being sent, false otherwise.
				 *
				 *  This can be used in conjunction with the \ref USB_Device_IsUSBSuspended() macro to determine if
				 *  a sent RMWAKEUP request was accepted or rejected by the host.
				 *
				 *  \note This macro should only be used if the device has indicated to the host that it
				 *        supports the Remote Wakeup feature in the device descriptors.
				 *
				 *  \see \ref Group_Descriptors for more information on the RMWAKEUP feature and device descriptors.
				 *
				 *  \return Boolean true if no Remote Wakeup request is currently being sent, false otherwise
				 */
				static inline bool USB_Device_IsRemoteWakeupSent(void);
				
				/** Indicates if the device is currently suspended by the host. Whilst suspended, the device is
				 *  to enter a low power state until resumed by the host. No USB traffic to or from the device
				 *  can occur (except for Remote Wakeup requests) during suspension by the host.
				 *
				 *  \return Boolean true if the USB communications have been suspended by the host, false otherwise.
				 */
				static inline bool USB_Device_IsUSBSuspended(void);
			#else
				#define USB_Device_SendRemoteWakeup()   MACROS{ UDCON |= (1 << RMWKUP); }MACROE

				#define USB_Device_IsRemoteWakeupSent()       ((UDCON &  (1 << RMWKUP)) ? false : true)

				#define USB_Device_IsUSBSuspended()           ((UDINT &  (1 << SUSPI)) ? true : false)
			#endif

		/* Enums: */
			/** Enum for the ErrorCode parameter of the \ref USB_DeviceError event.
			 *
			 *  \see Events.h for more information on this event.
			 */
			enum USB_Device_ErrorCodes_t
			{
				DEVICE_ERROR_GetDescriptorNotHooked        = 0, /**< Indicates that the \ref USB_GetDescriptor() method
				                                                 *   has not been hooked by the user application.
				                                                 *
				                                                 *   \see \ref Group_Descriptors for more information on
				                                                 *        the \ref USB_GetDescriptor() method.
				                                                 */
			};

	/* Private Interface - For use in library only: */
	#if !defined(__DOXYGEN__)
		/* Macros: */		
			#define USB_Device_SetLowSpeed()        MACROS{ UDCON |=  (1 << LSM);   }MACROE
			#define USB_Device_SetHighSpeed()       MACROS{ UDCON &= ~(1 << LSM);   }MACROE
	#endif

#endif

/** @} */
