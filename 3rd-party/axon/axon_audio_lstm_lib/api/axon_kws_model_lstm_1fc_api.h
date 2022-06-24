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

#include <assert.h>
#include "axon_api.h"
#include "axon_audio_features_api.h"


#ifdef __cplusplus
extern "C" {
#endif

#define LSTM_1FC_INPUT_SLICES   (61)  
#define LSTM_1FC_INPUT_LENGTH   (100 + MFCC_FEATURE_COUNT) // the sum of the size of the input features and the hidden vectors length
#define LSTM_1FC_OUTPUT_LENGTH  (12)

#define AUDIO_INPUT_FEATURE_HEIGHT MFCC_FEATURE_COUNT
typedef int32_t axon_kws_inference_output_type;

#define AXON_AUDIO_FEATURES_SLICE_CNT LSTM_1FC_INPUT_SLICES

/*
 * LSTM_1FC expects int8 input.
 * NOTE : the input datawidth is int32_t, the values are then normalized into int8 and packed by the LSTM
 */
typedef int32_t AudioInputFeatureType;
#define AXON_AUDIO_FEATURES_DATA_WIDTH kAxonDataWidth24

/*
 * Generic KWS model function to get input feature information.
 */
AxonResultEnum AxonKwsModelLstm1fcGetInputAttributes(
    uint8_t *bgfg_window_slice_cnt,            /**< valid window width for background/foreground detection */
    AxonAudioFeatureVariantsEnum *which_variant, /**< specify which variant to produce */
    int32_t **normalization_means_q11p12,      /**< normalization subtracts means (as q11.12)... */
    int32_t **normalization_inv_std_devs,      /**< ...then multiplies by the inverse std deviation... */
    uint8_t *normalization_inv_std_devs_q_factor,  /**< ...then right shifts by the inverse std deviation q-factor */
    int32_t *quantization_inv_scale_factor,    /**< quantization multiplies by the inverse scaling factor... */
    uint8_t *quantization_inv_scale_factor_q_factor, /** ...then right shifts by the inverse scaling factor q-factor... */
    int8_t *quantization_zero_point,          /**< ...then adds the zero point */
    AxonDataWidthEnum *output_saturation_packing_width ); /**< final output will be saturated/packed to this width (24 does nothing) */


/*
 * Generic KWS model prepare function.
 * Library declares private buffers to manage state information.
 */
AxonResultEnum AxonKwsModelLstm1fcPrepare(void *axon_handle, void (*result_callback_function)(AxonResultEnum result));

/*
 * Performs slice and results inference.
 */
AxonResultEnum AxonKwsModelLstm1fcInfer(uint8_t slice_count);

/*
 * Implemented by the host to return 1 slice of audio features.
 */
extern int AxonKwsHostGetNextAudioFeatureSlice(const AudioInputFeatureType **audio_features_in);

/*
 * Implemented by the host to return the max and minimum of the audio features.
 * NOTE : Not being used, but keeping the definition if we need something similar in the future
 *
 * extern void AxonKwsHostGetMaxMinAudioFeatures(int32_t *max_value, int32_t *min_value);
 */
 
/**
 * Called after a classification has been performed, finds the maximum value in the buffer
 * and returns its index.
 */
uint8_t AxonKwsModelLstm1fcGetClassification(int32_t *score, char **label);
