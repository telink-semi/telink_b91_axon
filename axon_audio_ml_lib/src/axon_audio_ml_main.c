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
#include <stdio.h>
#include <string.h>
#include "axon_api.h"
#include "axon_dep.h"
#include "axon_audio_features_api.h"
#include "axon_audio_framework_api.h"
#include "axon_logging_api.h"
#include "axon_audio_ml_api.h"

extern AxonInstanceStruct *gl_axon_instance;
#define AXON_GRNN 1
#define AXON_FC4  2
#define AXON_LSTM  3

#if AXON_NN_TYPE==AXON_GRNN
# include "axon_grnn_api.h"
# define AxonKwsModelInfer AxonKwsModelGrnnInfer
# define AxonKwsModelGetClassification AxonKwsModelGrnnGetClassification
# define AxonKwsModelGetInputAttributes AxonKwsModelGrnnGetInputAttributes
# define AxonKwsModelPrepare AxonKwsModelGrnnPrepare
#elif AXON_NN_TYPE==AXON_FC4
# include "axon_kws_model_fc4_api.h"
# define AxonKwsModelInfer AxonKwsModelFc4Infer
# define AxonKwsModelGetClassification AxonKwsModelFc4GetClassification
# define AxonKwsModelGetInputAttributes AxonKwsModelFc4GetInputAttributes
# define AxonKwsModelPrepare AxonKwsModelFc4Prepare
#elif AXON_NN_TYPE==AXON_LSTM
# include "axon_kws_model_lstm_1fc_api.h"
# define AxonKwsModelInfer AxonKwsModelLstm1fcInfer
# define AxonKwsModelGetClassification AxonKwsModelLstm1fcGetClassification
# define AxonKwsModelGetInputAttributes AxonKwsModelLstm1fcGetInputAttributes
# define AxonKwsModelPrepare AxonKwsModelLstm1fcPrepare
#endif


#define AUDIO_SAMPLE_GROUP_0 0
#define AUDIO_SAMPLE_GROUP_DAN_DOWN 2
#define AUDIO_SAMPLE_GROUP_DAN_NO 3
#define AUDIO_SAMPLE_GROUP_DAN_OFF 4
#define AUDIO_SAMPLE_GROUP_DAN_ON  5
#define AUDIO_SAMPLE_GROUP_DAN_UP  6
#define AUDIO_SAMPLE_GROUP_DAN_YES  7
#define AUDIO_SAMPLE_GROUP_POWERDOWN 8
#define AUDIO_SAMPLE_GROUP_TURNON 9
#define AUDIO_SAMPLE_GROUP_POWERUP 10
#define AUDIO_SAMPLE_GROUP_BATTERYSTATUS 11
#define AUDIO_SAMPLE_GROUP_MIKE_DOWN 12

#define AUDIO_SAMPLE_GROUP_GO 13
#define AUDIO_SAMPLE_GROUP_DOWN 14
#define AUDIO_SAMPLE_GROUP_RIGHT 15
#define AUDIO_SAMPLE_GROUP_STOP 16
#define AUDIO_SAMPLE_GROUP_UP 17
#define AUDIO_SAMPLE_GROUP_LEFT 18
#define AUDIO_SAMPLE_GROUP_OFF 19
#define AUDIO_SAMPLE_GROUP_YES 20
#define AUDIO_SAMPLE_GROUP_NO 21
#define AUDIO_SAMPLE_GROUP AUDIO_SAMPLE_GROUP_0

/*
 * AUDIO TEST VECTORS
 */
#include "test_audio/rawSampleon.h"

#if AUDIO_SAMPLE_GROUP>AUDIO_SAMPLE_GROUP_0
# include "test_audio/rawSamplego.h"
# include "test_audio/rawSampledown.h"
# include "test_audio/rawSampleright.h"
# include "test_audio/rawSamplestop.h"
# include "test_audio/rawSampleup.h"
# include "test_audio/rawSampleleft.h"
# include "test_audio/rawSampleoff.h"
# include "test_audio/rawSampleyes.h"
# include "test_audio/rawSampleno.h"
# include "test_audio/raw_samples_dan_down.h"
# include "test_audio/raw_samples_dan_no.h"
# include "test_audio/raw_samples_dan_off.h"
# include "test_audio/raw_samples_dan_on.h"
# include "test_audio/raw_samples_dan_up.h"
# include "test_audio/raw_samples_dan_yes.h"
# include "test_audio/raw_samples_rudy_powerdown.h"
# include "test_audio/raw_samples_mike_turnon.h"
# include "test_audio/raw_samples_mike_powerup.h"
#endif

