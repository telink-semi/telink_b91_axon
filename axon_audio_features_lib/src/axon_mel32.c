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
#include <assert.h>

#include "axon_api.h"
#include "axon_dep.h"
#include "axon_audio_features_api.h"
#include "axon_logging_api.h"
#include "axon_bg_fg_vol.h"

#define FILTER_BANK_EXTRA_COEFFS 2


/*
 * This file can be built to support one or both of Mel32 (spectrogram) and MFCCs.
 * Steps are:
 * 1. input: 512 samples of 16Khz audio (Mel32 and MFCC)
 * 2. cosine hamming window,     (Mel32 and MFCC)
 * 3. 512 tap FFT                (Mel32 and MFCC)
 * 4. Average power (X^2 + Y^2)/512 (Mel32 and MFCC)
 * 5. Power rounding (Mel32 and MFFC use different values.Mel32 needs additional rounding because it doesn't have a SqRt performed before the filter banks).
 * 6. sqrt (MFCC only)
 * 7. 32 filters banks between 0 and 8000hz. (Mel32 and MFCC)
 * 8. Software rounding to 24 bits (Mel32 only)
 * 9. ln()                                   (Mel32 and MFCC)
 * 10. ln() offset added - Input to ln() is interpreted as Q11.12 so whatever the actual q factor is, the difference needs to be added to the ln() output (Mel32 and MFCC)
 * 11. DCT (MFCC only)
 * 12. 8bit quantization (MFCC only)
 */

/*
 * 4 => Log input vectors
 * 3 => Log intermediate result vectors
 * 2 => log intermediate result execution times
 * 1 => Log final result vector
 * 0 => no logging
 */
#define MEL32_DEBUG_VECTORS 0


// after doing the final log, need to add ln(2^ADJUSTMENT_BIT_COUNT) (in q11.12)
/*
 * axon_mel32_weights.h uses some of #defines above
 */
#include "axon_mel32_weights_common.h"


/*
 * Declare a buffer for holding constants copied from flash.
 * At a minimum it needs to accommodate the hamming window vector, which is 512 x 4
 */
#define CONST_BUFFER_LEN AXON_AUDIO_FEATURE_FRAME_LEN
static union {
  int32_t full_size[CONST_BUFFER_LEN];
  struct {
    int32_t ping[CONST_BUFFER_LEN/2];
    int32_t pong[CONST_BUFFER_LEN/2];
  } half_size;
}axon_const_buffer;
static int32_t hamming_buffer[AXON_AUDIO_FEATURE_FRAME_LEN];
static int32_t log_offset_add[AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS];
/*
 * MAKE SURE NEITHER OF THE MEMCPY OPS EXCEED THE BUFFER SIZE!!
 */
static_assert((CONST_BUFFER_LEN/2)>=sizeof(mel32_coefs_group1)/sizeof(mel32_coefs_group1[0]), "MEL32_COEFS_GROUP1 TOO BIG!!");
static_assert((CONST_BUFFER_LEN/2)>=sizeof(mel32_coefs_group2)/sizeof(mel32_coefs_group2[0]), "MEL32_COEFS_GROUP2 TOO BIG!!");


/*
 * "oversampling" in this context means the fraction of the output of the FFT that
 * will be used.
 */
#define AUDIO_OVERSAMPLE_RATE (AXON_AUDIO_FEATURE_SAMPLE_RATE/AXON_AUDIO_FEATURE_HIGH_FREQUENCY)

/*
 * super set of all axon operations.
 */
typedef enum {
  kMel32AxonOpWindowXty,  // 1st op is the window vector multiply
  kMel32AxonOpFft,    // do the fft
  kMel32AxonOpFftPowerXspys, // Square and sum real/imaginary pairs
  kMfccAxonOpFftPowerSum,     // sum of the fft powers. Only for kAxonAudioFeatureMfccOrthoEnergyAppend
  kMfccAxonOpFftMagnitudeSqrt,  // Square root of power. Only for kAxonAudioFeatureMfccFftMagOrtho
  kMel32FilterBankPlaceHolder, // filterbank operations
  kMel32AxonOpMelBinLog, // natural log of the mel bin
  kMfccAxonOpAddLogOffsetScalar, // add the log offset to correct for q11.12 interpretation of input to log
  kMfccAxonOpAddLogOffsetVector, // add the log offset to correct for q11.12 interpretation of input to log
  kMfccAxonOpDctMatrixMult, // DCT is a matrix-mult
  kMel32AxonOpMemCpyMeans,     // only if means are provided.
  kMel32AxonOpSubtractMeanXmy, // subtract the normalization mean, include the log offset as part of this
  kMel32AxonOpMemCpyInvStds,
  kMel32AxonOpDivideStdDevXty, // divide by normalization std dev (via a multiply/round operation)
  kMfccAxonOpQuantScalingAxpb,
  kMfccAxonOpQuantZeroPointAxpb, // only if zero-point left shifted overflows 24bits.
  kMel32AxonOpCount // operation count
} Mel32AxonOperationEnum;

/*
 * Filter banks are the same for mel32 and mfcc, but input is not the same
 * Factor this out into its own batch so that it can be executed separately.
 */
typedef enum {
  kMel32AxonOpMemCpyGroup1,
  kMel32AxonOpMelBin1stMar, // 1st Mel bin (0)
  kMel32AxonOpMemCpyGroup2 = kMel32AxonOpMelBin1stMar+MEL32_COEFS_GROUP1_OP_CNT,
  kMel32AxonOpMelBinLastMar = kMel32AxonOpMemCpyGroup2+MEL32_COEFS_GROUP2_OP_CNT, // account for the
  kMel32FilterBankAxonOpCnt,
} Mel32FilterBankAxonOpEnum;

# define IS_MEL32_MEMCPY_OP(OP_NDX) ((OP_NDX==kMel32AxonOpMemCpyMeans) || (OP_NDX==kMel32AxonOpMemCpyInvStds))


/*
 * structure for storing mel32 state information/ buffers, etc.
 */
