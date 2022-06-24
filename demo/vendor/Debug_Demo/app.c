/********************************************************************************************************
 * @file	app.c
 *
 * @brief	This is the source file for B91m
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
#include "printf.h"
#include "app_config.h"
#if(MCU_CORE_B91)
#define LED1            GPIO_PB4
#define LED2            GPIO_PB5
#define LED3            GPIO_PB6
#define LED4            GPIO_PB7
#elif(MCU_CORE_B92)

#elif(MCU_CORE_B93)
#define LED1            GPIO_PD3
#define LED2            GPIO_PD2
#define LED3            GPIO_PD1
#define LED4            GPIO_PD5
#define LED5            GPIO_PD0
#define LED6            GPIO_PD4
#endif
void user_init()
{
	gpio_function_en(LED1|LED2|LED3|LED4);
	gpio_output_en(LED1|LED2|LED3|LED4);
	gpio_input_dis(LED1|LED2|LED3|LED4);
#if(DEBUG_BUS==USB_PRINT_DEBUG_ENABLE)
	usb_set_pin_en();
	usbhw_set_printer_threshold(1);
	usbhw_set_ep8_fifo_mode();
	delay_ms(1000);
	printf("\n Driver version: %2x \n Copyright (c) %d Telink semiconductor (%s) Ltd, Co \n",0xa001,2019,"shanghai");
#elif(DEBUG_BUS==UART_PRINT_DEBUG_ENABLE)
	printf("  \n");								// caution: The first byte will be error
	printf("\n Driver version: %2x \n",0xa001);
	printf("\n Copyright (c) %d \n",2020);
	printf("\n Telink semiconductor (%s)Ltd, Co \n","shanghai");

#endif

}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
void main_loop (void)
{
	printf(" Hello world! \n");
	gpio_toggle(LED1|LED2|LED3|LED4);
	delay_ms(50);
}




