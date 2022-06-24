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
#include <string.h>
#include <stdio.h>


#include "axon_api.h"
#include "axon_dep.h"
#include "axon_audio_features_api.h"
#include <math.h>
#include "axon_bg_fg_vol.h"

extern void AxonPrintf(char *fmt_string, ...);

/*
 * In duty cycle mode, the microphone turns on and off repeatedly,with the off-time >> on-time (to save power).
 * In this case, we will assume that each window starts w/ a long silence.
 */
#define BGFG_DUTY_CYCLE_MODE 1

#define BGFG_SUBTRACT_MEAN 1
/*
 * On AZ-N1 there is a DC bias on the microphone when it 1st turns on. Subtracting
 * the mean will minimize its affect on the measured energy.
 */
typedef enum {
#if BGFG_SUBTRACT_MEAN
  kBgFgAxonOpSampleMeanAccum, // sum the samples then round to calculate the mean
  kBgFgAxonOpSubtractMean,    // subtract the mean from the samples
  kBgFgAxonOpAltSamplePowerL2Norm,
#endif
  kBgFgAxonOpSamplePowerL2Norm,
  kBgFgAxonOpCount // 1 operation
} BgFgAxonOperationEnum;

RETAINED_MEMORY_SECTION_ATTRIBUTE
struct {
  BackgroundForegroundResultsStruct r;
#if BGFG_SUBTRACT_MEAN
  int32_t current_sample_mean;
  int32_t alt_sample_power;
  int32_t minus_1;
  int32_t *input_ptr;
#endif
  uint32_t current_sample_power;
  float background_level; // current level of background volume.
  AxonOpHandle axon_ops[kBgFgAxonOpCount];
  uint32_t profiling_timestamp;
  uint8_t winddow_width_in_slices;
  volatile uint8_t busy;
} BgFgInfoStruct;

void AxonBgFgPrintStats() {
  AxonPrintf("BG/FG Info: # %d, P=%u, E=%f, BG=%f, FGCnt=%d, BGCnt=%d, AltP=%u, Avg=%d\r\n",
      BgFgInfoStruct.r.frame_cnt, BgFgInfoStruct.current_sample_power, BgFgInfoStruct.r.current_energy,
      BgFgInfoStruct.background_level, BgFgInfoStruct.r.current_foreground_cnt, BgFgInfoStruct.r.current_background_cnt
#if BGFG_SUBTRACT_MEAN
      ,BgFgInfoStruct.alt_sample_power, BgFgInfoStruct.current_sample_mean);
#else
  ,0,0);
#endif
}

void AxonBgFgRestart() {
  memset(&BgFgInfoStruct.r, 0, sizeof(BgFgInfoStruct.r));
}


/*
 * API function
 */