/*
 * FIXME!!!! DON'T FORGET TO UNCOMMENT THE MEMORY ATTRIBUTE!!!
 */
RETAINED_MEMORY_SECTION_ATTRIBUTE
static struct {
  uint8_t mfcc_count; /**< number of mfccs to calculate. can be 0*/
  AxonAudioFeatureVariantsEnum audio_feature_variant;
  AxonDataWidthEnum output_saturation_packing_width;
  void *output_buffer; // user-supplied for each processed frame. Will be populated based on output_saturation_packing_width
  void *axon_handle;
  AxonResultEnum result;
  Mel32AxonOperationEnum op_enums[kMel32AxonOpCount];
  uint8_t op_cnt;
  uint8_t filterbank_op_ndx;
  AxonOpHandle mel32_op_handles[kMel32AxonOpCount];
  AxonOpHandle filterbank_op_handles[kMel32FilterBankAxonOpCnt];
  void (*frame_complete_callback_function)(AxonResultEnum result);
  uint32_t frame_cnt;
} mel32_state_info;

/*
 * buffers in un-retained memory
 */
static union {
  int32_t fft[AXON_AUDIO_FEATURE_FRAME_LEN*2]; // sized for 512 complex numbers, but only the 1st half is used after FFT operation

  struct {
    int32_t fft_1st_half[AXON_AUDIO_FEATURE_FRAME_LEN]; // holds the 1st 256 complex numbers
    int32_t after_filter_banks[AXON_AUDIO_FEATURE_FILTERBANK_COUNT]; // filter bank and later results go here
    int32_t fft_energy[FILTER_BANK_EXTRA_COEFFS];
  };
} buffers;

/*
 * queued ops struct doesn't need to be
 * in retained memory.
 */
static AxonMgrQueuedOpsStruct mel32_queued_ops;
static AxonMgrQueuedOpsStruct filterbank_queued_ops;


static inline void copy_raw_to_fft_buffer(
    const int16_t *raw_input_ping, // first set of samples
    uint32_t ping_count,           // number of samples in raw_input_ping
    const int16_t *raw_input_pong, // remaining set of samples
    int32_t *fft_buffer,           // destination int32 buffer, w/ space for imaginary components
    uint8_t input_stride) {        // spacing between samples (ie, 2 if input buffer(s) are stereo)
  uint32_t ndx;
  // copy from the ping buffer 1st...
  for (ndx=0; ndx<ping_count;ndx++) {
    // copy audio samples into real index of the FFT buffer. Imaginary component is 0'd out. Performs sign extension from 16bit to 32bit
    *fft_buffer++ = *raw_input_ping;

    *fft_buffer++ = 0; // 0 out the imaginary component
    raw_input_ping += input_stride; // skip past the 2nd channel
  }
  // copy the remaining samples from pong
  for (;ndx<AXON_AUDIO_FEATURE_FRAME_LEN;ndx++) {
    // copy audio samples into real index of the FFT buffer. Imaginary component is 0'd out. Pperforms sign extension from 16bit to 32bit
    *fft_buffer++ = *raw_input_pong;

    *fft_buffer++ = 0; // 0 out the imaginary component
    raw_input_pong += input_stride; // skip past the 2nd channel
  }

}

/*
 * All defineOp APIs have the same signature
 */
typedef AxonResultEnum (*AxonApiDefineOpFunction)(void *axon_handle, const AxonInputStruct *axon_input, AxonOpHandle *axon_op_handle);
typedef struct {
  char *label;
  int op_index;
  AxonApiDefineOpFunction define_op_function;
  AxonInputStruct axon_input;
} audio_feature_op_info_struct;