static const struct {
  const char *sample_label;
  int32_t sample_count;
  const int16_t *wave_data;
}  audio_sample_files[] ={
#if AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_0
    {
        .sample_label = AUDIO_NAME_ON,
        .sample_count = sizeof(wave_data_on)/sizeof(wave_data_on[0]),
        .wave_data = wave_data_on,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_DAN_DOWN
# if 1
    {
        .sample_label = AUDIO_NAME_DAN_DOWN1,
        .sample_count = sizeof(wave_data_dan_down1)/sizeof(wave_data_dan_down1[0]),
        .wave_data = wave_data_dan_down1,
    },
    {
        .sample_label = AUDIO_NAME_DAN_DOWN2,
        .sample_count = sizeof(wave_data_dan_down2)/sizeof(wave_data_dan_down2[0]),
        .wave_data = wave_data_dan_down2,
    },
    {
        .sample_label = AUDIO_NAME_DAN_DOWN3,
        .sample_count = sizeof(wave_data_dan_down3)/sizeof(wave_data_dan_down3[0]),
        .wave_data = wave_data_dan_down3,
    },
    {
        .sample_label = AUDIO_NAME_DAN_DOWN4,
        .sample_count = sizeof(wave_data_dan_down4)/sizeof(wave_data_dan_down4[0]),
        .wave_data = wave_data_dan_down4,
    },
# endif
    {
        .sample_label = AUDIO_NAME_DAN_DOWN5,
        .sample_count = sizeof(wave_data_dan_down5)/sizeof(wave_data_dan_down5[0]),
        .wave_data = wave_data_dan_down5,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_DAN_NO
    {
        .sample_label = AUDIO_NAME_DAN_NO1,
        .sample_count = sizeof(wave_data_dan_no1)/sizeof(wave_data_dan_no1[0]),
        .wave_data = wave_data_dan_no1,
    },
    {
        .sample_label = AUDIO_NAME_DAN_NO2,
        .sample_count = sizeof(wave_data_dan_no2)/sizeof(wave_data_dan_no2[0]),
        .wave_data = wave_data_dan_no2,
    },
    {
        .sample_label = AUDIO_NAME_DAN_NO3,
        .sample_count = sizeof(wave_data_dan_no3)/sizeof(wave_data_dan_no3[0]),
        .wave_data = wave_data_dan_no3,
    },
    {
        .sample_label = AUDIO_NAME_DAN_NO4,
        .sample_count = sizeof(wave_data_dan_no4)/sizeof(wave_data_dan_no4[0]),
        .wave_data = wave_data_dan_no4,
    },
    {
        .sample_label = AUDIO_NAME_DAN_NO5,
        .sample_count = sizeof(wave_data_dan_no5)/sizeof(wave_data_dan_no5[0]),
        .wave_data = wave_data_dan_no5,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_DAN_OFF
    {
        .sample_label = AUDIO_NAME_DAN_OFF1,
        .sample_count = sizeof(wave_data_dan_off1)/sizeof(wave_data_dan_off1[0]),
        .wave_data = wave_data_dan_off1,
    },
    {
        .sample_label = AUDIO_NAME_DAN_OFF2,
        .sample_count = sizeof(wave_data_dan_off2)/sizeof(wave_data_dan_off2[0]),
        .wave_data = wave_data_dan_off2,
    },
    {
        .sample_label = AUDIO_NAME_DAN_OFF3,
        .sample_count = sizeof(wave_data_dan_off3)/sizeof(wave_data_dan_off3[0]),
        .wave_data = wave_data_dan_off3,
    },
    {
        .sample_label = AUDIO_NAME_DAN_OFF4,
        .sample_count = sizeof(wave_data_dan_off4)/sizeof(wave_data_dan_off4[0]),
        .wave_data = wave_data_dan_off4,
    },
    {
        .sample_label = AUDIO_NAME_DAN_OFF5,
        .sample_count = sizeof(wave_data_dan_off5)/sizeof(wave_data_dan_off5[0]),
        .wave_data = wave_data_dan_off5,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_DAN_ON
    {
        .sample_label = AUDIO_NAME_DAN_ON1,
        .sample_count = sizeof(wave_data_dan_on1)/sizeof(wave_data_dan_on1[0]),
        .wave_data = wave_data_dan_on1,
    },
    {
        .sample_label = AUDIO_NAME_DAN_ON2,
        .sample_count = sizeof(wave_data_dan_on2)/sizeof(wave_data_dan_on2[0]),
        .wave_data = wave_data_dan_on2,
    },
    {
        .sample_label = AUDIO_NAME_DAN_ON3,
        .sample_count = sizeof(wave_data_dan_on3)/sizeof(wave_data_dan_on3[0]),
        .wave_data = wave_data_dan_on3,
    },
#if 0 // corrupted sample
    {
        .sample_label = AUDIO_NAME_DAN_ON4,
        .sample_count = sizeof(wave_data_dan_on4)/sizeof(wave_data_dan_on4[0]),
        .wave_data = wave_data_dan_on4,
    },
#endif
    {
        .sample_label = AUDIO_NAME_DAN_ON5,
        .sample_count = sizeof(wave_data_dan_on5)/sizeof(wave_data_dan_on5[0]),
        .wave_data = wave_data_dan_on5,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_DAN_UP
    {
        .sample_label = AUDIO_NAME_DAN_UP1,
        .sample_count = sizeof(wave_data_dan_up1)/sizeof(wave_data_dan_up1[0]),
        .wave_data = wave_data_dan_up1,
    },
    {
        .sample_label = AUDIO_NAME_DAN_UP2,
        .sample_count = sizeof(wave_data_dan_up2)/sizeof(wave_data_dan_up2[0]),
        .wave_data = wave_data_dan_up2,
    },
    {
        .sample_label = AUDIO_NAME_DAN_UP3,
        .sample_count = sizeof(wave_data_dan_up3)/sizeof(wave_data_dan_up3[0]),
        .wave_data = wave_data_dan_up3,
    },
    {
        .sample_label = AUDIO_NAME_DAN_UP4,
        .sample_count = sizeof(wave_data_dan_up4)/sizeof(wave_data_dan_up4[0]),
        .wave_data = wave_data_dan_up4,
    },
    {
        .sample_label = AUDIO_NAME_DAN_UP5,
        .sample_count = sizeof(wave_data_dan_up5)/sizeof(wave_data_dan_up5[0]),
        .wave_data = wave_data_dan_up5,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_DAN_YES
    {
        .sample_label = AUDIO_NAME_DAN_YES1,
        .sample_count = sizeof(wave_data_dan_yes1)/sizeof(wave_data_dan_yes1[0]),
        .wave_data = wave_data_dan_yes1,
    },
    {
        .sample_label = AUDIO_NAME_DAN_YES2,
        .sample_count = sizeof(wave_data_dan_yes2)/sizeof(wave_data_dan_yes2[0]),
        .wave_data = wave_data_dan_yes2,
    },
    {
        .sample_label = AUDIO_NAME_DAN_YES3,
        .sample_count = sizeof(wave_data_dan_yes3)/sizeof(wave_data_dan_yes3[0]),
        .wave_data = wave_data_dan_yes3,
    },
    {
        .sample_label = AUDIO_NAME_DAN_YES4,
        .sample_count = sizeof(wave_data_dan_yes4)/sizeof(wave_data_dan_yes4[0]),
        .wave_data = wave_data_dan_yes4,
    },
    {
        .sample_label = AUDIO_NAME_DAN_YES5,
        .sample_count = sizeof(wave_data_dan_yes5)/sizeof(wave_data_dan_yes5[0]),
        .wave_data = wave_data_dan_yes5,
    },
#elif AUDIO_SAMPLE_GROUP==1
    {
        .sample_label = AUDIO_NAME_GO,
        .sample_count = sizeof(wave_data_go)/sizeof(wave_data_go[0]),
        .wave_data = wave_data_go,
    },
    {
        .sample_label = AUDIO_NAME_DOWN,
        .sample_count = sizeof(wave_data_down)/sizeof(wave_data_down[0]),
        .wave_data = wave_data_down,
    },
    {
        .sample_label = AUDIO_NAME_RIGHT,
        .sample_count = sizeof(wave_data_right)/sizeof(wave_data_right[0]),
        .wave_data = wave_data_right,
    },
    {
        .sample_label = AUDIO_NAME_STOP,
        .sample_count = sizeof(wave_data_stop)/sizeof(wave_data_stop[0]),
        .wave_data = wave_data_stop,
    },
    {
        .sample_label = AUDIO_NAME_UP,
        .sample_count = sizeof(wave_data_up)/sizeof(wave_data_up[0]),
        .wave_data = wave_data_up,
    },
    {
        .sample_label = AUDIO_NAME_LEFT,
        .sample_count = sizeof(wave_data_left)/sizeof(wave_data_left[0]),
        .wave_data = wave_data_left,
    },
    {
        .sample_label = AUDIO_NAME_NO,
        .sample_count = sizeof(wave_data_no)/sizeof(wave_data_no[9]),
        .wave_data = wave_data_no,
    },
    {   /* sample is shifted too far to the end so no valid window is detected */
        .sample_label = AUDIO_NAME_OFF,
        .sample_count = sizeof(wave_data_off)/sizeof(wave_data_off[9]),
        .wave_data = wave_data_off,
    },
    {
        .sample_label = AUDIO_NAME_YES,
        .sample_count = sizeof(wave_data_yes)/sizeof(wave_data_yes[9]),
        .wave_data = wave_data_yes,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_POWERDOWN
    {
        .sample_label = AUDIO_NAME_RUDY_POWERDOWN,
        .sample_count = sizeof(wave_data_rudy_powerdown)/sizeof(wave_data_rudy_powerdown[9]),
        .wave_data = wave_data_rudy_powerdown,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_TURNON
    {
        .sample_label = AUDIO_NAME_MIKE_TURNON2,
        .sample_count = sizeof(wave_data_mike_turnon2)/sizeof(wave_data_mike_turnon2[9]),
        .wave_data = wave_data_mike_turnon2,
    },
# if 0 // async hangs on 2nd file...
    {
        .sample_label = AUDIO_NAME_MIKE_TURNON,
        .sample_count = sizeof(wave_data_mike_turnon)/sizeof(wave_data_mike_turnon[9]),
        .wave_data = wave_data_mike_turnon,
    },
# endif
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_POWERUP
    {
        .sample_label = AUDIO_NAME_MIKE_POWERUP,
        .sample_count = sizeof(wave_data_mike_powerup)/sizeof(wave_data_mike_powerup[9]),
        .wave_data = wave_data_mike_powerup,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_BATTERYSTATUS
    {
        .sample_label = AUDIO_NAME_MIKE_BATTERYSTATUS,
        .sample_count = sizeof(wave_data_mike_batterystatus)/sizeof(wave_data_mike_batterystatus[9]),
        .wave_data = wave_data_mike_batterystatus,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_MIKE_DOWN
    {
        .sample_label = AUDIO_NAME_MIKE_DOWN,
        .sample_count = sizeof(wave_data_mike_down)/sizeof(wave_data_mike_down[9]),
        .wave_data = wave_data_mike_down,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_GO
    {
        .sample_label = AUDIO_NAME_GO,
        .sample_count = sizeof(wave_data_go)/sizeof(wave_data_go[0]),
        .wave_data = wave_data_go,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_DOWN
    {
        .sample_label = AUDIO_NAME_DOWN,
        .sample_count = sizeof(wave_data_down)/sizeof(wave_data_down[0]),
        .wave_data = wave_data_down,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_RIGHT
    {
        .sample_label = AUDIO_NAME_RIGHT,
        .sample_count = sizeof(wave_data_right)/sizeof(wave_data_right[0]),
        .wave_data = wave_data_right,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_STOP
    {
        .sample_label = AUDIO_NAME_STOP,
        .sample_count = sizeof(wave_data_stop)/sizeof(wave_data_stop[0]),
        .wave_data = wave_data_stop,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_UP
    {
        .sample_label = AUDIO_NAME_UP,
        .sample_count = sizeof(wave_data_up)/sizeof(wave_data_up[0]),
        .wave_data = wave_data_up,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_LEFT
    {
        .sample_label = AUDIO_NAME_LEFT,
        .sample_count = sizeof(wave_data_left)/sizeof(wave_data_left[0]),
        .wave_data = wave_data_left,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_NO
    {
        .sample_label = AUDIO_NAME_NO,
        .sample_count = sizeof(wave_data_no)/sizeof(wave_data_no[9]),
        .wave_data = wave_data_no,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_OFF
    {   /* sample is shifted too far to the end so no valid window is detected */
        .sample_label = AUDIO_NAME_OFF,
        .sample_count = sizeof(wave_data_off)/sizeof(wave_data_off[9]),
        .wave_data = wave_data_off,
    },
#elif AUDIO_SAMPLE_GROUP==AUDIO_SAMPLE_GROUP_YES
    {
        .sample_label = AUDIO_NAME_YES,
        .sample_count = sizeof(wave_data_yes)/sizeof(wave_data_yes[9]),
        .wave_data = wave_data_yes,
    },
#endif
};

/*
 * END GRNN TEST VECTORS
 *
 */
#define USE_WAVE_DATA
#ifndef USE_WAVE_DATA
/*
 * #include "test_audio/logOutputleft.h"
 * #include "test_audio/logOutputno.h"
 * #include "test_audio/logOutputgo.h"
 * #include "test_audio/logOutputdown.h"
 */
#include "test_audio/logOutputno.h"
#endif


/*
 * Formats a string and prints it using AxonHostLog()
 */
void AxonPrintf(char *fmt_string, ...) {
  va_list arg_list;

   // get the variable arg list
   va_start(arg_list,fmt_string);
   // pass it to snprintf
   vsnprintf(gl_axon_instance->host_provided.log_buffer, gl_axon_instance->host_provided.log_buffer_size, fmt_string, arg_list);
   va_end(arg_list);

   // now print it
   AxonHostLog(gl_axon_instance, gl_axon_instance->host_provided.log_buffer);
}

/*
 * macros for handling circular buffer wrap. Audio features are stored
 * in a circular buffer.
 */
/*
 * Increments a circular buffer index, wrapping when it reaches the end.
 */
#define AXON_AUDIO_FEATURES_NEXT_NDX(ndx, buffer_length) ((ndx)+1 < buffer_length ? (ndx)+1 : 0)

/*
 * Backs up a circular buffer index.
 */
#define AXON_AUDIO_FEATURES_BACK_UP(from_ndx, by_how_much, buffer_length) \
     (buffer_length<=(by_how_much) ?  (from_ndx) : \
      (from_ndx) >= (by_how_much) ? (from_ndx) - (by_how_much) :\
          buffer_length + (from_ndx)-(by_how_much))



/*
 * enum for each of the states when AxonML is running in asynchronous mode.
 */
typedef enum {
  kAxonMlAsyncStateIdle, // doing nothing, ready for 1st audio frame.
  kAxonMlAsyncStateFeatureCalc, // calculating audio features.
  kAxonMlAsyncStateFeatureWaitForAudio, // waiting for audio to calc next feature with.
  kAxonMlAsyncStateInference, // performing inference
  kAxonMlAsyncStateComplete,       // classification is complete.
}AxonMlAsyncStateEnum;

/*
 * structure to keep all the state info in one place.
 * Note that it is not in retained memory; therefore each "1st frame" will
 * reset it, and the continuing inference/audio processing must prevent deep sleep
 * until it is complete.
 */
struct {
  // circular buffer for holding most recently calculated audio features.
  AudioInputFeatureType audio_features[AXON_AUDIO_FEATURES_SLICE_CNT][AUDIO_INPUT_FEATURE_HEIGHT];
  // index in the circular buffer to place the next calculated audio feature.
  uint32_t audio_featues_buf_head_ndx;

  uint32_t audio_features_elapsed_time;
  uint32_t nn_elapsed_time;
  uint32_t nn_final_elapsed_time;
  uint32_t total_test_start;
  uint32_t total_test_end;
  uint32_t total_classifications;
  uint32_t total_nn_slices;
  uint32_t total_foreground_periods;
  uint8_t bgfg_window_width;
  struct {
    int32_t score;
    int16_t classification;
    char *label;
  } output_score;

  volatile AxonMlAsyncStateEnum ml_async_state;
  KwsFirstOrLastAudioFrame first_or_last_frame;
  uint32_t start_time;
  AxonResultEnum result;
  KwsClassifyOptionEnum classify_option;
  uint32_t nn_frame_ndx;
  uint32_t nn_audio_features_frame_ndx; // index into audio features circular buffer

} axon_nn_state_info;


#if AXON_NN_TYPE==AXON_FC4

#endif



/*
 * Called when a final classification completes.
 */
static void process_final_classification_complete(AxonResultEnum result) {

  if (kAxonResultSuccess > axon_nn_state_info.result) {
    AxonPrintf(gl_axon_instance, "kws inference failed! %d \r\n", axon_nn_state_info.result);
    return;
  }

  /*
   * get the results.
   */
  axon_nn_state_info.output_score.classification =
      AxonKwsModelGetClassification(&axon_nn_state_info.output_score.score, &axon_nn_state_info.output_score.label);

  axon_nn_state_info.nn_final_elapsed_time += (AxonHostGetTime()-axon_nn_state_info.start_time);
  axon_nn_state_info.total_classifications++;


  axon_nn_state_info.ml_async_state = kAxonMlAsyncStateComplete;

  AxonMlDemoHostClassifyingEnd(0);
}

/*
 * Implemented by the host to return 1 slice of audio features.
 * slice_ndx is from the start of the audio window.
 */
int AxonKwsHostGetNextAudioFeatureSlice(const AudioInputFeatureType **audio_features_in) {
  if (NULL==audio_features_in) {
    return -2;
  }

  if (++axon_nn_state_info.nn_frame_ndx > (axon_nn_state_info.bgfg_window_width)) {
    // ASKED FOR TOO MANY!!!
    return -1;
  }
  // nn_audio_features_frame_ndx is the index into our circular buffer of features.
  *audio_features_in = &axon_nn_state_info.audio_features[axon_nn_state_info.nn_audio_features_frame_ndx][0];

  // move to next audio_features in circular buffer
  axon_nn_state_info.nn_audio_features_frame_ndx = AXON_AUDIO_FEATURES_NEXT_NDX(axon_nn_state_info.nn_audio_features_frame_ndx,AXON_AUDIO_FEATURES_SLICE_CNT);

  return 0;
}

/* NOT USING THIS AS WE ARE USING FIXED VALUES FOR THE LSTM MODEL
 * used to get the max and min from the audio features so that the input can be normalized for the LSTM based models
void AxonKwsHostGetMaxMinAudioFeatures(int32_t *max_value, int32_t *min_value){//FIXME the size of the inputs

  //FIXME if the model is not using the whole length of the window size, this may need to change
  *max_value = INT32_MIN;
  *min_value = INT32_MAX;
  for(uint8_t slice_ndx=0;slice_ndx<AXON_AUDIO_FEATURES_SLICE_CNT;slice_ndx++){
    for(uint8_t feature_ndx=0;feature_ndx<AUDIO_INPUT_FEATURE_HEIGHT;feature_ndx++){
      if((*max_value)<axon_nn_state_info.audio_features[slice_ndx][feature_ndx]){
        (*max_value) = axon_nn_state_info.audio_features[slice_ndx][feature_ndx];
      }

      if((*min_value)>axon_nn_state_info.audio_features[slice_ndx][feature_ndx]){
        (*min_value) = axon_nn_state_info.audio_features[slice_ndx][feature_ndx];
      }
    }
  }
}
*/

/*
 * Starts the classification process.
 *
 * In async mode, returns after the 1st grnn slice calculation is started. It is up to the
 * ISR to finish the job.
 *
 * In sync mode, provides a loop to process each grnn slice. Daisy-chaining the start of the next
 * slice to the end of the current slice would result in excessive stack use.
 *
 *
 */
static void classify_window_start() {
  axon_nn_state_info.start_time = AxonHostGetTime();

  axon_nn_state_info.total_foreground_periods++;

  /*
   * Note, the current audio_featues_buf_head_ndx is the oldest feature, not the newest.
   */
  axon_nn_state_info.nn_audio_features_frame_ndx =
      AXON_AUDIO_FEATURES_BACK_UP(axon_nn_state_info.audio_featues_buf_head_ndx,(axon_nn_state_info.bgfg_window_width), AXON_AUDIO_FEATURES_SLICE_CNT);

  // this counts how many frames have been processed .
  axon_nn_state_info.nn_frame_ndx=0;

  AxonKwsModelInfer(axon_nn_state_info.bgfg_window_width);
}

/*
 * Called synchronously and asynchronously when the audio feature calculation has completed.
 */
static void process_feature_complete(AxonResultEnum result) {
  // increment audio_features circular buffer index
  axon_nn_state_info.audio_featues_buf_head_ndx = AXON_AUDIO_FEATURES_NEXT_NDX(axon_nn_state_info.audio_featues_buf_head_ndx, AXON_AUDIO_FEATURES_SLICE_CNT);
  axon_nn_state_info.audio_features_elapsed_time += AxonHostGetTime()-axon_nn_state_info.start_time;

  // debug - print bg/fg stats to see sample energy
  // AxonBgFgPrintStats();

  if (( axon_nn_state_info.classify_option>=kDoClassify) || // caller wants classify performed no matter what
      (( axon_nn_state_info.classify_option==kClassifyOnValidWindow) &&
       (axon_nn_state_info.bgfg_window_width = AxonAudioFeaturesBgFgWindowWidth())) ) { // ...or caller wants us to have a valid window (and we do)
    // user can specify how many frames to classify on by adding that value to classify_option
    if (axon_nn_state_info.classify_option>kDoClassify) {
      axon_nn_state_info.bgfg_window_width = axon_nn_state_info.classify_option-kDoClassify;
    }

    // if there was no early detect of the window start, alert now.
    // tell the caller that classification has started (and we'll be awhile)
    AxonMlDemoHostClassifyingStart(AxonAudioFeaturesBgFgWindowFirstFrame(), AxonAudioFeaturesBgFgWindowWidth());

    classify_window_start(axon_nn_state_info.bgfg_window_width);
  } else {
    //turn off Axon Clk and Power
    AxonMlDemoHostAxonSetEnabled(kAxonBoolFalse);

    if (axon_nn_state_info.first_or_last_frame==kLastFrame) {
      // no more frames are coming. cancel it.
      axon_nn_state_info.ml_async_state =  kAxonMlAsyncStateComplete;
      // tell the host that no valid window was found.
      AxonMlDemoHostNoClassification();
    } else {
      // wait for the next audio frame.
      axon_nn_state_info.ml_async_state = kAxonMlAsyncStateFeatureWaitForAudio;
    }
  }

}

int AxonKwsLastFrameWasForeground() {
  // AxonBgFgPrintStats();
  return AxonAudioFeaturesBgSliceIsForeground() > 0;
}


/*
 * called frame-by-frame to process an audio stream.
 */
int AxonKwsProcessFrame(
    const int16_t *raw_input_ping,
    uint32_t ping_count,
    const int16_t *raw_input_pong,
    uint8_t input_stride,
    KwsFirstOrLastAudioFrame first_or_last_frame,
    KwsClassifyOptionEnum classify_option) {

  AxonResultEnum result;
  /*
   * make sure all previous processing has completed.
   * to submit a frame the state must either be idle (and this is the 1st fame)
   * or waiting for frame and this is not the first frame).
   */
  if (!(((axon_nn_state_info.ml_async_state == kAxonMlAsyncStateIdle) && (first_or_last_frame==kFirstFrame)) ||
      (((first_or_last_frame!=kFirstFrame)) &&(axon_nn_state_info.ml_async_state == kAxonMlAsyncStateFeatureWaitForAudio)))) {
    return -1000;
  }
  //turn on Axon Clk and Power
  AxonMlDemoHostAxonSetEnabled(kAxonBoolTrue);
  if (kFirstFrame==first_or_last_frame) {
    AxonAudioFeaturesRestart();

    memset(&axon_nn_state_info, 0, sizeof(axon_nn_state_info));
    axon_nn_state_info.total_test_start =  AxonHostGetTime();
  }

  axon_nn_state_info.ml_async_state = kAxonMlAsyncStateFeatureCalc;
  axon_nn_state_info.classify_option = classify_option;
  axon_nn_state_info.first_or_last_frame = first_or_last_frame;
  /*
   * every time we'll process 1 audio_features...
   */
  axon_nn_state_info.start_time = AxonHostGetTime();

  /*
   * calculate audio features
   */
  result=AxonAudioFeatureProcessFrame(raw_input_ping, ping_count,
              raw_input_pong, kLastFrame==first_or_last_frame, input_stride,
              &axon_nn_state_info.audio_features[axon_nn_state_info.audio_featues_buf_head_ndx][0]
              );

  return result;
}

/*
 * Places state machine back in the idle state, ready to start a new session.
 * Can only be called when in WaitingForAudio or Complete states.
 */
int AxonKwsClearLastResult(const char **result_label) {

  if ((axon_nn_state_info.ml_async_state==kAxonMlAsyncStateFeatureWaitForAudio) ||
      (axon_nn_state_info.ml_async_state==kAxonMlAsyncStateComplete) ||
      (axon_nn_state_info.ml_async_state==kAxonMlAsyncStateIdle)) {
    axon_nn_state_info.ml_async_state = kAxonMlAsyncStateIdle;
    if (result_label != NULL) {
      *result_label = axon_nn_state_info.output_score.label;
    }
    return axon_nn_state_info.output_score.classification;
  }
  return axon_nn_state_info.output_score.classification;
}


#ifndef USE_WAVE_DATA
/*
 * This function performs the inference using supplied audio features; it skips calculating
 * audio features from raw audio.
 */
static void AxonKwsProcessFeatures() {
  uint32_t frame_ndx;
  AxonResultEnum result;
  uint32_t start_time; // for profiling
  uint32_t end_time;   // for profiling

  AxonMel32Restart();
  memset(&axon_nn_state_info, 0, sizeof(axon_nn_state_info));
  axon_nn_state_info.total_test_start =  AxonHostGetTime();
  AxonKwsModelRestart();

  // now run nn on all the frames
  for (frame_ndx=0; frame_ndx<CANNED_AUDIO_FEATURES_COUNT;frame_ndx++) {

    start_time = AxonHostGetTime();
    if (kAxonResultSuccess >
        (result = AXON_NN_PROCESS_FRAME(gl_axon_instance,
            &audio_features[frame_ndx][0],
            frame_ndx))) {
      AxonHostLog(gl_axon_instance, "AXON_NN_PROCESS_FRAME: failed!\r\n");
      break;
    }
    end_time = AxonHostGetTime();
    axon_nn_state_info.nn_elapsed_time += (end_time-start_time);
    axon_nn_state_info.total_nn_slices++;
  }

  start_time = AxonHostGetTime();
  // track total nn frames processed
  result = AXON_NN_CALCULATE_RESULTS(gl_axon_instance,
                    &axon_nn_state_info.output_scores[0].classification,
                    &axon_nn_state_info.final_scores,
                    &axon_nn_state_info.output_scores[0].margin);
  axon_nn_state_info.output_scores[0].score = axon_nn_state_info.final_scores[axon_nn_state_info.output_scores[0].classification];
  end_time = AxonHostGetTime();
  axon_nn_state_info.nn_final_elapsed_time += (end_time-start_time);
  axon_nn_state_info.total_classifications++;


  if (kAxonResultSuccess > result) {
    AxonHostLog(gl_axon_instance, "AXON_NN_CALCULATE_RESULTS failed!\r\n");
    return;
  }
  snprintf(axon_nn_state_info.printbuffer, sizeof(axon_nn_state_info.printbuffer), "Classified as %d,%s\r\n",
      axon_nn_state_info.output_scores[0].classification, output_classes[axon_nn_state_info.output_scores[0].classification].class_name);
  AxonHostLog(gl_axon_instance, axon_nn_state_info.printbuffer);
}
#endif

static void AxonKwsPrintStats() {

  axon_nn_state_info.total_test_end = AxonHostGetTime();

  axon_printf(gl_axon_instance, "Total elapsed: %u, VAD: %u, audio_features: %u, nn: %u, nn: result %u\r\n",
      axon_nn_state_info.total_test_end-axon_nn_state_info.total_test_start,
      AxonAudioFeaturesBgFgExecutionTicks(),
      axon_nn_state_info.audio_features_elapsed_time-AxonAudioFeaturesBgFgExecutionTicks(),
      axon_nn_state_info.nn_elapsed_time,
      axon_nn_state_info.nn_final_elapsed_time );
}

/*
 * This is the basic demo Top-level function that processes a canned audio stream, start to finish.
 */
static int AxonKwsClassifyAudio(const int16_t *audio_samples, uint32_t audio_sample_count, uint8_t input_stride) {
  AxonResultEnum result = kAxonResultSuccess;
  uint32_t frame_idx;

#ifndef USE_WAVE_DATA
  /*
   * using canned, pre-calculated audio features.
   */
  AxonKwsProcessFeatures();
  return 0;
#endif
  /*
   * loop through the sample set, one frame at a time.
   */
  for (frame_idx=0;frame_idx<((audio_sample_count/AXON_AUDIO_FEATURE_FRAME_SHIFT)-1);frame_idx++) {

    while(1) {
      AxonHostDisableInterrupts();
      if ((axon_nn_state_info.ml_async_state != kAxonMlAsyncStateIdle) &&
          (axon_nn_state_info.ml_async_state != kAxonMlAsyncStateFeatureWaitForAudio) &&
          (axon_nn_state_info.ml_async_state != kAxonMlAsyncStateComplete)){

        AxonHostWfi();
        AxonHostEnableInterrupts();
      } else {
        AxonHostEnableInterrupts();
        break;
      }
    }
    // if that frame finished it, stop here.
    if(axon_nn_state_info.ml_async_state == kAxonMlAsyncStateComplete) {
      break;
    }

    if (kAxonResultSuccess > (result=AxonKwsProcessFrame(audio_samples, AXON_AUDIO_FEATURE_FRAME_LEN, NULL, input_stride,
        frame_idx==0 ? kFirstFrame : frame_idx == (audio_sample_count/AXON_AUDIO_FEATURE_FRAME_SHIFT - 2)? kLastFrame : kMiddleFrame,
        kClassifyOnValidWindow))) {
      break;
    }
    audio_samples += AXON_AUDIO_FEATURE_FRAME_SHIFT*input_stride;


  }
  /*
   * wait for it to complete
   */
  while((axon_nn_state_info.ml_async_state != kAxonMlAsyncStateComplete) &&
      (axon_nn_state_info.ml_async_state != kAxonMlAsyncStateFeatureWaitForAudio) &&
      (axon_nn_state_info.ml_async_state != kAxonMlAsyncStateIdle)){
    AxonHostDisableInterrupts();
    AxonHostWfi();
    AxonHostEnableInterrupts();
  }


  AxonKwsPrintStats();

  AxonKwsClearLastResult(NULL);

  return result;
}

/*
 * tag these as extern so the symbols don't get stripped out prior to linking.
 */
extern int32_t wave_data_length;
extern const int16_t *wave_data_playback;

int32_t wave_data_length;
const int16_t *wave_data_playback;

/*
 *
 */
int AxonDemoPrepare(void *unused) {
  static int32_t prepare_result = -1;
  uint8_t bgfg_window_slice_cnt;
  AxonAudioFeatureVariantsEnum which_variant;
  int32_t *normalization_means_q11p12;
  int32_t *normalization_inv_std_devs;
  uint8_t normalization_inv_std_devs_q_factor;
  int32_t quantization_inv_scale_factor;
  uint8_t quantization_inv_scale_factor_q_factor;
  int8_t quantization_zero_point;
  AxonDataWidthEnum output_saturation_packing_width;

  axon_nn_state_info.ml_async_state = kAxonMlAsyncStateIdle;
  /*
   * prepare Axon for MFCC and nn operations.
   * Get the audio feature input requirements from the model.
   */
  AxonKwsModelGetInputAttributes(
      &bgfg_window_slice_cnt,
      &which_variant,
      &normalization_means_q11p12,
      &normalization_inv_std_devs,
      &normalization_inv_std_devs_q_factor,
      &quantization_inv_scale_factor,
      &quantization_inv_scale_factor_q_factor,
      &quantization_zero_point,
      &output_saturation_packing_width);

  if (kAxonResultSuccess > (prepare_result=AxonAudioFeaturePrepare(
      gl_axon_instance,
      process_feature_complete,
      bgfg_window_slice_cnt,
      which_variant,
      normalization_means_q11p12,
      normalization_inv_std_devs,
      normalization_inv_std_devs_q_factor,
      quantization_inv_scale_factor,
      quantization_inv_scale_factor_q_factor,
      quantization_zero_point,
      output_saturation_packing_width))) {
    AxonPrintf("AxonAudioFeaturePrepare: failed! %d\r\n", prepare_result);
  }

  if (kAxonResultSuccess > (prepare_result=AxonKwsModelPrepare(gl_axon_instance, process_final_classification_complete))) {
    AxonPrintf("AxonKwsModelPrepare: failed! %d\r\n", prepare_result);
  }
  while(0 > prepare_result); // just hang here.
  wave_data_length = audio_sample_files[0].sample_count;
  wave_data_playback = audio_sample_files[0].wave_data;

  AxonHostLog(gl_axon_instance, "AxonDemoPrepared\r\n");
  return prepare_result;

}

int AxonDemoRun(void *unused1, uint8_t unused2) {

  uint8_t audio_sample_ndx;
  for(audio_sample_ndx=0;
      audio_sample_ndx < sizeof(audio_sample_files)/sizeof(audio_sample_files[0]);
      audio_sample_ndx++) {
    AxonHostLog(gl_axon_instance, "\r\n\r\n");
    AxonHostLog(gl_axon_instance, audio_sample_files[audio_sample_ndx].sample_label);
    AxonKwsClassifyAudio(audio_sample_files[audio_sample_ndx].wave_data, // samples
        audio_sample_files[audio_sample_ndx].sample_count,  // # of samples
        1);   // Stride between samples. For mono this is 1, for stereo it's 2
  }

  return 0;
}
