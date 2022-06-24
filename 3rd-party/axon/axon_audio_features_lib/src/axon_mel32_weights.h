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

#include "axon_audio_features_api.h"

/*
 * Save model (audio features, neural networks etc) parameters into FLASH
 */

/*
 * Rounding has to be balanced to retain maximum precision while still reducing the filter
 * bank output to no more than 24bits and not exceeding 24bits in any vector operation (matrix multiply
 * supports 32bits).
 *
 * The outputs of the filter banks have a ln() applied. The ln() treats the input as q11.12. If the input isn't
 * (and it won't be), an offset will need to be added.
 *
 * The general operations are:
 * 1) Multiply the 16bit audio samples by a hamming window of 8bit samples. The output of this is fed into an FFT
 * - without any rounding, the output would by q15.8, or the full 24 bits. Each FFT tap the sum of the 512 inputs times a scaling factor. This could saturate 24bits.
 * - Performing a round of at least 4 bits should prevent saturation.
 * 2) Perform the FFT. The output could be a full 24bits, and the next step is to square and sum (spectral power). 24bits needs to be taken down to 12bits.
 * 3) Calculate the spectral power of the FFT (real component squared + imaginary component squared).
 * blah blah blah
 *
 * At the end of the day, the output of the log will need an offset added to account for the precision difference between
 * what the trained model used and what the quantized inference uses.
 *
 * The trained model starts with Q15.0 raw input, squares and averages (over the sample size, ie 512). Everything else maintains
 * the precision. Q15.0 x Q15.0 = Q30.0; Q30.0 / 512 = Q21.0
 *
 * Quantized is less direct.
 * Q15.0 input is multiplied by Q1.HAMMING_BITS then rounded by HAMMING_ROUND, for Q15.
 */


#define AXON_LOG_FRACTION_BITS 12  // log treats input as Q11.12
#define TARGET_ROUNDING 9          // total rounding will be 9, the equivalent of the /512 in floating point.
#define HAMMING_BITS    8          // number of bits in the hamming vector coefficients.
#define HAMMING_ROUND   8          // amount we'll round the input * hamming window. These bits will be doubled because the output is squared later.
#define FFT_POWER_ROUND 11          // amount we'll round the FFT Power after calculating it, before the filter bank multiplication.
#define FILTER_BANK_BITS 8         // Number of bits in the mel filter bank vector coefficients
#define FILTER_BANK_RIGHT_SHIFT  0 // Number of bits to shave off the filter banks (instead of regenerating the banks)
#define FILTER_BANK_NET_BITS \
        (FILTER_BANK_BITS-FILTER_BANK_RIGHT_SHIFT)  // net effect of the filter banks on the q position

#  define FILTER_BANK_SW_ROUND     8 // rounding performed by software (really just a shift) after filterbank, before log.
#  define MEL32_ADJUSTMENT_BIT_COUNT (AXON_LOG_FRACTION_BITS-(TARGET_ROUNDING-2*(HAMMING_ROUND-HAMMING_BITS)-FFT_POWER_ROUND+FILTER_BANK_NET_BITS-FILTER_BANK_SW_ROUND))
static_assert(MEL32_ADJUSTMENT_BIT_COUNT==14, "\r\nADJUSTMENT BIT COUNT IS 14, RECALCULATE THE LOG OFFSET\r\n");

/*
 * One of these values gets added to the log of the filter banks to compensate for all the rounding
 * that has occurred and for the interpretation of the log input as a Q11.12.
 */
#define LN_2_TOTHE_5_11Q12 14196 /*  LN(2^5) =  3.465735, but as a Q11.12*/
#define LN_2_TOTHE_13_11Q12 36909 /*  LN(2^13) =  9.010913347, but as a Q11.12*/
#define LN_2_TOTHE_14_11Q12 39748  /* LN(2^14) = 9.70406, but as Q11.12 */
#define LN_2_TOTHE_15_11Q12 42587  /* LN(2^15) = 10.39720771, but as Q11.12 */
#define LN_2_TOTHE_22_11Q12 62461  /* LN(2^2) = 15.24923, but as Q11.12 */

/*
 * Mel32s arrive at the ln() operation as q.-2, need 14 bits to get to q11.12
 */
#define FFT_POWER_LN_OFFSET LN_2_TOTHE_14_11Q12
#define FFT_MAGNITUDE_LN_OFFSET LN_2_TOTHE_5_11Q12
#define FFT_ENERGY_LN_OFFSET LN_2_TOTHE_22_11Q12

/*
 * q_factor (and hence rounding) to apply to
 * DCT
 */
# define MFCC_DCT_ROUND 10

#if (AXON_AUDIO_FEATURE_MFCC)
#define MFCC_APPEND_ENERGY 1
/*
 * Ln adjustment is different for MFCC.
 * The square root after fft average power will halve the Q factor
 */
#  define MFCC_QFACTOR_BEFORE_SQRT (TARGET_ROUNDING-2*(HAMMING_ROUND-HAMMING_BITS)-FFT_POWER_ROUND)
static_assert((MFCC_QFACTOR_BEFORE_SQRT & 1)==0, "QFACTOR UP TO SQRT MUST BE EVEN!");

#  define MFCC_ADJUSTMENT_BIT_COUNT (AXON_LOG_FRACTION_BITS-(MFCC_QFACTOR_BEFORE_SQRT/2+FILTER_BANK_NET_BITS))

#    if MFCC_FFT_ENERGY_SQRT
/*
 * Mel32s arrive at the ln() operation as q.-2, need 14 bits to get to q11.12
 */
static_assert(MFCC_ADJUSTMENT_BIT_COUNT==5, "\r\n MFCC ADJUSTMENT BIT COUNT IS 5, RECALCULATE THE LOG OFFSET\r\n");
#      define MFCC_LN_OFFSET LN_2_TOTHE_5_11Q12
#    else
#      define FILTER_BANK_SW_ROUND     8 // rounding performed by software (really just a shift) after filterbank, before log.
#      define MFCC_ADJUSTMENT_BIT_COUNT (AXON_LOG_FRACTION_BITS-(TARGET_ROUNDING-2*(HAMMING_ROUND-HAMMING_BITS)-FFT_POWER_ROUND+FILTER_BANK_NET_BITS-FILTER_BANK_SW_ROUND))
static_assert(MFCC_ADJUSTMENT_BIT_COUNT==14, "\r\nADJUSTMENT BIT COUNT IS 14, RECALCULATE THE LOG OFFSET\r\n");
#      define MFCC_LN_OFFSET LN_2_TOTHE_14_11Q12
#    endif

#    if MFCC_APPEND_ENERGY
#      define FILTER_BANK_EXTRA_COEFFS  2
#    endif
/*
 * rounding bits to apply after DCT
 */

/*
 * output quantization may need to be in 2 steps, scaling, then add zero point.
 * The output of the scaling maybe a very high q factor, which will make the zero point overflow 24bits.
 */
/*
 * Quantization Scaling
 */
# define MFCC_OUTPUT_QUANT_MULTIPLIER  (907)   // this is a q.8, the dct output is a q
# define MFCC_OUTPUT_QUANT_SCALED_ADDER 0      // 77 << 20 overflows 24 bits so need a separate add opperation
# define MFCC_OUTPUT_QUANT_ROUND        20     // output of the DCT is q.12, multiplied by q.8 so need to round by 20
# define MFCC_OUTPUT_QUANT_ADDER        0


//static_assert((MFCC_QUANT_ZP_INV_QP8<8388608) && (MFCC_QUANT_ZP_INV_QP8>=-8388608, "MFCC QUANT 24BIT OVERFLOW!!!")


#endif