static const audio_feature_op_info_struct audio_feature_ops[]= {
    {
        .label = "Hamming Window",
        .op_index = kMel32AxonOpWindowXty,
        .define_op_function = AxonApiDefineOpXty,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FRAME_LEN,
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone+HAMMING_ROUND,
            .output_af = kAxonAfDisabled,
            .x_in = buffers.fft,
            .x_stride = kAxonStride2,
            .y_in = hamming_buffer,
            .y_stride = kAxonStride1,
            .q_out = buffers.fft,
            .q_stride = kAxonStride2,
        },
    },
    {
        .label = "FFT",
        .op_index = kMel32AxonOpFft,
        .define_op_function = AxonApiDefineOpFft,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FRAME_LEN,
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone,
            .output_af = kAxonAfDisabled,
            .x_in = buffers.fft,
            .x_stride = kAxonStride1,
            .y_in = NULL,
            .y_stride = kAxonStride1,
            .q_out = buffers.fft,
            .q_stride = kAxonStride1,
        },
    },
    {
        .label = "FFT POWER",
        .op_index = kMel32AxonOpFftPowerXspys,
        .define_op_function = AxonApiDefineOpXspys,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FRAME_LEN/AUDIO_OVERSAMPLE_RATE, // (512 samples * 2) (values per sample) / 2
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone+FFT_POWER_ROUND,
            .output_af = kAxonAfDisabled,
            .x_in = buffers.fft,
            .x_stride = kAxonStride2,
            .y_in = buffers.fft + 1,
            .y_stride = kAxonStride2,
            .q_out = buffers.fft,
            .q_stride = kAxonStride1,
        },
    },
    // #  if MFCC_APPEND_ENERGY  // place mfcc_energies[32] with ln(sum(avg(fft_power())))
      {
          .label = "FFT ENERGY",
          .op_index = kMfccAxonOpFftPowerSum,
          .define_op_function = AxonApiDefineOpAcc,
          .axon_input = {
              .length = AXON_AUDIO_FEATURE_FRAME_LEN/AUDIO_OVERSAMPLE_RATE, // (512 samples * 2) (values per sample) / 2
              .y_length = 1,
              .data_width = kAxonDataWidth24,
              .data_packing = kAxonDataPackingDisabled,
              .output_rounding = kAxonRoundingNone,
              .output_af = kAxonAfDisabled,
              .x_in = buffers.fft,
              .x_stride = kAxonStride1,
              .q_out = buffers.fft_energy,
              .q_stride = kAxonStride1,
          },
      },
      // #  if MFCC_FFT_ENERGY_SQRT
      {
          .label = "kMfccAxonOpFftMagnitudeSqrt",
          .op_index = kMfccAxonOpFftMagnitudeSqrt,
          .define_op_function = AxonApiDefineOpSqrt,
          .axon_input = {
              .length = AXON_AUDIO_FEATURE_FRAME_LEN/AUDIO_OVERSAMPLE_RATE,
              .data_width = kAxonDataWidth24,
              .data_packing = kAxonDataPackingDisabled,
              .output_rounding = kAxonRoundingNone,
              .output_af = kAxonAfDisabled,
              .x_in = buffers.fft,
              .x_stride = kAxonStride1,
              .y_in = NULL,
              .y_stride = kAxonStride1,
              .q_out = buffers.fft,
              .q_stride = kAxonStride1,
          },
      },


    { // filterbank operations
        .label = "Mel32 Filterbanks",
        .op_index = kMel32FilterBankPlaceHolder,
        .define_op_function = NULL,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT,
            .y_length = 0,
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone,
            .output_af = kAxonAfDisabled,
            .x_in = buffers.fft,
            .x_stride = kAxonStride1,
            .y_in = NULL,
            .y_stride = kAxonStride1,
            .q_out = buffers.after_filter_banks,
            .q_stride = kAxonStride1,
        },
    },
    {
        .label = "ln(mel power)",
        .op_index = kMel32AxonOpMelBinLog,
        .define_op_function = AxonApiDefineOpLogn,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS,
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone,
            .output_af = kAxonAfDisabled,
            .x_in = buffers.after_filter_banks,
            .x_stride = kAxonStride1,
            .y_in = NULL,
            .y_stride = kAxonStride1,
            .q_out = buffers.after_filter_banks,
            .q_stride = kAxonStride1,
       },
    },
    {   // scalar add. Used w/o energy append. Different value added if using fft magnitude or power
        .label = "kMfccAxonOpAddLogOffsetScalar",
        .op_index = kMfccAxonOpAddLogOffsetScalar,
        .define_op_function = AxonApiDefineOpAxpb,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT,
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone,
            .output_af = kAxonAfDisabled,
            .x_in = buffers.after_filter_banks,
            .x_stride = kAxonStride1,
            .y_in = NULL,
            .y_stride = kAxonStride1,
            .q_out = buffers.after_filter_banks,
            .q_stride = kAxonStride1,
            .a_in = 1,
            .b_in = FFT_POWER_LN_OFFSET,
        },
    },
    // #  if MFCC_APPEND_ENERGY
    {   // has to be a vector add
        .label = "kMfccAxonOpAddLogOffsetVector",
        .op_index = kMfccAxonOpAddLogOffsetVector,
        .define_op_function = AxonApiDefineOpXpy,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS, // add 2 for the fft energy,
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone,
            .output_af = kAxonAfDisabled,
            .x_in = buffers.after_filter_banks,
            .x_stride = kAxonStride1,
            .y_in = log_offset_add,
            .y_stride = kAxonStride1,
            .q_out = buffers.after_filter_banks,
            .q_stride = kAxonStride1,
        },
    },
    {
        .label = "kMfccAxonOpDctMatrixMult",
        .op_index = kMfccAxonOpDctMatrixMult,
        .define_op_function = AxonApiDefineOpMatrixMult,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT,
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone+MFCC_DCT_ROUND,
            .output_af = kAxonAfDisabled,
            .x_in = buffers.after_filter_banks,
            .x_stride = kAxonStride1,
            .y_in = mel_dct_vectors,
            .y_length = MFCC_FEATURE_COUNT,
            .y_stride = kAxonStride1,
            .q_out = buffers.after_filter_banks,
            .q_stride = kAxonStride1,
        },
    },
    {
      .label = "MemCpyMeans",
      .op_index = kMel32AxonOpMemCpyMeans,
      .define_op_function = AxonApiDefineOpMemCpy,
      .axon_input = {
          .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = NULL, // user provided
          .x_stride = kAxonStride1,
          .y_in = NULL,
          .y_length = 0, //num of padding to make the total length multiply of 2 or 4 required by the following axon operations.
          .y_stride = kAxonStride1,
          .q_out = axon_const_buffer.half_size.ping,
          .q_stride = kAxonStride1,
      },
    },
    {
        .label = "SubtracMeans",
        .op_index = kMel32AxonOpSubtractMeanXmy,
        .define_op_function = AxonApiDefineOpXmy,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS,
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone,
            .output_af = kAxonAfDisabled,
            .x_in = buffers.after_filter_banks,
            .x_stride = kAxonStride1,
            .y_in = axon_const_buffer.half_size.ping,
            .y_stride = kAxonStride1,
            .q_out = buffers.after_filter_banks,
            .q_stride = kAxonStride1,
        },
    },
    {
        .label = "MemCpyInvStds",
        .op_index = kMel32AxonOpMemCpyInvStds,
        .define_op_function = AxonApiDefineOpMemCpy,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS,
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone,
            .output_af = kAxonAfDisabled,
            .x_in = NULL,  // user provided
            .x_stride = kAxonStride1,
            .y_in = NULL,
            .y_length = 0, //num of padding to make the total length multiply of 2 or 4 required by the following axon operations.
            .y_stride = kAxonStride1,
            .q_out = axon_const_buffer.half_size.pong,
            .q_stride = kAxonStride1,
        },
    },
    {
        .label = "divide by - normalization std",
        .op_index = kMel32AxonOpDivideStdDevXty,
        .define_op_function = AxonApiDefineOpXty,
        .axon_input = {
            .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS,
            .data_width = kAxonDataWidth24,
            .data_packing = kAxonDataPackingDisabled,
            .output_rounding = kAxonRoundingNone,
            .output_af = kAxonAfDisabled,
            .x_in = buffers.after_filter_banks,
            .x_stride = kAxonStride1,
            .y_in = axon_const_buffer.half_size.pong,
            .y_stride = kAxonStride1,
            .q_out = buffers.after_filter_banks,
            .q_stride = kAxonStride1,
        },
    },
    {
      .label = "kMfccAxonOpQuantScalingAxpb",
      .op_index = kMfccAxonOpQuantScalingAxpb,
      .define_op_function = AxonApiDefineOpAxpb,
      .axon_input = {
          .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone, // user-supplied
          .output_af = kAxonAfDisabled,
          .x_in = buffers.after_filter_banks,
          .x_stride = kAxonStride1,
          .y_in = NULL,
          .y_stride = kAxonStride1,
          .q_out = buffers.after_filter_banks,
          .q_stride = kAxonStride1,
          .a_in = 0, // user supplied
          .b_in = 0, // user supplied,
      },
    },
    // # if (MFCC_OUTPUT_QUANT_ADDER!=0)
    {
      .label = "kMfccAxonOpQuantZeroPointAxpb",
      .op_index = kMfccAxonOpQuantZeroPointAxpb,
      .define_op_function = AxonApiDefineOpAxpb,
      .axon_input = {
          .length = AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = buffers.after_filter_banks,
          .x_stride = kAxonStride1,
          .y_in = NULL,
          .y_stride = kAxonStride1,
          .q_out = buffers.after_filter_banks,
          .q_stride = kAxonStride1,
          .a_in = 1,
          .b_in = 0, // user supplied
      },
    },
};

