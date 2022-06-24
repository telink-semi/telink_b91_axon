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
#define UART0_RTX_PIN	UART0_RX_PB3
#define UART0_TX_PIN    UART0_TX_PB2
#define UART0_RX_PIN    UART0_RX_PB3
#define UART0_CTS_PIN   UART0_CTS_PA1
#define UART0_RTS_PIN   UART0_RTS_PA2

#define UART1_RTX_PIN	UART1_RX_PC7
#define UART1_TX_PIN    UART1_TX_PC6
#define UART1_RX_PIN    UART1_RX_PC7
#define UART1_CTS_PIN   UART1_CTS_PC4
#define UART1_RTS_PIN   UART1_RTS_PC5




#elif(MCU_CORE_B92)
#define LED1            		GPIO_PD0
#define LED2            		GPIO_PD1
#define LED3            		GPIO_PD2
#define LED4            		GPIO_PD3
#define LED5           		    GPIO_PD4
#define LED6            		GPIO_PD5
#define UART0_RTX_PIN 			GPIO_FC_PB3
#define UART0_TX_PIN            GPIO_FC_PB1
#define UART0_RX_PIN            GPIO_FC_PB2
#define UART0_CTS_PIN           GPIO_FC_PB4
#define UART0_RTS_PIN           GPIO_FC_PA0

#define UART1_RTX_PIN 			GPIO_FC_PB3
#define UART1_TX_PIN            GPIO_FC_PB1
#define UART1_RX_PIN            GPIO_FC_PB2
#define UART1_CTS_PIN           GPIO_FC_PB4
#define UART1_RTS_PIN           GPIO_FC_PA0

#endif

#define UART_DMA  		    1     //uart use dma
#define UART_NDMA  	   		2     //uart not use dma
#define UART_MODE	 	    UART_DMA

#define BASE_TX	   		0 //just for NDMA
#define NORMAL	   		1
#define USE_CTS    		2
#define USE_RTS    		3
#define FLOW_CTR  		NORMAL


#define UART_1WIRE_MODE       0  //tx and rx is a same line
#define UART_2WIRE_MODE       1  //tx and rx are two lines
#define UART_WIRE_MODE        UART_2WIRE_MODE


#define UART_RX_IRQ_LEN    16 //uart receive data length by irq







/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
