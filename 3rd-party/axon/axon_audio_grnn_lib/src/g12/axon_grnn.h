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
#include "axon_grnn_api.h"
#include <assert.h>

/*
 * number of input frames that are calculated before performing the final classification.
 */
#define GRNN_INPUT_WIDTH 61

/*
 * Input is 1x32, fully-connected w/ weights 32x100 to produce a 1x100
 * produce a 1x100
 */
#define GRNN_INPUT_HT 32  // input is 1x32

static_assert(GRNN_INPUT_HT==MEL32_FEATURE_COUNT, "Mismatch between GRNN input height and audio features height");
#define GRNN_INPUT_WT_HT 100  // input weights are 32x100


/*
 * hidden is 1x100, fully-connected w/ weights 100x100 to
 * produce a 1x100 that is added to the input FC output so the heights must match.
 */
#define GRNN_HIDDEN_HT 100 // hidden state is 1x100
#define GRNN_HIDDEN_WT_HT GRNN_INPUT_WT_HT // hidden weights  100x100, has to match input wt height

/*
 * Final fully connected must be the same width as the height of the input/hidden FC outputs.
 */
#define GRNN_CLASS_COUNT 12
#define GRNN_FINAL_FC_HEIGHT GRNN_CLASS_COUNT
#define GRNN_FINAL_FC_WIDTH GRNN_INPUT_WT_HT

typedef int16_t grnn_weight_type;
typedef int8_t grnn_weight_flash_type;

extern const int32_t GRNN_INPUT_NORMALIZATION_MEANS_11Q12[];
extern const int32_t GRNN_INPUT_NORMALIZATION_INV_STDS_0Q7[];

#define GRNN_INPUT_NORMALIZATION_INV_STDS_ROUNDING 8

#define GRNN_8_BIT_SYMMETRIC_QUANT  1

#if GRNN_8_BIT_SYMMETRIC_QUANT
  #define GRNN_FINAL_FC_WEIGHT_RIGHT_SHIFT 6
#else
  #define GRNN_FINAL_FC_WEIGHT_RIGHT_SHIFT 10
#endif


extern const int32_t GRNN_CLASSIFICATION_UNKNOWN_NDX;