static_assert(sizeof(audio_feature_ops)/sizeof(audio_feature_ops[0])==kMel32AxonOpCount, "MEL32 OP COUNT MISMATCH!!");


static const audio_feature_op_info_struct filter_bank_ops[] = {
  {
    .label = "MemCpyGroup1",
    .op_index = kMel32AxonOpMemCpyGroup1,
    .define_op_function = AxonApiDefineOpMemCpy,
    .axon_input = {
        .length = sizeof(mel32_coefs_group1)/sizeof(mel32_coefs_group1[0]),
        .data_width = kAxonDataWidth24,
        .data_packing = kAxonDataPackingDisabled,
        .output_rounding = kAxonRoundingNone,
        .output_af = kAxonAfDisabled,
        .x_in = mel32_coefs_group1,
        .x_stride = kAxonStride1,
        .y_in = NULL,
        .y_length = 0, //num of padding to make the total length multiply of 2 or 4 required by the following axon operations.
        .y_stride = kAxonStride1,
        .q_out = axon_const_buffer.half_size.ping,
        .q_stride = kAxonStride1,
    },
  },
  {
      .label = "Filter Bank 0",
      .op_index = kMel32AxonOpMelBin1stMar+0,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN0_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN0_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN0,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[0],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 1",
      .op_index = kMel32AxonOpMelBin1stMar+1,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN1_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN1_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN1,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[1],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 2",
      .op_index = kMel32AxonOpMelBin1stMar+2,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN2_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN2_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN2,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[2],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 3",
      .op_index = kMel32AxonOpMelBin1stMar+3,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN3_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN3_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN3,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[3],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 4",
      .op_index = kMel32AxonOpMelBin1stMar+4,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN4_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN4_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN4,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[4],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 5",
      .op_index = kMel32AxonOpMelBin1stMar+5,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN5_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN5_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN5,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[5],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 6",
      .op_index = kMel32AxonOpMelBin1stMar+6,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN6_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN6_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN6,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[6],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 7",
      .op_index = kMel32AxonOpMelBin1stMar+7,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN7_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN7_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN7,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[7],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 8",
      .op_index = kMel32AxonOpMelBin1stMar+8,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN8_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN8_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN8,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[8],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 9",
      .op_index = kMel32AxonOpMelBin1stMar+9,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN9_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN9_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN9,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[9],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 10",
      .op_index = kMel32AxonOpMelBin1stMar+10,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN10_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN10_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN10,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[10],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 11",
      .op_index = kMel32AxonOpMelBin1stMar+11,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN11_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN11_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN11,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[11],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 12",
      .op_index = kMel32AxonOpMelBin1stMar+12,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN12_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN12_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN12,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[12],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 13",
      .op_index = kMel32AxonOpMelBin1stMar+13,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN13_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN13_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN13,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[13],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 14",
      .op_index = kMel32AxonOpMelBin1stMar+14,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN14_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN14_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN14,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[14],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 15",
      .op_index = kMel32AxonOpMelBin1stMar+15,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN15_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN15_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN15,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[15],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 16",
      .op_index = kMel32AxonOpMelBin1stMar+16,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN16_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN16_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN16,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[16],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 17",
      .op_index = kMel32AxonOpMelBin1stMar+17,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN17_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN17_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN17,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[17],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 18",
      .op_index = kMel32AxonOpMelBin1stMar+18,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN18_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN18_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN18,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[18],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 19",
      .op_index = kMel32AxonOpMelBin1stMar+19,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN19_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN19_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN19,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[19],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 30",
      .op_index = kMel32AxonOpMelBin1stMar+20,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN30_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN30_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN30,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[30],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 31",
      .op_index = kMel32AxonOpMelBin1stMar+21,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN31_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN31_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.ping+MEL32_COEFF_OFFSET_BIN31,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[31],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "MemCpyGroup2",
      .op_index = kMel32AxonOpMemCpyGroup2,
      .define_op_function = AxonApiDefineOpMemCpy,
      .axon_input = {
          .length = sizeof(mel32_coefs_group2)/sizeof(mel32_coefs_group2[0]),
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = mel32_coefs_group2,
          .x_stride = kAxonStride1,
          .y_in = NULL,
          .y_length = 0, //num of padding to make the total length multiply of 2 or 4 required by the following axon operations.
          .y_stride = kAxonStride1,
          .q_out = axon_const_buffer.half_size.pong,
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 20",
      .op_index = kMel32AxonOpMelBin1stMar+23,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN20_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN20_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.pong+MEL32_COEFF_OFFSET_BIN20,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[20],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 21",
      .op_index = kMel32AxonOpMelBin1stMar+24,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN21_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN21_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.pong+MEL32_COEFF_OFFSET_BIN21,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[21],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 22",
      .op_index = kMel32AxonOpMelBin1stMar+25,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN22_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN22_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.pong+MEL32_COEFF_OFFSET_BIN22,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[22],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 23",
      .op_index = kMel32AxonOpMelBin1stMar+26,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN23_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN23_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.pong+MEL32_COEFF_OFFSET_BIN23,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[23],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 24",
      .op_index = kMel32AxonOpMelBin1stMar+27,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN24_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN24_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.pong+MEL32_COEFF_OFFSET_BIN24,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[24],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 25",
      .op_index = kMel32AxonOpMelBin1stMar+28,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN25_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN25_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.pong+MEL32_COEFF_OFFSET_BIN25,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[25],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 26",
      .op_index = kMel32AxonOpMelBin1stMar+29,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN26_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN26_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.pong+MEL32_COEFF_OFFSET_BIN26,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[26],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 27",
      .op_index = kMel32AxonOpMelBin1stMar+30,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN27_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN27_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.pong+MEL32_COEFF_OFFSET_BIN27,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[27],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 28",
      .op_index = kMel32AxonOpMelBin1stMar+31,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN28_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN28_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.pong+MEL32_COEFF_OFFSET_BIN28,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[28],
          .q_stride = kAxonStride1,
      },
  },
  {
      .label = "Filter Bank 29",
      .op_index = kMel32AxonOpMelBin1stMar+32,
      .define_op_function = AxonApiDefineOpMar,
      .axon_input = {
          .length = MEL32_BIN29_TAP_COUNT,
          .data_width = kAxonDataWidth24,
          .data_packing = kAxonDataPackingDisabled,
          .output_rounding = kAxonRoundingNone,
          .output_af = kAxonAfDisabled,
          .x_in = &buffers.fft[MEL32_BIN29_1ST_TAP],
          .x_stride = kAxonStride1,
          .y_in = axon_const_buffer.half_size.pong+MEL32_COEFF_OFFSET_BIN29,
          .y_stride = kAxonStride1,
          .q_out = &buffers.after_filter_banks[29],
          .q_stride = kAxonStride1,
      },
  },
};

