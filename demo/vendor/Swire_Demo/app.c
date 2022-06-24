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

/**********************************************************************************************************************
*                                         swire device selection                                                	 *
*********************************************************************************************************************/
#define SWIRE_MASTER_DEVICE		       		  1
#define SWIRE_SLAVE_DEVICE		      		  2
#define SWIRE_DEVICE				   		  SWIRE_MASTER_DEVICE


#define DATA_LEN			6
#define ADDRESS_LEN			4			//B91s is four bytes, B85s is three bytes.
#define ADDRESS_VALUE		0xc0000000
#define SLAVE_ID			0           //The default id of an empty chip is 0
#define SWIRE_SPEED			1000000


unsigned char wbuf[DATA_LEN] = {};
unsigned char rbuf[DATA_LEN] = {};
void user_init()
{
	gpio_function_en(LED1|LED2|LED3|LED4);
	gpio_output_en(LED1|LED2|LED3|LED4);
	gpio_input_dis(LED1|LED2|LED3|LED4);
	delay_ms(3000);
#if (SWIRE_DEVICE == SWIRE_MASTER_DEVICE)
	unsigned int i;
	for( i = 0; i < DATA_LEN; i++){
		wbuf[i]= i;
	}

	swire_set_swm_en(GPIO_SWM);

	//Swire communication needs to be connected to a pull-up resistor
	//if there is no external pull-up, the software configuration of the pull-up resistor is required
	gpio_set_up_down_res(GPIO_SWM, GPIO_PIN_PULLUP_10K);
	swire_set_clk(SWIRE_SPEED);
#else
	swire_set_sws_en();
	swire_set_slave_id(SLAVE_ID);
#endif
}
void main_loop (void)
{
#if (SWIRE_DEVICE == SWIRE_MASTER_DEVICE)
	unsigned char i;
	unsigned int addr = ADDRESS_VALUE;
	swire_master_write(SLAVE_ID,(unsigned char*)(&addr),ADDRESS_LEN,wbuf,DATA_LEN);
	unsigned int read_result = swire_master_read(SLAVE_ID,(unsigned char*)(&addr),ADDRESS_LEN,rbuf,DATA_LEN);
	if(read_result == 0){
		//read timeout
		gpio_set_high_level(LED4);
	}else{
		for( i = 0; i < DATA_LEN; i++){
			if(wbuf[i] != rbuf[i]){
				break;
				gpio_set_high_level(LED3);
			}
		}
		if(i<DATA_LEN){
			//read error
			gpio_set_high_level(LED4);
		}else{
			//read ok
			gpio_set_high_level(LED1);
		}
	}
#if(MCU_CORE_B91)
	//If you keep reading and writing in the mainloop and use the BDT to access the chip through sws at the same time,
	//sometimes there will be BDT access errors (the reason is: there is a problem with the BMC of the B91 chip, so there will be such a phenomenon)
	//In case there is a bus conflict, after the test, the program stops here
	gpio_input_dis(GPIO_SWM);
	gpio_output_dis(GPIO_SWM);
	gpio_function_en(GPIO_SWM);
	gpio_set_low_level(LED2);
	while(1);
#endif

#else
	gpio_toggle(LED1|LED2|LED3|LED4);
	delay_ms(1000);
#endif
}











