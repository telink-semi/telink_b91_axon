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

#define FC4_INPUT_SLICES (61)
#define FC4_INPUT_LENGTH (FC4_INPUT_SLICES*MFCC_FEATURE_COUNT)
#define FC4_MIDLAYER_LENGTH (144)
#define FC4_OUTPUT_LENGTH (12)
#define FC4_MIN_IO_BUFFER_LENGTH (FC4_INPUT_LENGTH << 2 < FC4_MIDLAYER_LENGTH ? FC4_MIDLAYER_LENGTH : FC4_INPUT_LENGTH << 2)

#define AUDIO_INPUT_FEATURE_HEIGHT MFCC_FEATURE_COUNT
typedef int32_t axon_kws_inference_output_type;

#define AXON_AUDIO_FEATURES_SLICE_CNT FC4_INPUT_SLICES

/*
 * FC4 expects int8 input.
 */
typedef int8_t AudioInputFeatureType;
#define AXON_AUDIO_FEATURES_DATA_WIDTH kAxonDataWidth8

/*
 * Generic KWS model function to get input feature information.
 */
AxonResultEnum AxonKwsModelFc4GetInputAttributes(
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
AxonResultEnum AxonKwsModelFc4Prepare(void *axon_handle, void (*result_callback_function)(AxonResultEnum result));

AxonResultEnum AxonKwsModelFc4Infer(uint8_t window_width);

/*
 * Implemented by the host to return 1 slice of audio features.
 */
extern int AxonKwsHostGetNextAudioFeatureSlice(const AudioInputFeatureType **audio_features_in);

/**
 * Called after a classification has been performed, finds the maximum value in the buffer
 * and returns its index.
 */
uint8_t AxonKwsModelFc4GetClassification(int32_t *score, char **label);

#ifdef __cplusplus
} // extern "C" {
#endif