/*
 * ensure all operations are accounted for in the filterbank ops list.
 */
static_assert( (sizeof(filter_bank_ops)/sizeof(filter_bank_ops[0]))==kMel32FilterBankAxonOpCnt, "filter_bank_ops mis-sized");

/*
 * API function to define all the operations for Mel32 feature calculation.
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
    AxonDataWidthEnum output_saturation_packing_width ) { /**< final output will be saturated/packed to this width (24 does nothing) */
  AxonResultEnum result;
  uint32_t ndx;
  mel32_state_info.axon_handle = axon_handle;
  mel32_state_info.audio_feature_variant = which_variant;
  mel32_state_info.output_saturation_packing_width = output_saturation_packing_width;
  mel32_state_info.op_cnt = 0;
  mel32_state_info.filterbank_op_ndx = 0;
  mel32_state_info.frame_complete_callback_function = callback_function;

  /*
   * prepare background/foreground detect
   */
  if (kAxonResultSuccess > (result=AxonBgFgPrepare(axon_handle,buffers.fft, AXON_AUDIO_FEATURE_FRAME_LEN, bgfg_window_slice_cnt))) {
    return result;
  }

  /*
   * Prepare the filter bank ops. These are in a dedicated batch used by mel32 and mfcc separately
   */
  for (ndx=0;ndx<kMel32FilterBankAxonOpCnt;ndx++) {
    if (kAxonResultSuccess > (result=filter_bank_ops[ndx].define_op_function(axon_handle, &filter_bank_ops[ndx].axon_input, mel32_state_info.filterbank_op_handles+filter_bank_ops[ndx].op_index))) {
      return result;
    }
  }

  /*
   * prepare the remaining ops. Which of these get used depends on the user parameters passed in.
   */
  for (ndx=0;ndx<kMel32AxonOpCount;ndx++) {
    AxonInputStruct lo_input_struct;
    // copy flash copy into ram in case modifications are needed.
    memcpy(&lo_input_struct, &audio_feature_ops[ndx].axon_input,sizeof(lo_input_struct));
    switch (ndx) {
      case kMfccAxonOpFftPowerSum: // sum of the fft powers. Only for kAxonAudioFeatureMfccOrthoEnergyAppend
        if (which_variant != kAxonAudioFeatureMfccOrthoEnergyAppend) {
          continue;
        }
        break; // add this one as-is.

      case kMel32FilterBankPlaceHolder: // filterbank operations
        // save this index and fall through to
        mel32_state_info.filterbank_op_ndx = mel32_state_info.op_cnt;
        break; // add this one as-is

      case kMel32AxonOpWindowXty:  // 1st op is the window vector multiply
      case kMel32AxonOpFft:    // do the fft
      case kMel32AxonOpFftPowerXspys: // Square and sum real/imaginary pairs
      case kMel32AxonOpMelBinLog: // natural log of the mel bin
        break; // add these as-is
      case kMfccAxonOpFftMagnitudeSqrt:  // Square root of power. Only for kAxonAudioFeatureMfccFftMagOrtho
        if (which_variant != kAxonAudioFeatureMfccFftMagOrtho) {
          continue;
        }
        break;
      case kMfccAxonOpAddLogOffsetScalar:
        switch (which_variant) {
          case kAxonAudioFeatureMfccOrthoEnergyAppend:
            // need the vector version
            continue;
          case kAxonAudioFeatureMfccFftMagOrtho:
            // use the fft magnitude ln offset
            lo_input_struct.b_in = FFT_MAGNITUDE_LN_OFFSET;
          default:
            break;
        }
        break;
      case kMfccAxonOpAddLogOffsetVector: // add the log offset to correct for q11.12 interpretation of input to log
        if (which_variant!=kAxonAudioFeatureMfccOrthoEnergyAppend) {
          // use the scalar version
          continue;
        }
        break;

      case kMfccAxonOpDctMatrixMult: // DCT is a matrix-mult
        if (which_variant==kAxonAudioFeatureMel32) {
          // no DCT for mel32
          continue;
        }
        break;
      case kMel32AxonOpMemCpyMeans:     // only if means are provided.
        if (NULL==normalization_means_q11p12) {
          continue;
        }
        // supply the correct source address for means.
        lo_input_struct.x_in = normalization_means_q11p12;
        break;
      case kMel32AxonOpSubtractMeanXmy: // subtract the normalization mean, include the log offset as part of this
        if (NULL==normalization_means_q11p12) {
          continue;
        }
        break;

      case kMel32AxonOpMemCpyInvStds: // only if provided
        if (NULL==normalization_inv_std_devs) {
          continue;
        }
        // supply the correct source address for inverse std devs.
        lo_input_struct.x_in = normalization_inv_std_devs;
        break;

      case kMel32AxonOpDivideStdDevXty: // divide by normalization std dev (via a multiply/round operation)
        if (NULL==normalization_inv_std_devs) {
          continue;
        }
        // supply the correct rounding to maintain q11.12
        lo_input_struct.output_rounding = normalization_inv_std_devs_q_factor;
        break;

      case kMfccAxonOpQuantScalingAxpb:
        // scaling has to be greater than 1
        if (quantization_inv_scale_factor<2) {
          continue; // no scaling
        }
        // supply the correct a_in and b_in and rounding
        lo_input_struct.a_in = quantization_inv_scale_factor;
        lo_input_struct.output_rounding = quantization_inv_scale_factor_q_factor+AXON_LOG_FRACTION_BITS;
        // does scaled zero point fit in 24 bits? if yes, it goes here, otherwise need a discrete op.
        int32_t temp = quantization_zero_point;
        temp <<= quantization_zero_point;
        temp &= 0xFFFFFF; // strip off upper 8 bits
        temp >>= (AXON_LOG_FRACTION_BITS+quantization_inv_scale_factor);
        if ((int8_t)temp==quantization_zero_point) {
          // it fits, use it.
          lo_input_struct.b_in = quantization_zero_point <<(AXON_LOG_FRACTION_BITS+quantization_inv_scale_factor);
          // 0 out the zero point so it doesn't get used in the next op.
          quantization_zero_point = 0;
        }
        break;

      case kMfccAxonOpQuantZeroPointAxpb: // only if zero-point left shifted overflows 24bits.
        if (0==quantization_zero_point) {
          continue;
        }
        lo_input_struct.b_in = quantization_zero_point;
        break;

      default:
        while(1); // ???

    } // switch(ndx)

    // add this op

    // save the op_enum here
    mel32_state_info.op_enums[mel32_state_info.op_cnt]=ndx;

    // define the op & save it.
    if (NULL!=audio_feature_ops[ndx].define_op_function) {
      if (kAxonResultSuccess > (result=audio_feature_ops[ndx].define_op_function(axon_handle, &lo_input_struct, mel32_state_info.mel32_op_handles+mel32_state_info.op_cnt))) {
        return result;
      }
    }
    // count it!
    mel32_state_info.op_cnt++;
  }

  return result;
}