AxonResultEnum AxonBgFgPrepare(void *axon_handle, int32_t *raw_input, uint32_t raw_input_len, uint8_t bgfg_window_slice_cnt) {
  AxonInputStruct axon_input;
  AxonResultEnum result;

  AxonBgFgRestart();
#if BGFG_SUBTRACT_MEAN
  BgFgInfoStruct.input_ptr = raw_input;
  // sum the samples then round to calculate the mean
  axon_input.length = raw_input_len>>1;
  axon_input.data_width = kAxonDataWidth24;
  axon_input.data_packing = kAxonDataPackingDisabled;
  axon_input.output_rounding = kAxonRoundingNone+8; // divide by 256
  axon_input.output_af = kAxonAfDisabled;
  axon_input.x_in = raw_input;
  axon_input.x_stride = kAxonStride2;
  axon_input.q_out = (int32_t*)&BgFgInfoStruct.current_sample_mean;
  axon_input.q_stride = kAxonStride1;

  if (kAxonResultSuccess > (result=AxonApiDefineOpAcc(axon_handle, &axon_input, &BgFgInfoStruct.axon_ops[kBgFgAxonOpSampleMeanAccum]))) {
    return result;
  }
  // subtract the mean from the samples. Can't actually do that, but we can negate the values and add the mean to them since they are going to be squared in the next stop anyhow.
  BgFgInfoStruct.minus_1 = -1;
  axon_input.output_rounding = kAxonRoundingNone;
  axon_input.output_af = kAxonAfDisabled;
  axon_input.x_in = raw_input;
  axon_input.x_stride = kAxonStride2;
  axon_input.a_in = (int32_t)&BgFgInfoStruct.minus_1;
  axon_input.b_in = (int32_t)&BgFgInfoStruct.current_sample_mean;
  axon_input.q_out = raw_input+1; // place the output in the "imaginary" locations. Need to 0 this back out later.
  axon_input.q_stride = kAxonStride2;

  // use AxpbyPtr because the "b_in" is calculated in the previous step.
  if (kAxonResultSuccess > (result=AxonApiDefineOpAxpbPointer(axon_handle, &axon_input, &BgFgInfoStruct.axon_ops[kBgFgAxonOpSubtractMean]))) {
    return result;
  }
  // last, square and sum the new values
  axon_input.length = raw_input_len>>1;
  axon_input.data_width = kAxonDataWidth24;
  axon_input.data_packing = kAxonDataPackingDisabled;
  axon_input.output_rounding = kAxonRoundingNone;
  axon_input.output_af = kAxonAfDisabled;
  axon_input.x_in = raw_input+1;
  axon_input.x_stride = kAxonStride2;
  axon_input.q_out = (int32_t*)&BgFgInfoStruct.alt_sample_power;
  axon_input.q_stride = kAxonStride1;

  if (kAxonResultSuccess > (result=AxonApiDefineOpL2norm(axon_handle, &axon_input, &BgFgInfoStruct.axon_ops[kBgFgAxonOpAltSamplePowerL2Norm]))) {
    return result;
  }

#endif

  // step 1, square & sum the values
  axon_input.length = raw_input_len>>1;
  axon_input.data_width = kAxonDataWidth24;
  axon_input.data_packing = kAxonDataPackingDisabled;
  axon_input.output_rounding = kAxonRoundingNone;
  axon_input.output_af = kAxonAfDisabled;
  axon_input.x_in = raw_input;
  axon_input.x_stride = kAxonStride2;
  axon_input.q_out = (int32_t*)&BgFgInfoStruct.current_sample_power;
  axon_input.q_stride = kAxonStride1;

  if (kAxonResultSuccess > (result=AxonApiDefineOpL2norm(axon_handle, &axon_input, &BgFgInfoStruct.axon_ops[kBgFgAxonOpSamplePowerL2Norm]))) {
    return result;
  }
  BgFgInfoStruct.winddow_width_in_slices = bgfg_window_slice_cnt;
  return kAxonResultSuccess;
}

/**
 * This function returns whether or not there is a valid window to perform classification on.
 * To be a valid window, it must...
 * 1) start and end with long background periods.
 * 2) Not have any long background periods in it.
 * 3) Have at least 1 long foreground period in it.
 * 4) Be exactly the window length long.
 */
