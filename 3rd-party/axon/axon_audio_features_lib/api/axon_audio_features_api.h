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

/**
 * Audio Feature Extraction
 *
 * Axon Audio Features supports several build variations to produce different versions of audio features.
 *
 * All variations have the following in common:
 * 1) Audio input is 512 samples of 16bit at 16000fps. This is 32ms of audio samples. The input can be supplied in 2 buffers (to support
 *    ping-pong audio buffer management).
 *
 * 2) A hamming window is applied to the 512 samples. Hamming window formula is
 *                Coefficient kn (n=0..511) = 0.54-0.46*COS(2*PI*n/511)
 *
 * 3) 512tap, complex FFT is computed (imaginary values of input are initialized to 0).
 *
 * 4) The power of the FFT is computed and rounded. Power of each tap is the sum of the real and imaginary parts squared.
 *
 *
 * The results from here diverge depending on options
 *
 * Variant "A": Mel32s (kAxonAudioFeatureMel32)
 * Mel32s do not apply a DCT.
 *
 *   5a) Mel frequency filter banks are applied to the FFT power. There are 32 filter taps spaced evenly between 0hz and 8khz.
 *    The taps are centered on the nearest FFT tap to the corresponding Mel frequency.
 *
 *   6a) Natural Log is applied to the output of the filter banks.
 *
 *   The result is a vector of 32 24-bit values in q11.12 format.
 *
 * Variant "B": Plain MFCCs (kAxonAudioFeatureMfccOrtho)
 *   In this variant, the MFCC is simply the Mel32 with a DCT applied. The DCT is 10x32, coefficients match the SciPy DCT type 2 with "ortho".
 *
 *   7b) Perform the DCT on the ln() values. The resulting 10 coefficients are the MFCCs
 *
 *   The result is a vector of 10 24-bit values in q11.12 format.
 *
 * Variant "C": MFCCs with energy append (kAxonAudioFeatureMfccOrthoEnergyAppend)
 *   Same as variant B but replace coefficient 0 with the log of the FFT energy.
 *
 *   5c) Calculate the FFT energy by summing the FFT power coefficients calculated in step 4).
 *
 *   6c) Apply the same filter banks to the FFT power (but not the energy).
 *
 *   7c) Take the natural log of the filter bank outputs AND the FFT energy.
 *
 *   8c) Perform the DCT on the filter bank log values.
 *
 *   9c) Substitute the log of the FFT energy into coefficient 0.
 *
 *   The result is a vector of 10 24-bit values in q11.12 format.
 *
 * Variant "D": MFCCs using FFT magnitude instead of MFCC power
 *
 *   This is identical to Variant "B" except...
 *
 *   5d) Prior to filter banks, take the square root of the FFT power.
 *
 *   6d) Apply filter banks.
 *
 *   7d) Take natural log.
 *
 *   8d) Apply DCT
 *
 *   The result is a vector of 10 24-bit values in q11.12 format.
 *
 * Batch Normalization Option
 *
 *
 * API functions are:
 *
 * AxonMel32Prepare()
 * Called once at start-up to pre-define all the axon operations that comprise Mel32 feature calculation. Note: this also prepare the
 * background/foreground volume algorithm.
 *
 * AxonMel32Prepare()
 * Called at the start of each new distinct audio recording session.
 *
 * AxonMel32ProcessFrame()
 * Called every 16ms with 32ms slice of audio. This will calculate both the audio features as well as the background/foreground
 * for the slice.
 */

/*
 * Basic audio parameters:
 * - 512 samples @ 16000FPS, striding by 256 samples (32ms slice width, 16ms stride)
 * - 32 Filterbanks spread between 0 and 8000 FPS
 */
#define AXON_AUDIO_FEATURE_FRAME_LEN 512       //32ms samples per frame, 512=16*32
#define AXON_AUDIO_FEATURE_FRAME_SHIFT 256     //stride 16ms=256 samples
#define AXON_AUDIO_FEATURE_SAMPLE_RATE 16000
#define AXON_AUDIO_FEATURE_HIGH_FREQUENCY 8000  // mel frequency bins go up 8kfps
#define AXON_AUDIO_FEATURE_FILTERBANK_COUNT 32

/*
 * Mel32 parameters
 */
#define MEL32_OUTPUT_Q_FORMAT 12 // final outputs have 12 bits of fractional precision.
#define MEL32_FEATURE_COUNT   AXON_AUDIO_FEATURE_FILTERBANK_COUNT // mel32 is the output of the spectrogram which has 32 bins.

#define MFCC_FEATURE_COUNT 10

/*
 * Background/Foreground audio energy detection parameters
 */