/*
 * Don't really have anything to do other than restart the background/foreground model...
 * Except copy the hamming window constants to RAM. We assume that since audio is up the constants
 * will remain retained in memory for the duration of the session.
 *
 * This is the very 1st operaion and so there is no way to hide it in the shadow of another operation
 * so just copy it once per session.
 */
void AxonAudioFeaturesRestart() {
  AxonBgFgRestart();
  memcpy(hamming_buffer, mel32_window, AXON_AUDIO_FEATURE_FRAME_LEN * sizeof(int32_t) );
  mel32_state_info.frame_cnt = 0;
  if (kAxonAudioFeatureMfccOrthoEnergyAppend==mel32_state_info.audio_feature_variant) {
    // the ln() energy has a different q offset from the rest of the filterbank output.
    for (uint8_t loNdx=0; loNdx < AXON_AUDIO_FEATURE_FILTERBANK_COUNT; loNdx++) {
      log_offset_add[loNdx] = FFT_POWER_LN_OFFSET;
    }
    log_offset_add[AXON_AUDIO_FEATURE_FILTERBANK_COUNT] = FFT_ENERGY_LN_OFFSET;
  }
}

static void filterbank_software_rounding() {
  // just like mel32, if no sqrt then need to do a software round
  for (uint8_t filter_bank_coef=0;filter_bank_coef<(AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS);filter_bank_coef++) {
    buffers.after_filter_banks[filter_bank_coef] = buffers.after_filter_banks[filter_bank_coef] >> FILTER_BANK_SW_ROUND;
  }

}

/*
 * This gets called to perform the 3rd (and last) async set of processing.
 * If in async mode, it is expected to be in response to an axon interrupt, so
 * need to check the driver status.
 */
static void all_ops_done_callback(AxonResultEnum result, void *callback_context) {

  switch (mel32_state_info.audio_feature_variant) {
  case kAxonAudioFeatureMel32: // copy 32 coefficients
    AxonApiCopySaturateVector(AXON_CONSTRUCT_COMPOSITE_WIDTH(mel32_state_info.output_saturation_packing_width, kAxonDataWidth24),
        mel32_state_info.output_buffer, buffers.after_filter_banks, AXON_AUDIO_FEATURE_FILTERBANK_COUNT, 0);
# if MEL32_DEBUG_VECTORS > 0
    print_int32_vector(mel32_state_info.axon_handle, "audio_features",  buffers.after_filter_banks, AXON_AUDIO_FEATURE_FILTERBANK_COUNT, 1);
# endif
    break;

  case kAxonAudioFeatureMfccOrthoEnergyAppend:
    // replace coefficient 0 w/ the energy before falling through.
    buffers.after_filter_banks[0] = buffers.fft_energy[0];

  case kAxonAudioFeatureMfccOrtho:
  case kAxonAudioFeatureMfccFftMagOrtho:
    AxonApiCopySaturateVector(AXON_CONSTRUCT_COMPOSITE_WIDTH(mel32_state_info.output_saturation_packing_width, kAxonDataWidth24),
        mel32_state_info.output_buffer, buffers.after_filter_banks, MFCC_FEATURE_COUNT, 0);
# if MEL32_DEBUG_VECTORS > 0
    print_int32_vector(mel32_state_info.axon_handle, "audio_features",  buffers.after_filter_banks, MFCC_FEATURE_COUNT, 1);
# endif
    break;

  }

  /*
   * invoke the callback
   */
  mel32_state_info.frame_complete_callback_function(result);
}


