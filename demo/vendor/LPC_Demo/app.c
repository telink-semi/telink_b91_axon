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
#include "app_config.h"

#define OUTPUT_PIN   GPIO_PB7

unsigned char result = 0;


void user_init(void)
{
	gpio_function_en(LED1|LED2|LED3|LED4);
	gpio_output_en(LED1|LED2|LED3|LED4);
	gpio_input_dis(LED1|LED2|LED3|LED4);
	gpio_set_high_level(OUTPUT_PIN);

	lpc_power_on();
	lpc_set_input_chn(LPC_INPUT_PB1);
	lpc_set_input_ref(LPC_NORMAL,LPC_REF_872MV);
	lpc_set_scaling_coeff(LPC_SCALING_PER50);
}


void main_loop(void)
{
	result = lpc_get_result();

	if(result)
	{
		gpio_set_high_level(OUTPUT_PIN);
	}
	else
	{
		gpio_set_low_level(OUTPUT_PIN);
	}

	gpio_toggle(LED1|LED3);
	delay_ms(200);
}


