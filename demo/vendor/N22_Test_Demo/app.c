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


/****  This demo is for N22 only and needs to be used with the main core D25f
 *
 * Notices: 1.N22 and D25 cannot write the same register at the same time
 *          2.N22 and D25 can read the same register at the same time
 *  ***/



void user_init()
{
	gpio_function_en(LED4);
	gpio_function_en(LED5);
	gpio_function_en(LED6);


	gpio_output_en(LED4);
	gpio_output_en(LED5);
	gpio_output_en(LED6);
}


volatile unsigned int loop_cnt=1;
/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
void main_loop (void)
{

	gpio_toggle(LED6|LED5|LED4);//D2F cannot go to toggle at the same time
	delay_ms(100);//can read with D25F at the same time
	loop_cnt++;

}