/*
 * Define the background/foreground detection parameters.
 */
#define VOICE_WINDOW_TYPE FIXED_LENGTH_WINDOW_TYPE
/*
 * A valid window is
 * - Fixed length
 * - has exactly LONG_BACKGROUND_LENGTH background frames to start (there can be background frames before these, but they are not included in the window
 * - has the specified min/max number of long/short foregrounds and short backgrounds
 * - ends with at least LONG_BACKGROUND_LENGTH frames
 */
# define LONG_BACKGROUND_LENGTH  8     // min number of consecutive background frames to qualify as long background
# define LONG_FORGROUND_MIN_LENGTH  8  // min number of consecutive foreground frames to qualify as long foreground
# define WINDOW_MAX_LONG_FOREGROUNDS  2
# define WINDOW_MIN_LONG_FOREGROUNDS  1
# define WINDOW_MAX_SHORT_FOREGROUNDS  2
# define WINDOW_MIN_SHORT_FOREGROUNDS  0

typedef enum {
  kAxonAudioFeatureMel32,
  kAxonAudioFeatureMfccOrtho,
  kAxonAudioFeatureMfccOrthoEnergyAppend,
  kAxonAudioFeatureMfccFftMagOrtho,
} AxonAudioFeatureVariantsEnum;

/*
 * Call this once at start-up. The axon operations will be defined and stored in static state variables.
 * The callback function will be invoked upon completion of a single frame.
 *
 * NOTE: normalization used with append energy requires that element 32 in the inv_std_devs and
 *      means be the value used for the energy coefficient.
 */
AxonResultEnum AxonAudioFeaturePrepare(
    void *axon_handle,
    void (*callback_function)(AxonResultEnum result),
    uint8_t bgfg_window_slice_cnt,            /**< valid window width for background/foreground detection */
    AxonAudioFeatureVariantsEnum which_variant, /**< specify which variant to produce */
    int32_t *normalization_means_q11p12,      /**< normalization subtracts means (as q11.12)... */
    int32_t *normalization_inv_std_devs,      /**< ...then multiplies by the inverse std deviation... */
    uint8_t normalization_inv_std_devs_q_factor,  /**< ...then right shifts by the inverse std deviation q-factor */
    int32_t quantization_inv_scale_factor,    /**< quantization multiplies by the inverse scaling factor... */
    uint8_t quantization_inv_scale_factor_q_factor, /** ...then right shifts by the inverse scaling factor q-factor... */
    int8_t quantization_zero_point,          /**< ...then adds the zero point */
    AxonDataWidthEnum output_saturation_packing_width ); /**< final output will be saturated/packed to this width (24 does nothing) */

/*
 * Call this at the beginning of a new audio stream to clear out the memory of
 * the last stream.
 */
void AxonAudioFeaturesRestart();

/*
 * Call this once per audio slice. Calculates the background/foreground and mel32 audio features.
 * Audio samples can be submitted in 2 discontiguous buffers, raw_input_ping and raw_input_pong.
 * raw_input_ping contains the 1st ping_count samples, and raw_input_pong contains the remaining
 * (MEL32_AUDIO_FRAME_LEN-ping_count) samples.
 * Note the pong can be null if ping has the entire window (ping_count==MEL32_AUDIO_FRAME_LEN).
 *
 * @returns
 * - In async mode, a result > 0 indicates that AxonMel32ProcessFrameAsync() needs to be called
 *   from the next axon interrupt.
 * - Otherwise, result is an AxonResultEnum.
 */
AxonResultEnum AxonAudioFeatureProcessFrame(
    const int16_t *raw_input_ping,
    uint32_t ping_count,
    const int16_t *raw_input_pong,
    AxonBoolEnum last_frame, /**< if true, last frame BG/FG exception occurs */
    uint8_t input_stride,
    void *output_buffer /**< store the output vector in the format specified in the prepare argument output_saturation_packing_width */
    );

/*
 * Returns the width of the current audio window that meets the window
 * requirements. Returns 0 if there is no valid window that includes the most
 * recent audio slice.
 */
uint8_t AxonAudioFeaturesBgFgWindowWidth();

/*
 * returns the index of the 1st frame in the valid window. The index counts
 * from the last time AxonAudioFeaturesRestart() was called.
 */
uint32_t AxonAudioFeaturesBgFgWindowFirstFrame();

/*
 * Returns 1 if the most recently processed slice is foreground,
 * 0 if it is background.
 */
uint8_t AxonAudioFeaturesBgSliceIsForeground();

uint32_t AxonAudioFeaturesBgFgExecutionTicks();

void AxonAudioFeaturesBgFgPrintStats();
