/********************************************************************************************************
 * @file	app_config.h
 *
 * @brief	This is the header file for B91
 *
 * @author	Driver Group
 * @date	2019
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

#pragma once
#include "driver.h"
/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#define  NPE_BASE_ADDR               0x112000


#define GPIO_LED_WHITE    GPIO_PB6
#define GPIO_LED_GREEN    GPIO_PB5
#define GPIO_LED_BLUE     GPIO_PB4
#define GPIO_LED_RED      GPIO_PB7
#define LED_ON_LEVAL      1     //gpio output high voltage to turn on led

#define PB7_FUNC        AS_GPIO
#define PB6_FUNC        AS_GPIO
#define PB5_FUNC        AS_GPIO
#define PB4_FUNC        AS_GPIO

#define PB7_OUTPUT_ENABLE   1
#define PB6_OUTPUT_ENABLE   1
#define PB5_OUTPUT_ENABLE   1
#define PB4_OUTPUT_ENABLE   1


/*
 * configure PE[7:4] as JTAG interface
 * note: the "AS_Txx" doesn't really matter as long as it isn't "AS_GPIO"
 */
#define PE4_FUNC  AS_TDI
#define PE4_INPUT_ENABLE  1
#define PE4_OUTPUT_ENABLE  1

#define PE5_FUNC  AS_TDO
#define PE5_INPUT_ENABLE  1
#define PE5_OUTPUT_ENABLE  1

#define PE6_FUNC  AS_TMS
#define PE6_INPUT_ENABLE  1
#define PE6_OUTPUT_ENABLE  1

#define PE7_FUNC  AS_TCK
#define PE7_INPUT_ENABLE  1
#define PE7_OUTPUT_ENABLE  1




/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