typedef enum {
  kInForeground,
  kInLongBackground,
  kLongBackgroundEnding
} BgFgWindowWidthCalcEventEnum;
static void calc_window_width(BackgroundForegroundResultsStruct *bg_fg_results, BgFgWindowWidthCalcEventEnum event) {
#if VOICE_WINDOW_TYPE==FIXED_INTERVAL
  if(BgFgInfoStruct.r.frame_cnt==WINDOW_LENGTH) {
    bg_fg_results->frames_since_last_long_background_end = WINDOW_LENGTH;
    // reset the count
    BgFgInfoStruct.r.frame_cnt = WINDOW_LENGTH-WINDOW_INTERVAL;

    return kAxonBoolTrue;
  }
  return kAxonBoolFalse;
#elif FIXED_LENGTH_WINDOW_TYPE==VOICE_WINDOW_TYPE
  bg_fg_results->valid_window_length = 0;
  /*
   * Framing of the window is as follows:
   * 1) Currently in a long background and the previous long background ended exactly BgFgInfoStruct.winddow_width_in_slices-LONG_BACKGROUND_LENGTH
   *    frames ago. This forward-biases the voice activity to the front of a window.
   * 2) A long background just ended but BgFgInfoStruct.winddow_width_in_slices-LONG_BACKGROUND_LENGTH frames haven't passed.
   *    The window can be backed-up so long as the leading long-backround is long enough to absorb the difference.
   *
   * Once the window is framed, it has to satisfy the min/max long/short foreground counts
   */

  if ( ( (event==kInLongBackground) &&// currently in a a long background...
         (bg_fg_results->frames_since_last_long_background_end <= BgFgInfoStruct.winddow_width_in_slices) &&//...
         (bg_fg_results->frame_cnt >= BgFgInfoStruct.winddow_width_in_slices) && // ...and have enough frames...
           ((bg_fg_results->frame_cnt==bg_fg_results->frames_since_last_long_background_end==BgFgInfoStruct.winddow_width_in_slices)
           ||
           ((BgFgInfoStruct.winddow_width_in_slices-LONG_BACKGROUND_LENGTH)== bg_fg_results->frames_since_last_long_background_end))) // ...and last long background ended exactly BgFgInfoStruct.winddow_width_in_slices-LONG_BACKGROUND_LENGTH frames ago
        || // OR
       ( (event==kLongBackgroundEnding) &&
         (BgFgInfoStruct.winddow_width_in_slices <= (bg_fg_results->frames_since_last_long_background_end+BgFgInfoStruct.r.starting_long_background_length)) &&
         ((BgFgInfoStruct.winddow_width_in_slices-LONG_BACKGROUND_LENGTH)>= bg_fg_results->frames_since_last_long_background_end) ))
  {
    if ((WINDOW_MIN_LONG_FOREGROUNDS  <= BgFgInfoStruct.r.long_foreground_count) &&       // enough long foregrounds in the window...
      (WINDOW_MAX_LONG_FOREGROUNDS  >= BgFgInfoStruct.r.long_foreground_count) &&      // ...but not too many
      (WINDOW_MIN_SHORT_FOREGROUNDS  <= BgFgInfoStruct.r.short_foreground_count) &&    // enough short foregrounds in the window...
      (WINDOW_MAX_SHORT_FOREGROUNDS  >= BgFgInfoStruct.r.short_foreground_count) ) {     // ...but not too many
      bg_fg_results->valid_window_length = BgFgInfoStruct.winddow_width_in_slices;
    }
  }
#else
  return
      (bg_fg_results->frames_since_last_long_background_end<=(WINDOW_MAX_LENGTH+LONG_BACKGROUND_LENGTH)) && // exact number of frames since window start
      (bg_fg_results->frames_since_last_long_background_end>=(WINDOW_MIN_LENGTH+LONG_BACKGROUND_LENGTH)) && // exact number of frames since window start
      (LONG_BACKGROUND_LENGTH <= BgFgInfoStruct.r.current_background_cnt) &&           // window is ending on a long background
      (WINDOW_MIN_LONG_FOREGROUNDS  <= BgFgInfoStruct.r.long_foreground_count) &&       // enough long foregrounds in the window...
      (WINDOW_MAX_LONG_FOREGROUNDS  >= BgFgInfoStruct.r.long_foreground_count) &&      // ...but not too many
      (WINDOW_MIN_SHORT_FOREGROUNDS  <= BgFgInfoStruct.r.short_foreground_count) &&    // enough short foregrounds in the window...
      (WINDOW_MAX_SHORT_FOREGROUNDS  >= BgFgInfoStruct.r.short_foreground_count);      // ...but not too many
#endif
}

/*
 * callback when axon ops complete.
 */
