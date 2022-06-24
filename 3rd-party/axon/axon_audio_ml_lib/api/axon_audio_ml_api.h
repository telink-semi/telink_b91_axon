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

#ifdef __cplusplus
extern "C" {
#endif
#include "stdarg.h"
#include "axon_api.h"
/*
 * Axon interrupt handler.
 */
void AxonDemoHandleIrq();

/*
 * Formats a string and prints it using AxonHostLog()
 */
void AxonPrintf(char *fmt_string, ...);
/*
 * Prepares the demo.
 */
int AxonDemoPrepare(void *unused);

/*
 * Runs the demo. This will process a set of stored audio files.
 */
int AxonDemoRun(void *unused1, uint8_t unused2);

typedef enum {
  kClassifyOnValidWindow,  /**< "automatic" mode. classifications occurs whenever there is a valid window of audio */
  kDoNotClassify,           /**< "manual" mode, process audio frame but do not classify */
  kDoClassify                /**< "manual" mode, process audio frame and perform classification on processed audio frames */
}KwsClassifyOptionEnum;


typedef enum {
  kMiddleFrame,
  kFirstFrame,
  kLastFrame
}KwsFirstOrLastAudioFrame;
/**
 * called frame-by-frame to process an audio stream.
 *
 * @param raw_input_ping First set of audio samples of a single, 32ms (512sample) frame.
 * @param ping_count     Number of samples in raw_input_ping, must be less than or equal to 512.
 * @param raw_input_pong Remaining audio samples in the 32ms frame. Implied count is 512-ping_count
 * @param input_stride   Offset in (in whole samples) to the next sample in the buffer. 0=>use the same sample for the whole frame, 1=>mono samples 2=>stereo samples
 * @param first_frame    if 1, indicates that state information from previous invocations should be discarded. Otherwise, previous state is retained.
 * @param classify_option See  @KwsClassifyOptionEnum.
 */
int AxonKwsProcessFrame(
    const int16_t *raw_input_ping,
    uint32_t ping_count,
    const int16_t *raw_input_pong,
    uint8_t input_stride,
    KwsFirstOrLastAudioFrame first_or_last_frame,
    KwsClassifyOptionEnum classify_option);

int AxonKwsClearLastResult(const char **result_label);

/**
 * returns 1 if last audio frame submitted was "foreground" (high volume), 0 if not.
 */
int AxonKwsLastFrameWasForeground();

/**
 * Call-back function implemented by host. Invoked by library when the start of a valid
 * window of audio has been detected. Users should read in any audio features from this function
 * to guarantee that they don't shift in time while the read back is occurring.
 */
extern void AxonMlDemoHostStartWindowReady(uint32_t start_frame_no, uint32_t frame_cnt);

/**
 * Call-back function implemented by host, invoked to indicate classification has started.
 * In synchronous mode, classification will block for 100's of milliseconds.
 *
 * In asynchronous mode, call-back occurs in Axon ISR context, and classification occurs in Axon ISR context.
 * Because the work is done predominantly by Axon, not the CPU, the CPU is available for other work load.
 */
extern void AxonMlDemoHostClassifyingStart(uint32_t start_frame_no, uint32_t frame_cnt);

/**
 * Call-back function implemented by host, invoked to indicate classification has completed.
 */
extern void AxonMlDemoHostClassifyingEnd(uint32_t classification_number);

/**
 * Call-back function implemented by host, invoked to indicate classification will not occur because
 * the last audio frame was received and no valid audio window was detected.
 */
extern void AxonMlDemoHostNoClassification();

/**
 * Call-back function implemented by host to put axon in its lowest power state
 * enabled=True  : turns on the clock and power to Axon
 * enabled=False : gates the clock and power to Axon
 */
extern void AxonMlDemoHostAxonSetEnabled(AxonBoolEnum enabled);
#ifdef __cplusplus
} // extern "C" {
#endif

