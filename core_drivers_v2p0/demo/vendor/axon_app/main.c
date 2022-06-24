/********************************************************************************************************
 * @file  app_config.h
 *
 * @brief This is the header file for B91
 *
 * @author  Driver Group
 * @date  2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
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
/**
 *          Copyright (c) 2020-2022, Atlazo Inc.
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
 */

#include <stdint.h>
#include "app_config.h"
#include "calibration.h"
#include <nds_intrinsic.h>
#include "printf.h"

extern void user_init();
extern void main_loop (void);


/**
 * @brief		This is main function
 * @param[in]	none
 * @return      none
 */
int main (void)
{
  uint32_t ticks_as_32 = 0;
  ticks_as_32 = __nds__csrr(NDS_MCYCLE);
  gpio_function_en(GPIO_LED_RED);
  gpio_set_output(GPIO_LED_RED, 1);
  gpio_set_high_level(GPIO_LED_RED);  //red light on while waiting
  while((__nds__csrr(NDS_MCYCLE)-ticks_as_32) < (24000000*10));
  gpio_set_low_level(GPIO_LED_RED);  //red light off

#if(MCU_CORE_B91)
  sys_init(LDO_1P4_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6);
  //Note: This function can improve the performance of some modules, which is described in the function comments.
  //Called immediately after sys_init, set in other positions, some calibration values may not take effect.
  user_read_flash_value_calib();
#elif(MCU_CORE_B92)
  sys_init();
#endif
  CCLK_24M_HCLK_24M_PCLK_24M;

  user_init();

  while (1) {
    main_loop ();
  }
  return 0;
}