static bg_fg_ops_done_callback(AxonResultEnum result, void *callback_context) {
  AxonBoolEnum last_frame = (AxonBoolEnum)callback_context;
  // not busy any more
  BgFgInfoStruct.busy = 0;
#if BGFG_SUBTRACT_MEAN
  // need to 0 out the imaginary slots.
  for (int sample_ndx=0; sample_ndx < AXON_AUDIO_FEATURE_FRAME_LEN; sample_ndx+=2) {
    *(BgFgInfoStruct.input_ptr+1+sample_ndx) = 0;
  }
  float power_f = BgFgInfoStruct.alt_sample_power;
#else
  float power_f = BgFgInfoStruct.current_sample_power;
#endif

  // take the natural log of the power to get the energy
  power_f *= (1<<POWER_ROUND);
  BgFgInfoStruct.r.current_energy = logf(power_f);

  // increment the window length, but only if we've seen
  // a long background already some time in the past
  if (0 < BgFgInfoStruct.r.frames_since_last_long_background_end) {
    BgFgInfoStruct.r.frames_since_last_long_background_end++;
  }

#if BGFG_DUTY_CYCLE_MODE
  if (0==BgFgInfoStruct.r.frame_cnt) {
    // fake a long silence here
    BgFgInfoStruct.r.current_background_cnt = LONG_BACKGROUND_LENGTH;
  }
#endif

  BgFgInfoStruct.r.frame_cnt++;
  if (0==BgFgInfoStruct.background_level) {
    // 1st one, initialize bg to current energy
    BgFgInfoStruct.background_level = BgFgInfoStruct.r.current_energy;
    BgFgInfoStruct.r.current_background_cnt = 1;
  } else if ((BgFgInfoStruct.r.current_energy-BgFgInfoStruct.background_level)>FOREGROUND_THRESHOLD) {
    // this is a foreground sample, increment the current foreground count
    if(0==BgFgInfoStruct.r.current_foreground_cnt++) {
      // just broke a streak of background samples
      BgFgInfoStruct.r.prev_background_cnt = BgFgInfoStruct.r.current_background_cnt;
      BgFgInfoStruct.r.current_background_cnt = 0;
      // was this a long background?
      if (LONG_BACKGROUND_LENGTH <= BgFgInfoStruct.r.prev_background_cnt) {
        // before we lose this long-background, check to see if there was a valid window
        calc_window_width(&BgFgInfoStruct.r, kLongBackgroundEnding);

        // yep, 0 out window stats
        BgFgInfoStruct.r.frames_since_last_long_background_end=1;
        BgFgInfoStruct.r.long_foreground_count = 0;
        BgFgInfoStruct.r.short_foreground_count = 0;
        BgFgInfoStruct.r.starting_long_background_length = BgFgInfoStruct.r.prev_background_cnt;
      }
    }
    // update the backgound using alpha_foreground
    BgFgInfoStruct.background_level = (1-FOREGROUND_ALPHA) * BgFgInfoStruct.background_level +
        FOREGROUND_ALPHA * BgFgInfoStruct.r.current_energy;
  } else {
    // this is a background sample, increment the current background count
    if(0==BgFgInfoStruct.r.current_background_cnt++) {
      // just broke a streak of foreground samples
      BgFgInfoStruct.r.prev_prev_foreground_cnt = BgFgInfoStruct.r.prev_foreground_cnt;
      BgFgInfoStruct.r.prev_foreground_cnt = BgFgInfoStruct.r.current_foreground_cnt;
      BgFgInfoStruct.r.current_foreground_cnt = 0;
      // was that a long foreground, or a short one?
      if (LONG_FORGROUND_MIN_LENGTH <= BgFgInfoStruct.r.prev_foreground_cnt) {
        BgFgInfoStruct.r.long_foreground_count++; // a long one
      } else {
        BgFgInfoStruct.r.short_foreground_count++; // short one
      }
    }
    // if this is a long background, check for a valid window
    if (LONG_BACKGROUND_LENGTH <= BgFgInfoStruct.r.current_background_cnt) {
      calc_window_width(&BgFgInfoStruct.r, last_frame ? kLongBackgroundEnding : kInLongBackground);
    }
    // update the backgound using alpha_foreground
    BgFgInfoStruct.background_level = (1-BACKGROUND_ALPHA) * BgFgInfoStruct.background_level +
        BACKGROUND_ALPHA * BgFgInfoStruct.r.current_energy;

  }

  // profiling
  BgFgInfoStruct.r.execution_time_ticks += (AxonHostGetTime()-BgFgInfoStruct.profiling_timestamp);
}

/*
 * API function
 * Note: raw_input expects signed 32bit values every other index, ie, the input format to the FFT.
 * This will queue up an operation then perform all the calculations in the callback.
 */
