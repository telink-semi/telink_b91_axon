/********************************************************************************************************
 * @file	mouse_app.c
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
#include "app_config.h"
#if(USB_DEMO_TYPE==USB_MOUSE)
#include "application/usbstd/usb.h"
#include "application/usb_app/usbmouse.h"

char  mouse[4];

void user_init(void)
{
	//1.enable USB DP pull up 1.5k
	usb_set_pin_en();
	//2.enable USB manual interrupt(in auto interrupt mode,USB device would be USB printer device)
	usb_init_interrupt();
	//3.enable global interrupt
	core_interrupt_enable();
#if(MCU_CORE_B91)
	usbhw_set_irq_mask(USB_IRQ_RESET_MASK|USB_IRQ_SUSPEND_MASK);
#endif

	//initiate LED for indication
	gpio_function_en(LED1|LED2|LED3|LED4);
	gpio_input_dis(LED1|LED2|LED3|LED4);
	gpio_output_en(LED1|LED2|LED3|LED4);
	gpio_set_high_level(LED1|LED2|LED3|LED4);
	delay_us(100000);
	gpio_set_low_level(LED1|LED2|LED3|LED4);

	//initiate Button for Mouse input
	gpio_function_en(GPIO_PC1);
	gpio_input_en(GPIO_PC1);
	gpio_output_dis(GPIO_PC1);
	gpio_set_up_down_res(GPIO_PC1, GPIO_PIN_PULLUP_10K);

	gpio_function_en(GPIO_PC2);
	gpio_input_en(GPIO_PC2);
	gpio_output_dis(GPIO_PC2);
	gpio_set_up_down_res(GPIO_PC2, GPIO_PIN_PULLUP_10K);
}

/* enum to USB input device and simulate the left click and right click of mouse */
void main_loop (void)
{
	usb_handle_irq();
	if(g_usb_config != 0 )
	{
		if(gpio_get_level(GPIO_PC1)==0)
		{
			delay_us(10000);
			if(gpio_get_level(GPIO_PC1)==0)
			{
				while(gpio_get_level(GPIO_PC1)==0);
				gpio_set_high_level(LED1);
				//printf("Key:Mouse  Click ! \r\n");
				mouse[0] = BIT(1);// BIT(0) - left key; BIT(1) - right key; BIT(2) - middle key; BIT(3) - side key; BIT(4) - external key
				mouse[1] = -2;	  // Displacement relative to x coordinate
				mouse[2] = 2;	  // Displacement relative to y coordinate
				mouse[3] = 0;     // Displacement relative to the roller
				usbmouse_hid_report(USB_HID_MOUSE,(unsigned char*)mouse,4);
			}
		}

		if(gpio_get_level(GPIO_PC2)==0)
		{
			delay_us(10000);
			if(gpio_get_level(GPIO_PC2)==0)
			{
				while(gpio_get_level(GPIO_PC2)==0);
				gpio_set_low_level(LED1);
				//printf("Key:release \r\n");
				mouse[0] = 0;
				mouse[1] = 0;
				mouse[2] = 0;
				mouse[3] = 0;
				usbmouse_hid_report(USB_HID_MOUSE,(unsigned char*)mouse,4);
			}
		}
	}
}

#endif
