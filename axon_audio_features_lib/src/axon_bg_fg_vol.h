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
#include "axon_api.h"
#include <assert.h>


/*
 * Energy threshold that the current slice must exceed background energy
 * by to be considered foreground audio.
 */
#define FOREGROUND_THRESHOLD (2.0)

/*
 * Alpha value for adapting back ground to a foreground slice.
 */
#define FOREGROUND_ALPHA    (0.015625)
/*
 * Alpha value for adapting background to a background slice.
 */
#define BACKGROUND_ALPHA    (0.3)
#define POWER_ROUND         0

#define FIXED_LENGTH_WINDOW_TYPE 0
#define VAR_LENGTH_WINDOW_TYPE 1
#define FIXED_INTERVAL 2  // all windows that meet minimum length requirement are allowed.

#if VOICE_WINDOW_TYPE==FIXED_INTERVAL
# define WINDOW_LENGTH  62         // max number of frames between long backgrounds to qualify as a valid window
# define WINDOW_INTERVAL  31         // minimum number of frames between long backgrounds to qualify as a valid window
# define LONG_BACKGROUND_LENGTH  8     // min number of consecutive background frames to qualify as long background
# define LONG_FORGROUND_MIN_LENGTH  8  // min number of consecutive foreground frames to qualify as long foreground
# define WINDOW_MAX_LENGTH  33         // max number of frames between long backgrounds to qualify as a valid window
# define WINDOW_MIN_LENGTH  33         // minimum number of frames between long backgrounds to qualify as a valid window
# define WINDOW_MAX_LONG_FOREGROUNDS  2
# define WINDOW_MIN_LONG_FOREGROUNDS  1
# define WINDOW_MAX_SHORT_FOREGROUNDS  2
# define WINDOW_MIN_SHORT_FOREGROUNDS  0
# define WINDOW_MAX_ENDING_BACKGROUND  33
#elif VOICE_WINDOW_TYPE==FIXED_LENGTH_WINDOW_TYPE
#elif VOICE_WINDOW_TYPE==VAR_LENGTH_WINDOW_TYPE
/*
 * Original (almost) implementation. The window can be variable length and
 * must consist of pure foreground (original version didn't enforce the leading and
 * trailing background sequences)
 */
# define LONG_BACKGROUND_LENGTH  8     // min number of consecutive background frames to qualify as long background
# define LONG_FORGROUND_MIN_LENGTH  10  // min number of consecutive foreground frames to qualify as long foreground
# define WINDOW_MAX_LENGTH  32         // max number of frames between long backgrounds to qualify as a valid window
# define WINDOW_MIN_LENGTH  10         // max number of frames between long backgrounds to qualify as a valid window
# define WINDOW_MAX_LONG_FOREGROUNDS  1
# define WINDOW_MIN_LONG_FOREGROUNDS  1
# define WINDOW_MAX_SHORT_FOREGROUNDS  10
# define WINDOW_MIN_SHORT_FOREGROUNDS  0
# define WINDOW_MAX_ENDING_BACKGROUND  33
#endif


typedef struct {
  float current_energy;
  uint32_t frame_cnt;              // total frames processed since last Restart.
  uint32_t current_background_cnt; // number of consecutive background level samples. 0 if currently in foreground
  uint32_t prev_background_cnt; // number of previous consecutive background level samples.
  uint32_t prev_prev_background_cnt; // number of previous consecutive background level samples.
  uint32_t current_foreground_cnt; // number of consecutive foreground level samples. 0 if currently in background
  uint32_t prev_foreground_cnt; // number of previous consecutive foreground level samples.
  uint32_t prev_prev_foreground_cnt; // number of previous consecutive foreground level samples.
  uint32_t frames_since_last_long_background_end;
  uint32_t starting_long_background_length;
  uint32_t long_foreground_count;    // number of long foregrounds since last long background
  uint32_t short_foreground_count;    // number of short foregrounds since last long background
  uint8_t valid_window_length;
  uint8_t starting_window_length;
  uint32_t execution_time_ticks;
} BackgroundForegroundResultsStruct;


void AxonBgFgRestart();

AxonResultEnum AxonBgFgProcessFrame(void *axon_handle, AxonBoolEnum last_frame, AxonAsyncModeEnum async_mode);
/*
 * Call this to get the processing state (busy=1, idle=0)
 */
AxonResultEnum AxonBgFgProcessState();

AxonResultEnum AxonBgFgPrepare(void *axon_handle, int32_t *raw_input, uint32_t raw_input_len, uint8_t bgfg_window_slice_cnt);