AxonResultEnum AxonBgFgProcessFrame(void *axon_handle, AxonBoolEnum last_frame, AxonAsyncModeEnum async_mode) {
  /*
   * queued ops struct doesn't need to be
   * in retained memory.
   */
  static AxonMgrQueuedOpsStruct bg_fg_queued_ops;
  BgFgInfoStruct.profiling_timestamp = AxonHostGetTime();
  BgFgInfoStruct.r.valid_window_length = 0;
  if (async_mode==kAxonAsyncModeSynchronous) {
    AxonApiExecuteOps(axon_handle, kBgFgAxonOpCount,  BgFgInfoStruct.axon_ops, kAxonAsyncModeSynchronous);
    AxonApiQueueOpsList(axon_handle,&bg_fg_queued_ops);
    bg_fg_ops_done_callback(kAxonResultSuccess, (void *)last_frame);
    return kAxonResultSuccess;
  } else {
    // queued batch for async mode
    bg_fg_queued_ops.callback_context = (void *)last_frame; // to pass this on to the processing callback
    bg_fg_queued_ops.callback_function = bg_fg_ops_done_callback;
    bg_fg_queued_ops.op_handle_count = kBgFgAxonOpCount;
    bg_fg_queued_ops.op_handle_list = BgFgInfoStruct.axon_ops;
    // mark as busy
    BgFgInfoStruct.busy = 1;
    return AxonApiQueueOpsList(axon_handle,&bg_fg_queued_ops);
  }
}
/*
 * return
 */
AxonResultEnum AxonBgFgProcessState() {
  // return
  return BgFgInfoStruct.busy ? kAxonResultNotFinished : kAxonResultSuccess;
}

/**
 * This function returns whether or not there is a valid window to perform classification on.
 * To be a valid window, it must...
 * 1) start and end with long background periods.
 * 2) Not have any long background periods in it.
 * 3) Have at least 1 long foreground period in it.
 * 4) Be exactly the window length long.
 */
uint8_t AxonBgFgWindowWidth() {
#if VOICE_WINDOW_TYPE==FIXED_INTERVAL
  if(BgFgInfoStruct.r.frame_cnt==WINDOW_LENGTH) {
    bg_fg_results->frames_since_last_long_background_end = WINDOW_LENGTH;
    // reset the count
    BgFgInfoStruct.r.frame_cnt = WINDOW_LENGTH-WINDOW_INTERVAL;

    return kAxonBoolTrue;
  }
  return kAxonBoolFalse;
#elif FIXED_LENGTH_WINDOW_TYPE==VOICE_WINDOW_TYPE
  return BgFgInfoStruct.r.valid_window_length;
#else
  return
      (bg_fg_results->frames_since_last_long_background_end<=(WINDOW_MAX_LENGTH+LONG_BACKGROUND_LENGTH)) && // exact number of frames since window start
      (bg_fg_results->frames_since_last_long_background_end>=(WINDOW_MIN_LENGTH+LONG_BACKGROUND_LENGTH)) && // exact number of frames since window start
      (LONG_BACKGROUND_LENGTH <= BgFgInfoStruct.r.current_background_cnt) &&           // window is ending on a long background
      (WINDOW_MIN_LONG_FOREGROUNDS  <= BgFgInfoStruct.r.long_foreground_count) &&       // enough long foregrounds in the window...
      (WINDOW_MAX_LONG_FOREGROUNDS  >= BgFgInfoStruct.r.long_foreground_count) &&      // ...but not too many
      (WINDOW_MIN_SHORT_FOREGROUNDS  <= BgFgInfoStruct.r.short_foreground_count) &&    // enough short foregrounds in the window...
      (WINDOW_MAX_SHORT_FOREGROUNDS  >= BgFgInfoStruct.r.short_foreground_count);      // ...but not too many
#endif
}

uint8_t AxonAudioFeaturesBgFgWindowWidth() {
  return BgFgInfoStruct.r.valid_window_length;
}

uint32_t AxonAudioFeaturesBgFgWindowFirstFrame() {
  return BgFgInfoStruct.r.frame_cnt - BgFgInfoStruct.r.valid_window_length;
}

uint32_t AxonAudioFeaturesBgFgExecutionTicks() {
  return BgFgInfoStruct.r.execution_time_ticks;
}

uint8_t AxonAudioFeaturesBgSliceIsForeground() {
  return BgFgInfoStruct.r.current_foreground_cnt;
}