/*
 * This gets called after mfcc filterbanks are completed.
 */
static void filterbanks_done_callback(AxonResultEnum result, void *callback_context) {

  if (kAxonResultSuccess != result) {
    // had a failure, short circuit the result.
    mel32_state_info.frame_complete_callback_function(result);
  }
#if MEL32_DEBUG_VECTORS > 0
  print_int32_vector(mel32_state_info.axon_handle, "mfcc", buffers.after_filter_banks, AXON_AUDIO_FEATURE_FILTERBANK_COUNT, 1);
#endif

  if (mel32_state_info.audio_feature_variant != kAxonAudioFeatureMfccFftMagOrtho) {
  // do a software round unless using fft magnitude
    filterbank_software_rounding();
  }

  // MFCC filterbank just finished up, so now add the remainder of the ops
  // resume after the mel32 filterbanks.
  mel32_queued_ops.op_handle_list = mel32_state_info.mel32_op_handles+mel32_state_info.filterbank_op_ndx+1;

  mel32_queued_ops.callback_function = all_ops_done_callback;
  mel32_queued_ops.callback_context = NULL;
  mel32_queued_ops.op_handle_count = mel32_state_info.op_cnt-mel32_state_info.filterbank_op_ndx-1;

  AxonApiQueueOpsList(mel32_state_info.axon_handle,&mel32_queued_ops);

}


#if (AXON_AUDIO_FEATURE_MEL32)

/*
 * This gets called after mel32 filterbanks are completed.
 */
static void mel32_filterbanks_done_callback(AxonResultEnum result, void *callback_context) {

  if (kAxonResultSuccess != result) {
    // had a failure, short circuit the result.
    mel32_state_info.frame_complete_callback_function(result);
  }
#if MEL32_DEBUG_VECTORS > 0
  print_int32_vector(mel32_state_info.axon_handle, "mel32", buffers.after_filter_banks, AXON_AUDIO_FEATURE_FILTERBANK_COUNT, 1);
#endif


  // MEL32 filterbank just finished up...
#if FILTER_BANK_SW_ROUND > 0
  /*
   * do the software round
   */
  filterbank_software_rounding();
#endif
  // resume after the mel32 filterbanks.
  mel32_queued_ops.op_handle_list = mel32_state_info.mel32_op_handles+kMel32FilterBankPlaceHolder+1;

#if (AXON_AUDIO_FEATURE_MFCC)
  // mel32s and mfccs are both enabled, need to execute up to the mfccs filterbank
  mel32_queued_ops.callback_function = NULL; // don't need a callback
  mel32_queued_ops.callback_context = NULL;
  mel32_queued_ops.op_handle_count = kMfccFilterBankPlaceHolder-kMel32FilterBankPlaceHolder-1;
  AxonApiQueueOpsList(mel32_state_info.axon_handle,&mel32_queued_ops);
  // and also queue up the mfcc filterbanks.
  filterbank_queued_ops.callback_function = mfcc_filterbanks_done_callback;
  filterbank_queued_ops.op_handle_list = mel32_state_info.filterbank_op_handles;
  filterbank_queued_ops.callback_context = NULL;
  filterbank_queued_ops.op_handle_count = kMel32FilterBankAxonOpCnt;
  AxonApiQueueOpsList(mel32_state_info.axon_handle,&filterbank_queued_ops);

#else
  // no MFCCs, so just finish up the mel32s
  mel32_queued_ops.callback_function = all_ops_done_callback;
  mel32_queued_ops.op_handle_list = mel32_state_info.mel32_op_handles+kMel32FilterBankPlaceHolder+1;
  mel32_queued_ops.callback_context = NULL;
  mel32_queued_ops.op_handle_count = kMel32AxonOpCount-kMel32FilterBankPlaceHolder-1;
  AxonApiQueueOpsList(mel32_state_info.axon_handle,&mel32_queued_ops);
#endif


}
#endif

/*
 * API function
 */
