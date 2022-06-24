/********************************************************************************************************
 * @file	main.c
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
#include "calibration.h"
#include "gpio_default.h"
extern void user_init();
extern void main_loop (void);
volatile unsigned int gpio_irq_cnt=0,gpio_irq_rsic0_cnt=0,gpio_irq_rsic1_cnt=0;

#if(MCU_CORE_B92|MCU_CORE_B93)
volatile unsigned int gpio_src_irq0_cnt=0;
volatile unsigned int gpio_src_irq1_cnt=0;
volatile unsigned int gpio_src_irq2_cnt=0;
volatile unsigned int gpio_src_irq3_cnt=0;
volatile unsigned int gpio_src_irq4_cnt=0;
volatile unsigned int gpio_src_irq5_cnt=0;
volatile unsigned int gpio_src_irq6_cnt=0;
volatile unsigned int gpio_src_irq7_cnt=0;
#endif

/**
 * @brief		This function serves to handle the interrupt of MCU
 * @param[in] 	none
 * @return 		none
 */
#if (GPIO_MODE == GPIO_IRQ )
_attribute_ram_code_sec_noinline_ void gpio_irq_handler(void)
{
	gpio_toggle(LED2);
	gpio_irq_cnt++;
	gpio_clr_irq_status(FLD_GPIO_IRQ_CLR);
}
#elif(GPIO_MODE == GPIO_IRQ_RSIC0)
_attribute_ram_code_sec_noinline_ void gpio_risc0_irq_handler(void)
{
	gpio_toggle(LED3);
	gpio_irq_rsic0_cnt++;
	gpio_clr_irq_status(FLD_GPIO_IRQ_GPIO2RISC0_CLR);
}
#elif(GPIO_MODE == GPIO_IRQ_RSIC1)

_attribute_ram_code_sec_noinline_ void gpio_risc1_irq_handler(void)
{
	gpio_toggle(LED4);
	gpio_irq_rsic1_cnt++;
	gpio_clr_irq_status(FLD_GPIO_IRQ_GPIO2RISC1_CLR);
}
#endif
#if(MCU_CORE_B92|MCU_CORE_B93)
#if(GPIO_MODE == GPIO_SEL_IRQ_SRC)
_attribute_ram_code_sec_noinline_ void gpio_src0_irq_handler(void)
{
	gpio_src_irq0_cnt++;
	gpio_toggle(LED1);
 	gpio_clr_group_irq_status(FLD_GPIO_GROUP_IRQ0);
}
_attribute_ram_code_sec_noinline_ void gpio_src1_irq_handler(void)
{
	gpio_src_irq1_cnt++;
	gpio_clr_group_irq_status(FLD_GPIO_GROUP_IRQ1);
}
_attribute_ram_code_sec_noinline_ void gpio_src2_irq_handler(void)
{
	gpio_src_irq2_cnt++;
	gpio_clr_group_irq_status(FLD_GPIO_GROUP_IRQ2);
}
_attribute_ram_code_sec_noinline_ void gpio_src3_irq_handler(void)
{
	gpio_src_irq3_cnt++;
	gpio_clr_group_irq_status(FLD_GPIO_GROUP_IRQ3);
}
_attribute_ram_code_sec_noinline_ void gpio_src4_irq_handler(void)
{
	gpio_src_irq4_cnt++;
	gpio_clr_group_irq_status(FLD_GPIO_GROUP_IRQ4);
}
_attribute_ram_code_sec_noinline_ void gpio_src5_irq_handler(void)
{
	gpio_src_irq5_cnt++;
	gpio_clr_group_irq_status(FLD_GPIO_GROUP_IRQ5);
}
_attribute_ram_code_sec_noinline_ void gpio_src6_irq_handler(void)
{
	gpio_src_irq6_cnt++;
	gpio_clr_group_irq_status(FLD_GPIO_GROUP_IRQ6);
}
_attribute_ram_code_sec_noinline_ void gpio_src7_irq_handler(void)
{
	gpio_src_irq7_cnt++;
	gpio_clr_group_irq_status(FLD_GPIO_GROUP_IRQ7);
}
#endif
#endif

/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
int main (void)   //must on ramcode
{
#if(MCU_CORE_B91)
	sys_init(LDO_1P4_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6);
	//Note: This function can improve the performance of some modules, which is described in the function comments.
	//Called immediately after sys_init, set in other positions, some calibration values may not take effect.
	user_read_flash_value_calib();
	CCLK_24M_HCLK_24M_PCLK_24M;
#elif(MCU_CORE_B92)
	sys_init();
#elif(MCU_CORE_B93)
	sys_init();
#endif
	//gpio_init(1);
	user_init();
	while (1) {
		main_loop ();
	}
	return 0;
}
