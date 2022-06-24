/********************************************************************************************************
 * @file	cdc_app.c
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
#if(USB_DEMO_TYPE==USB_CDC)
#include "application/usb_app/usbcdc.h"
#include "application/usbstd/usb.h"

extern unsigned char usb_cdc_data[CDC_TXRX_EPSIZE];
extern unsigned short usb_cdc_data_len;
void user_init(void)
{
	//1.enable USB DP pull up 1.5k
	usb_set_pin_en();
	//2.enable USB manual interrupt(in auto interrupt mode,USB device would be USB printer device)
	usb_init_interrupt();
	//3.set ep5 BUSY(BUSY same as ACK) bit as 1 means output endpoint buffer can receive data from USB host
	usbhw_data_ep_ack(USB_EDP_CDC_OUT);
	//4.enable global interrupt
	core_interrupt_enable();
#if(MCU_CORE_B91)
	usbhw_set_irq_mask(USB_IRQ_RESET_MASK|USB_IRQ_SUSPEND_MASK);
#endif
}

void main_loop (void)
{
	usb_handle_irq();

	if(usb_cdc_data_len!=0)
			{
			usb_cdc_tx_data_to_host(usb_cdc_data,usb_cdc_data_len);
			usb_cdc_data_len = 0;
		}

}

#endif