AxonResultEnum AxonAudioFeatureProcessFrame(
    const int16_t *raw_input_ping,
    uint32_t ping_count,
    const int16_t *raw_input_pong,
    AxonBoolEnum last_frame, /**< if true, last frame BG/FG exception occurs */
    uint8_t input_stride,
    void *output_buffer /**< stores the mel32 output vector */
    ){
  AxonResultEnum result;
#if MEL32_DEBUG_VECTORS > 1
  uint32_t start_time;
  uint32_t end_time;
  uint32_t elapsed_time = 0;
  AxonPrintf("AudioFeature #%u\r\n", mel32_state_info.frame_cnt++);
#endif

  mel32_state_info.output_buffer = output_buffer;

#if MEL32_DEBUG_VECTORS > 4
  print_int16_vector(axon_handle, "raw_input_ping", raw_input_ping, ping_count, input_stride);
  //print_int16_vector(axon_handle, "raw_input_pong", raw_input_ping, AXON_AUDIO_FEATURE_FRAME_LEN-ping_count, input_stride);
#endif

  // copy raw input to our internal int32 buffer
  copy_raw_to_fft_buffer( raw_input_ping, ping_count, raw_input_pong, buffers.fft, input_stride);

#if MEL32_DEBUG_VECTORS > 1
  // need bg fg to run synchronously so that axon is free to be used upon return
#  define BG_FG_ASYNC_MODE kAxonAsyncModeSynchronous
#else
#  define BG_FG_ASYNC_MODE kAxonAsyncModeAsynchronous
#endif
  // background/foreground needs the samples in buffers.fft
  if (kAxonResultSuccess>(result=AxonBgFgProcessFrame(mel32_state_info.axon_handle, last_frame, BG_FG_ASYNC_MODE))) {
    return result; // error!
  }

#if MEL32_DEBUG_VECTORS > 3
  print_int32_vector(mel32_state_info.axon_handle, "shifted_input", buffers.fft, 512,2);
#endif

  // run the operations
#if MEL32_DEBUG_VECTORS > 1
  /*
   * DEBUG! ONE AT A TIME SO WE CAN EXAMINE RESULTS! NOTE: THIS WILL BE DONE SYNCHRONOUSLY!
   */
  uint8_t op_cnt; // almost one at a time. do 2 ops if the 1st is a memcpy
  for (uint32_t lo_ndx=0;lo_ndx<mel32_state_info.op_cnt;lo_ndx+=op_cnt) {
    Mel32AxonOperationEnum op_enum = mel32_state_info.op_enums[lo_ndx];

    start_time = AxonHostGetTime();

    /*
     * Look for special cases:
     * 1) filter bank (separate op group)
     * 2) software round of mel32 fileter bank.
     */

    /*
     * handle the filter banks separately
     */
    if (op_enum==kMel32FilterBankPlaceHolder) { // filterbank operations
      if (kAxonResultSuccess>(result=AxonApiExecuteOps(mel32_state_info.axon_handle, kMel32FilterBankAxonOpCnt, mel32_state_info.filterbank_op_handles,kAxonAsyncModeSynchronous ))) {
        break; // error!
      }
      AxonPrintf("%s elapsed %u ticks\r\n", audio_feature_ops[op_enum].label, elapsed_time);
#if MEL32_DEBUG_VECTORS > 2
      print_int32_vector(mel32_state_info.axon_handle, audio_feature_ops[op_enum].label,
            (int32_t*)audio_feature_ops[op_enum].axon_input.q_out,
            audio_feature_ops[op_enum].axon_input.y_length > 0 ? audio_feature_ops[op_enum].axon_input.y_length : audio_feature_ops[op_enum].axon_input.length,
            audio_feature_ops[op_enum].axon_input.q_stride);
#endif
      continue;
    }

    // if this is the log operation, perform a software round 1st
    if (op_enum==kMel32AxonOpMelBinLog) {
      if (mel32_state_info.audio_feature_variant != kAxonAudioFeatureMfccFftMagOrtho) {
        // software rounding required if filterbank inputs weren't fft maagnitude.
        filterbank_software_rounding();

#if MEL32_DEBUG_VECTORS > 2
        print_int32_vector(mel32_state_info.axon_handle, "sw_round", buffers.after_filter_banks, AXON_AUDIO_FEATURE_FILTERBANK_COUNT+FILTER_BANK_EXTRA_COEFFS, 1);
#endif
      }
    }

    /*
     *  the very last op needs to be queued so that
     * the execution can continue in the interrupt context. This means that the very last op won't get logged
     * here, but that's okay because the last op is by definition the final output, and that will
     * get logged separately.
     */

    op_cnt = IS_MEL32_MEMCPY_OP(op_enum) ? 2 : 1;

    if ((lo_ndx+op_cnt)==mel32_state_info.op_cnt) {
      // last op(s); queue them up
      mel32_queued_ops.callback_function = all_ops_done_callback;
      mel32_queued_ops.op_handle_list = mel32_state_info.mel32_op_handles+lo_ndx;
      mel32_queued_ops.callback_context = NULL;
      mel32_queued_ops.op_handle_count = op_cnt;
      return AxonApiQueueOpsList(mel32_state_info.axon_handle, &mel32_queued_ops);
    } else  if (kAxonResultSuccess>(result=AxonApiExecuteOps(mel32_state_info.axon_handle, op_cnt, &mel32_state_info.mel32_op_handles[lo_ndx],kAxonAsyncModeSynchronous ))) {
      break; // error!
    }

    end_time = AxonHostGetTime();
    elapsed_time += (end_time-start_time);
#if MEL32_DEBUG_VECTORS > 2
    Mel32AxonOperationEnum print_op_enum = mel32_state_info.op_enums[lo_ndx+op_cnt-1];
    AxonPrintf("%s elapsed %u ticks\r\n", audio_feature_ops[print_op_enum].label, elapsed_time);
    print_int32_vector(mel32_state_info.axon_handle, audio_feature_ops[print_op_enum].label,
          (int32_t*)audio_feature_ops[print_op_enum].axon_input.q_out,
          audio_feature_ops[print_op_enum].axon_input.y_length > 0 ? audio_feature_ops[print_op_enum].axon_input.y_length : audio_feature_ops[print_op_enum].axon_input.length,
          audio_feature_ops[print_op_enum].axon_input.q_stride);
#endif

  }
#else
  /*
   * not breaking this up into individual operations, but we do have at least 2 batches to
   * execute.
   *
   * Batch 1:
   * Starts at the beginning and either end w/ the mel32 filterbanks (if mel32 defined)
   * or the mfcc filterbanks.
   *
   * Batch 2:
   * queued along w/ Batch 1, consists of either the mel32 filterbanks (if mel32 defined)
   * or the mfcc filterbanks.
   */
  mel32_queued_ops.callback_function = NULL; // don't need a callback, just proceed to batch 2
  mel32_queued_ops.op_handle_list = mel32_state_info.mel32_op_handles;
  mel32_queued_ops.callback_context = NULL;
  // stop at the filter banks place-holder
  mel32_queued_ops.op_handle_count = mel32_state_info.filterbank_op_ndx;
  if (kAxonResultSuccess>(result=AxonApiQueueOpsList(mel32_state_info.axon_handle, &mel32_queued_ops))) {
    return result;
  }

  // now queue up the filter banks
  filterbank_queued_ops.callback_function = filterbanks_done_callback;
  filterbank_queued_ops.op_handle_list = mel32_state_info.filterbank_op_handles;
  filterbank_queued_ops.callback_context = NULL;
  filterbank_queued_ops.op_handle_count = kMel32FilterBankAxonOpCnt;
  return AxonApiQueueOpsList(mel32_state_info.axon_handle, &filterbank_queued_ops);

#endif
}


