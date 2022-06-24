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

void user_init()
{
	gpio_function_en(LED1);
	gpio_function_en(LED2);
	gpio_function_en(LED3);
	gpio_function_en(LED4);

	gpio_output_en(LED1);
	gpio_output_en(LED2);
	gpio_output_en(LED3);
	gpio_output_en(LED4);

	gpio_function_en(GPIO_PA2);
	gpio_output_en(GPIO_PA2);
	gpio_function_en(SW1);
	gpio_output_dis(SW1);
	gpio_input_en(SW1);
	gpio_set_up_down_res(SW1, GPIO_PIN_PULLUP_10K);
	core_interrupt_enable();

#if (GPIO_MODE == GPIO_IRQ )
	 /****GPIO_IRQ POL_FALLING   SW1 link PA2**/
    //if disable gpio interrupt,choose disable gpio mask , use interface gpio_clr_irq_mask instead of gpio_irq_dis,if use gpio_irq_dis,may generate a false interrupt.
	gpio_set_irq(SW1, INTR_RISING_EDGE);
	plic_interrupt_enable(IRQ25_GPIO);
#elif(GPIO_MODE == GPIO_IRQ_RSIC0)
	/****GPIO_IRQ_RSIC0  POL_FALLING   SW1 link PA2**/
	//if disable gpio2risc0 interrupt,choose disable gpio2risc0 mask , use interface gpio_clr_irq_mask instead of gpio_gpio2risc0_irq_dis,if use gpio_gpio2risc0_irq_dis,may generate a false interrupt..
	gpio_set_gpio2risc0_irq(SW1,INTR_FALLING_EDGE);
	plic_interrupt_enable(IRQ26_GPIO2RISC0);
#elif(GPIO_MODE == GPIO_IRQ_RSIC1)
	/****GPIO_IRQ_RSIC1  POL_FALLING   SW1 link PA2**/
	//if disable gpio2risc1 interrupt,choose disable gpio2risc1 mask , use interface gpio_clr_irq_mask instead of gpio_gpio2risc1_irq_dis,if use gpio_gpio2risc1_irq_dis,may generate a false interrupt.
	gpio_set_gpio2risc1_irq(SW1,INTR_FALLING_EDGE);
	plic_interrupt_enable(IRQ27_GPIO2RISC1);
#elif(GPIO_MODE == GPIO_HIGH_RESISTOR)
	gpio_shutdown(GPIO_ALL);//set all gpio as high resistor except sws and mspi
#endif
#if(MCU_CORE_B92|MCU_CORE_B93)
#if(GPIO_MODE == GPIO_SEL_IRQ_SRC)
	/*Note : the use method of 8 new GPIO irq source :
	 * First ,you can choose a gpio group,like gpio GPIO_GROUP_A,8 GPIO in all.
	 * Second,the 8 gpio you choose corresponding to the 8 gpio irq source respectively.for example,irq source0-GPIO_PA0,irq source1-GPIO-PA1......
	 * Attention:Once you choose a gpio-group,you can only use the gpio as irq_source in this group.And you should obey the rule of correspondence.
	 */
	gpio_set_src_irq_group(GPIO_GROUP_A);
	gpio_set_src_irq(SW1,INTR_RISING_EDGE);
	plic_interrupt_enable(IRQ34_GPIO_SRC0);
#endif
#endif
}
/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
void main_loop (void)
{
	delay_ms(200);
	gpio_toggle(GPIO_PA2);
	delay_ms(200);
}




