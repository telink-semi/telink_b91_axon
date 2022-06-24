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
/**
 * This GRNN for keyword spotting has 2 parts. The 1st part calculates the "hidden" vector based on the
 * current set of audio features (mel32's) and the previous hidden vector.
 * This gets invoked until the entire audio sample has been processed.
 * After the last audio frame is processed, the hidden vector is passed to the 2nd part which does the
 * actual classification.
 */
#pragma once
#include "axon_audio_features_api.h"
#include <assert.h>


AxonResultEnum AxonKwsModelGrnnPrepare(void *axon_handle, void (*result_callback_function)(AxonResultEnum result));
AxonResultEnum AxonKwsModelGrnnGetInputAttributes(
    uint8_t *bgfg_window_slice_cnt,            /**< valid window width for background/foreground detection */
    AxonAudioFeatureVariantsEnum *which_variant, /**< specify which variant to produce */
    int32_t **normalization_means_q11p12,      /**< normalization subtracts means (as q11.12)... */
    int32_t **normalization_inv_std_devs,      /**< ...then multiplies by the inverse std deviation... */
    uint8_t *normalization_inv_std_devs_q_factor,  /**< ...then right shifts by the inverse std deviation q-factor */
    int32_t *quantization_inv_scale_factor,    /**< quantization multiplies by the inverse scaling factor... */
    uint8_t *quantization_inv_scale_factor_q_factor, /** ...then right shifts by the inverse scaling factor q-factor... */
    int8_t *quantization_zero_point,          /**< ...then adds the zero point */
    AxonDataWidthEnum *output_saturation_packing_width ); /**< final output will be saturated/packed to this width (24 does nothing) */



typedef int16_t AudioInputFeatureType;
#define AUDIO_INPUT_FEATURE_HEIGHT MEL32_FEATURE_COUNT
#define AXON_AUDIO_FEATURES_SLICE_CNT  61

#define OUTPUT_CLASS_COUNT GRNN_CLASS_COUNT

/*
 * Implemented by the host to return 1 slice of audio features.
 */
extern int AxonKwsHostGetNextAudioFeatureSlice(const AudioInputFeatureType **audio_features_in);
/*
 * Performs slice and results inference. Calls  AxonGrnnHostGetSlice() slice_count times to get the slice data,
 */
AxonResultEnum AxonKwsModelGrnnInfer(uint8_t slice_count);

uint8_t AxonKwsModelGrnnGetClassification(int32_t *score, char **label);
