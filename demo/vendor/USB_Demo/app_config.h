/********************************************************************************************************
 * @file	app_config.h
 *
 * @brief	This is the header file for B91m
 *
 * @author	Driver Group
 * @date	2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#pragma once
#include "driver.h"

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#if(MCU_CORE_B91)
#define LED1            GPIO_PB4
#define LED2            GPIO_PB5
#define LED3            GPIO_PB6
#define LED4            GPIO_PB7
#elif(MCU_CORE_B92)
#define LED1            GPIO_PD0
#define LED2            GPIO_PD1
#define LED3            GPIO_PD2
#define LED4            GPIO_PD3

#define AISO              0
#define INT               1


#define USB_MODE          INT
#endif

#define	 USB_MOUSE          1
#define	 USB_KEYBOARD		2
#define	 USB_MICROPHONE		3
#define	 USB_SPEAKER		4
#define	 USB_CDC	        5
#define	 USB_MIC_SPEAKER    6

#define	 USB_DEMO_TYPE		USB_MOUSE



#if	(USB_DEMO_TYPE == USB_MOUSE)
	#define	USB_MOUSE_ENABLE 		1
#elif (USB_DEMO_TYPE == USB_KEYBOARD)
	#define	USB_KEYBOARD_ENABLE 	1
#elif (USB_DEMO_TYPE == USB_MICROPHONE)
	#define	  USB_MIC_ENABLE 		1
#elif (USB_DEMO_TYPE == USB_SPEAKER)
	#define	USB_SPEAKER_ENABLE 		1
#elif (USB_DEMO_TYPE == USB_CDC)
	#define USB_CDC_ENABLE          1
#elif (USB_DEMO_TYPE == USB_MIC_SPEAKER)
	#define	USB_MIC_ENABLE 			1
	#define	USB_SPEAKER_ENABLE 		1
#endif





#define CHIP_VER_A0              0
#define CHIP_VER_A1              1
#define CHIP_VER             CHIP_VER_A1










/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
