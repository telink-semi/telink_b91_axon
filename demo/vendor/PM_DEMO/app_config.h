/********************************************************************************************************
 * @file	app_config.h
 *
 * @brief	This is the header file for B91m
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
#pragma once

#include "driver.h"

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif

#define LED1            		GPIO_PB4
#define LED2            		GPIO_PB5
#define LED3            		GPIO_PB6
#define LED4            		GPIO_PB7

#define WAKEUP_PAD				GPIO_PA0
#define CURRENT_TEST	     	1
#define CRC_OK			     	1
#define	MDEC_MATCH_VALUE		0x02

/* IDLE MODE */
#define IDLE_TIMER_WAKEUP				1

/* SUSPEND MODE */
//The A0 version of the suspend execution process is abnormal and the program restarts.
#define SUSPEND_PAD_WAKEUP   			2	//62.1uA
#define SUSPEND_32K_RC_WAKEUP   		3	//63.7uA
#define SUSPEND_32K_XTAL_WAKEUP			4	//62.7uA
#define SUSPEND_MDEC_WAKEUP				5	//65.2uA
#define SUSPEND_COMPARATOR_WAKEUP      	6	//62.3uA
#define SUSPEND_CORE_WAKEUP      		7	//

/* DEEPSLEEP MODE */
#define DEEP_PAD_WAKEUP		 			8	//0.7uA
#define DEEP_32K_RC_WAKEUP      		9	//1.3uA
#define DEEP_32K_XTAL_WAKEUP      		10	//1.7uA
#define DEEP_MDEC_WAKEUP      			11	//1.4uA
#define DEEP_COMPARATOR_WAKEUP      	12	//1.6uA

/* DEEPSLEEP WITH RETENTION MODE */
#define DEEP_RET32K_PAD_WAKEUP     		14	//1.8uA
#define DEEP_RET32K_32K_RC_WAKEUP     	15	//2.4uA
#define DEEP_RET32K_32K_XTAL_WAKEUP     16	//2.8uA
#define DEEP_RET32K_MDEC_WAKEUP      	17	//2.6uA
#define DEEP_RET32K_COMPARATOR_WAKEUP   18	//2.8uA

#define DEEP_RET64K_PAD_WAKEUP     		20	//2.7uA
#define DEEP_RET64K_32K_RC_WAKEUP     	21	//3.2uA
#define DEEP_RET64K_32K_XTAL_WAKEUP     22	//3.7uA
#define DEEP_RET64K_MDEC_WAKEUP      	23	//3.4uA
#define DEEP_RET64K_COMPARATOR_WAKEUP   24	//3.7uA

#define PM_MODE			     			DEEP_PAD_WAKEUP


/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

