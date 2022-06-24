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
#if TEST_DEMO == BQB_DEMO
#include "BQB/bqb.h"
#endif

extern void user_init(void);
extern void main_loop (void);
#if TEST_DEMO==BQB_DEMO && SUPPORT_CONFIGURATION
extern void rd_usr_definition(unsigned char _s);
#endif

/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
int main(void)
{
#if TEST_DEMO==BQB_DEMO
	#if SUPPORT_CONFIGURATION
		rd_usr_definition(1);
		if(usr_config.power_mode == 0)
		{
			sys_init(LDO_1P4_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6);
		}
		else
		{
			sys_init(DCDC_1P4_DCDC_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6);
		}
	#else
		#if SWITCH_POWER_MODE
			sys_init(LDO_1P4_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6);
		#else
			sys_init(DCDC_1P4_DCDC_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6);
		#endif
	#endif
#else
	sys_init(LDO_1P4_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6);
#endif
	//Note: This function can improve the performance of some modules, which is described in the function comments.
	//Called immediately after sys_init, set in other positions, some calibration values may not take effect.
	user_read_flash_value_calib();
	CCLK_24M_HCLK_24M_PCLK_24M;

    user_init();

    while(1)
    {
    	main_loop();
    }
    return 0;
}


