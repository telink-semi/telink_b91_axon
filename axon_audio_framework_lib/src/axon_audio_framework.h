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
#define LED1            GPIO_PB4
#define LED2            GPIO_PB5
#define LED3            GPIO_PB6
#define LED4            GPIO_PB7
#define LED5            GPIO_PD4
#define LED6            GPIO_PD5

#define HIGH            1
#define LOW             0

#define static_assert _Static_assert

#define FLD_CLK1_DMA_EN 	BIT(2)

#define UART_DMA  		    1     //uart use dma
#define UART_NDMA  	   		2     //uart not use dma

#define UART_MODE	 	    2

#define BASE_TX	   		0 //just for NDMA
#define NORMAL	   		1
#define USE_CTS    		2
#define USE_RTS    		3

#define FLOW_CTR  		NORMAL

#define UART0_RX_IRQ_LEN    16 //uart receive data length by irq

#define PLL_192M			PLL_CLK_192M
#define PLL_DIV_TO_CCLK		PAD_PLL_DIV
#define CCLK_48M      PLL_DIV4_TO_CCLK
#define CCLK_24M			PLL_DIV8_TO_CCLK
#define HCLK_24M			CCLK_DIV1_TO_HCLK
#define PCLK_24M			HCLK_DIV1_TO_PCLK
#define MSPI_CLK_24M		CCLK_TO_MSPI_CLK

// Audio defines
#define LINEIN_TO_LINEOUT               1
#define AMIC_TO_LINEOUT                 2
#define DMIC_TO_LINEOUT                 3
#define BUFFER_TO_LINEOUT               4
#define EXT_CODEC_LINEIN_LINEOUT        5

#define AUDIO_MODE     AMIC_TO_LINEOUT

// #define CAPTURE_AUDIO_PLAYBACK this should be defined by the build system

#define INPUT_STRIDE 1
#define AUDIO_SAMPLE_RATE_KHZ  16
#define AUDIO_SAMPLE_BIT_WIDTH 16

/*
 * Audio is processed in 32ms frames that are offset by 16ms.
 * Our strategy is the use a DMA buffer that is exactly 32ms long. This will get copied to
 * 16ms ping/pong buffers, and submitted to the model.
 */
#define RECORD_FRAME_DURATION_MS 32  // total record buffer is 32ms long.
#define RECORD_HALF_FRAME_DURATION_MS (RECORD_FRAME_DURATION_MS/2)  // A record frame is 16ms, half the buffer

/*
 * NOTE: DMA DEALS IN BYTES, NOT SAMPLES. WE'LL USE THE CONVENTION _LEN IS THE LENGTH IN UNITS OF STORAGE (ie,
 * what the buffer is pointing to), and _SIZE is in units of bytes.
 */
#define RECORD_FRAME_LEN (RECORD_FRAME_DURATION_MS * AUDIO_SAMPLE_RATE_KHZ)
#define RECORD_FRAME_SIZE (RECORD_FRAME_LEN * AUDIO_SAMPLE_BIT_WIDTH/8)
#define RECORD_HALF_FRAME_LEN (RECORD_FRAME_LEN>>1)
#define RECORD_HALF_FRAME_SIZE (RECORD_FRAME_SIZE>>1)

#if (TRIGGER_MODE_ALWAYS_ON)
/*
 * always on triggering. recording is enabled periodically to listen for volume. If volume is detected, recording
 * continues for 1s. Framework does its own valid BG/FG check.
 */
#  define PLAYBACK_BUFFER_LEN ((int)(1 * 1000 * AUDIO_SAMPLE_RATE_KHZ)) // length in samples of the playback buffer
#  define MAX_RECORDING_SEC 1                                             // maximum time recording will occur each time button is pressed
#elif (TRIGGER_MODE_BUTTON_PRESS)
/*
 * button press triggering. User presses the button, then releases it, then has 2s to speak a keyword.
 * BG/FG algorithm finds the 1s window in the 2s that has a valid volume profile (starts w/ silence, ends w/ silence, has at least 1 sustained
 * volume in between).
 */
#  define PLAYBACK_BUFFER_LEN ((int)(1.5 * 1000 * AUDIO_SAMPLE_RATE_KHZ)) // length in samples of the playback buffer
#  define MAX_RECORDING_SEC	2                                             // maximum time recording will occur each time button is pressed
#endif
#define MAX_HALF_FRAME_COUNT (MAX_RECORDING_SEC * 1000 / RECORD_HALF_FRAME_DURATION_MS)       // convert time to audio half frames

#define AUDIO_RX_DMA_CH DMA2
#define AUDIO_TX_DMA_CH DMA3


/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
