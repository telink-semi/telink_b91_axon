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

#define LED1            GPIO_PB4
#define LED2            GPIO_PB5
#define LED3            GPIO_PB6
#define LED4            GPIO_PB7

#define ADC_DMA_MODE				1
#define ADC_NDMA_MODE				2

#define ADC_MODE 		ADC_NDMA_MODE

#define ADC_GPIO_SAMPLE			1	//GPIO voltage
/**
	* 	The Vbat channel battery voltage sample range is 1.8~3.5V and is low accuracy,
	* 	and must set sys_init with the mode for battery voltage less than 3.6V.
	* 	For accurate battery voltage sampling or battery voltage > 3.6V, should use gpio sampling with some external voltage divider.
	*	Recommended configuration parameters:
	*	--3/4 external resistor voltage divider(total resistance 400k, without any capacitance),
	*	--1.2V Vref,
	*	--1/4 Scale
	*	--Sampling frequence below 48K.
	*	add by chaofan.20201230.
**/
#define ADC_VBAT_SAMPLE			2	//Vbat channel Battery Voltage with low accuracy.
#define ADC_TEMP_SENSOR_SAMPLE	3	//Temp test

#define ADC_SAMPLE_MODE				ADC_GPIO_SAMPLE


/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
