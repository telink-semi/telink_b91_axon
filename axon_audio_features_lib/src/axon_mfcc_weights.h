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
#include "axon_mfcc.h"
/*
 * Window function is a simple vector multiply of the input vector
 * The function is simply cosine scaled between 0 and 1 (amplitude) and scaled
 * between 0 and Pi across the input.
 *
 * the format in the array is Q15.8
 */

#if 0
// DEPRECATED!!!
/**
 * MFCCs still require all constants to be placed in RAM (have not implemented memcpy operations for it yet).
 */

static  MEL_WINDOW_TYPE mfcc_window[] = {
    0,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,2,3,3,3,4,4,5,5,6,6,6,7,8,8,9,9,10,10,11,12,12,13,14,14,15,16,17,17,18,19,20,21,22,23,23,24,25,26,27,28,29,30,31,32,33,34,35,37,38,39,40,41,42,43,45,46,47,48,49,51,52,53,54,56,57,58,60,61,62,64,65,67,68,69,71,72,74,75,76,78,79,81,82,84,85,87,88,90,91,93,94,96,97,99,100,102,103,105,106,108,110,111,113,114,116,117,119,121,122,124,125,127,128,130,132,133,135,136,138,139,141,143,144,146,147,149,150,152,153,155,156,158,160,161,163,164,166,167,169,170,172,173,175,176,177,179,180,182,183,185,186,187,189,190,192,193,194,196,197,198,200,201,202,203,205,206,207,208,210,211,212,213,214,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,236,237,238,239,240,240,241,242,243,243,244,245,245,246,247,247,248,248,249,249,250,250,251,251,252,252,252,253,253,253,254,254,254,254,255,255,255,255,255,256,256,256,256,256,256,256,256,256,256,256,256,256,256,255,255,255,255,255,254,254,254,254,253,253,253,252,252,252,251,251,250,250,249,249,248,248,247,247,246,245,245,244,243,243,242,241,240,240,239,238,237,236,236,235,234,233,232,231,230,229,228,227,226,225,224,223,222,221,220,219,218,217,216,214,213,212,211,210,208,207,206,205,203,202,201,200,198,197,196,194,193,192,190,189,187,186,185,183,182,180,179,177,176,175,173,172,170,169,167,166,164,163,161,160,158,156,155,153,152,150,149,147,146,144,143,141,139,138,136,135,133,132,130,128,127,125,124,122,121,119,117,116,114,113,111,110,108,106,105,103,102,100,99,97,96,94,93,91,90,88,87,85,84,82,81,79,78,76,75,74,72,71,69,68,67,65,64,62,61,60,58,57,56,54,53,52,51,49,48,47,46,45,43,42,41,40,39,38,37,35,34,33,32,31,30,29,28,27,26,25,24,23,23,22,21,20,19,18,17,17,16,15,14,14,13,12,12,11,10,10,9,9,8,8,7,6,6,6,5,5,4,4,3,3,3,2,2,2,2,1,1,1,1,1,0,0,0,0,0,0,0,0,
};
/*
 * don't forget, everything has to be a multiple of 4
 */
#define MFCC_BIN0_1ST_TAP 1
#define MFCC_BIN0_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin0[] = {
    1,  // 1st valid tap index
    4,  // # of valid tap indexes
    87, 191,
    0, 0, 0, 0
};

#define MFCC_BIN1_1ST_TAP 2
#define MFCC_BIN1_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin1[] = {
    2,  // 1st valid tap index
    4,  // # of valid tap indexes
    65,222,
    0,0
};

#define MFCC_BIN2_1ST_TAP 3
#define MFCC_BIN2_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin2[] = {
    3,  // 1st valid tap index
    4,  // # of valid tap indexes
    34,250,54,
    0,0,0,
};
#define MFCC_BIN3_1ST_TAP 5
#define MFCC_BIN3_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin3[] = {
    5,  // 1st valid tap index
    4,  // # of valid tap indexes
    202,110,
    0,0,0, // pad out
};
#define MFCC_BIN4_1ST_TAP 6
#define MFCC_BIN4_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin4[] = {
    6,  // 1st valid tap index
    4,  // # of valid tap indexes
    146, 172,
    0,0,0, // pad out
};

#define MFCC_BIN5_1ST_TAP 7
#define MFCC_BIN5_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin5[] = {
    7,  // 1st valid tap index
    4,  // # of valid tap indexes
    84,241,60,
    0,0,0, // pad out
};

#define MFCC_BIN6_1ST_TAP 8
#define MFCC_BIN6_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin6[] = {
    8,  // 1st valid tap index
    4,  // # of valid tap indexes
    15,196,141,
    0,
};

#define MFCC_BIN7_1ST_TAP 10
#define MFCC_BIN7_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin7[] = {
    10,  // 1st valid tap index
    4,  // # of valid tap indexes
    115,227,62,
    0,
};

#define MFCC_BIN8_1ST_TAP 11
#define MFCC_BIN8_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin8[] = {
    11,  // 1st valid tap index
    4,  // # of valid tap indexes
    29,194,157,
    0,0,0, // pad out
};

#define MFCC_BIN9_1ST_TAP 13
#define MFCC_BIN9_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin9[] = {
    13,  // 1st valid tap index
    4,  // # of valid tap indexes
    99,255,106,
    0,0,0, // pad out
};

#define MFCC_BIN10_1ST_TAP 15
#define MFCC_BIN10_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin10[] = {
    15,  // 1st valid tap index
    4,  // # of valid tap indexes
    150,214,70,
    0,0,0, // pad out
};

#define MFCC_BIN11_1ST_TAP 16
#define MFCC_BIN11_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin11[] = {
    16,  // 1st valid tap index
    4,  // # of valid tap indexes
    42,186,186,49,
};

#define MFCC_BIN12_1ST_TAP 18
#define MFCC_BIN12_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin12[] = {
    18,  // 1st valid tap index
    4,  // # of valid tap indexes
    70,207,172,41,
    0,0,0, // pad out
};

#define MFCC_BIN13_1ST_TAP 20
#define MFCC_BIN13_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin13[] = {
    20,  // 1st valid tap index
    4,  // # of valid tap indexes
    84,215,170,45,
    0,0,0, // pad out
};

#define MFCC_BIN14_1ST_TAP 22
#define MFCC_BIN14_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin14[] = {
    22,  // 1st valid tap index
    4,  // # of valid tap indexes
    86,211,179,60,
    0,0,0, // pad out
};

#define MFCC_BIN15_1ST_TAP 24
#define MFCC_BIN15_TAP_COUNT 4

static  MEL_WEIGHTS_TYPE mfcc_weights_bin15[] = {
    24,  // 1st valid tap index
    4,  // # of valid tap indexes
    77,196,199,85,
    0,0,0, // pad out
};

#define MFCC_BIN16_1ST_TAP 26
#define MFCC_BIN16_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin16[] = {
    26,  // 1st valid tap index
    8,  // # of valid tap indexes
    57,171,229,119,11,
    0,0,0, // pad out
};

#define MFCC_BIN17_1ST_TAP 28
#define MFCC_BIN17_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin17[] = {
    28,  // 1st valid tap index
    8,  // # of valid tap indexes
    27,137,245,161,57,
    0,0,0, // pad out
};

#define MFCC_BIN18_1ST_TAP 31
#define MFCC_BIN18_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin18[] = {
    31,  // 1st valid tap index
    8,  // # of valid tap indexes
    95,199,212,111,13,
    0,0,0, // pad out
};

#define MFCC_BIN19_1ST_TAP 33
#define MFCC_BIN19_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin19[] = {
    33,  // 1st valid tap index
    8,  // # of valid tap indexes
    44,145,243,172,77,
    0,0,0,
};

#define MFCC_BIN20_1ST_TAP 36
#define MFCC_BIN20_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin20[] = {
    36,  // 1st valid tap index
    8,  // # of valid tap indexes
    84,179,240,148,58,
    0,0,0,
};

#define MFCC_BIN21_1ST_TAP 38
#define MFCC_BIN21_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin21[] = {
    38,  // 1st valid tap index
    8,  // # of valid tap indexes
    16,108,198,225,137,51,
    0,0,0,
};

#define MFCC_BIN22_1ST_TAP 41
#define MFCC_BIN22_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin22[] = {
    41,  // 1st valid tap index
    8,  // # of valid tap indexes
    31,119,205,222,139,56,
    0,0,0,
};

#define MFCC_BIN23_1ST_TAP 44
#define MFCC_BIN23_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin23[] = {
    44,  // 1st valid tap index
    8,  // # of valid tap indexes
    34,117,200,231,151,72,
    0,0,0,
};

#define MFCC_BIN24_1ST_TAP 47
#define MFCC_BIN24_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin24[] = {
    47,  // 1st valid tap index
    8,  // # of valid tap indexes
    25,105,184,250,173,98,23,
    0,0,0,
};

#define MFCC_BIN25_1ST_TAP 50
#define MFCC_BIN25_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin25[] = {
    50,  // 1st valid tap index
    8,  // # of valid tap indexes
    6,83,158,233,205,133,61,
    0,0,0,
};

#define MFCC_BIN26_1ST_TAP 54
#define MFCC_BIN26_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin26[] = {
    54,  // 1st valid tap index
    8,  // # of valid tap indexes
    51,123,195,246,176,107,38,
    0,0,0,
};

#define MFCC_BIN27_1ST_TAP 57
#define MFCC_BIN27_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin27[] = {
    57,  // 1st valid tap index
    8,  // # of valid tap indexes
    10,80,149,218,227,160,94,29,
    0,0,0,
};

#define MFCC_BIN28_1ST_TAP 61
#define MFCC_BIN28_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin28[] = {
    61,  // 1st valid tap index
    8,  // # of valid tap indexes
    29,96,162,227,221,157,94,32,
    0,0,0,
};

#define MFCC_BIN29_1ST_TAP 65
#define MFCC_BIN29_TAP_COUNT 8

static  MEL_WEIGHTS_TYPE mfcc_weights_bin29[] = {
    65,  // 1st valid tap index
    8,  // # of valid tap indexes
    35,99,162,224,226,166,105,46,
    0,0,0,
};

#define MFCC_BIN30_1ST_TAP 69
#define MFCC_BIN30_TAP_COUNT 12

static  MEL_WEIGHTS_TYPE mfcc_weights_bin30[] = {
    69,  // 1st valid tap index
    12,  // # of valid tap indexes
    30,90,151,210,243,184,127,70,13,
    0,0,0,
};

#define MFCC_BIN31_1ST_TAP 73
#define MFCC_BIN31_TAP_COUNT 12

static  MEL_WEIGHTS_TYPE mfcc_weights_bin31[] = {
    73,  // 1st valid tap index
    12,  // # of valid tap indexes
    13,72,129,186,243,213,158,103,48,
    0,0,0,
};

#define MFCC_BIN32_1ST_TAP 78
#define MFCC_BIN32_TAP_COUNT 12

static  MEL_WEIGHTS_TYPE mfcc_weights_bin32[] = {
    78,  // 1st valid tap index
    12,  // # of valid tap indexes
    43,98,153,208,250,197,144,92,40,
    0,0,0,
};

#define MFCC_BIN33_1ST_TAP 83
#define MFCC_BIN33_TAP_COUNT 12

static  MEL_WEIGHTS_TYPE mfcc_weights_bin33[] = {
    83,  // 1st valid tap index
    12,  // # of valid tap indexes
    59,112,164,216,245,194,143,93,44,
    0,0,0,
};

#define MFCC_BIN34_1ST_TAP 87
#define MFCC_BIN34_TAP_COUNT 12

static  MEL_WEIGHTS_TYPE mfcc_weights_bin34[] = {
    87,  // 1st valid tap index
    12,  // # of valid tap indexes
    11,62,113,163,212,251,202,154,106,58,11,
    0,0,0,
};

#define MFCC_BIN35_1ST_TAP 92
#define MFCC_BIN35_TAP_COUNT 12

static  MEL_WEIGHTS_TYPE mfcc_weights_bin35[] = {
    92,  // 1st valid tap index
    12,  // # of valid tap indexes
    5,54,102,150,198,245,221,174,129,83,38,
    0,0,0,
};

#define MFCC_BIN36_1ST_TAP 98
#define MFCC_BIN36_TAP_COUNT 12

static  MEL_WEIGHTS_TYPE mfcc_weights_bin36[] = {
    98,  // 1st valid tap index
    12,  // # of valid tap indexes
    35,82,127,173,218,249,205,161,117,73,30,
    0,0,0,
};

#define MFCC_BIN37_1ST_TAP 103
#define MFCC_BIN37_TAP_COUNT 12

static  MEL_WEIGHTS_TYPE mfcc_weights_bin37[] = {
    103,  // 1st valid tap index
    12,  // # of valid tap indexes
    7,51,95,139,183,226,244,201,159,117,76,35,
    0,0,0,
};

#define MFCC_BIN38_1ST_TAP 109
#define MFCC_BIN38_TAP_COUNT 16

static  MEL_WEIGHTS_TYPE mfcc_weights_bin38[] = {
    109,  // 1st valid tap index
    16,  // # of valid tap indexes
    12,55,97,139,180,221,250,209,169,129,89,50,11,
    0,0,0,
};

#define MFCC_BIN39_1ST_TAP 115
#define MFCC_BIN39_TAP_COUNT 16

static  MEL_WEIGHTS_TYPE mfcc_weights_bin39[] = {
    115,  // 1st valid tap index
    16,  // # of valid tap indexes
    6,47,87,127,167,206,245,228,189,151,113,75,37,
    0,0,0,
};


static  MEL_WEIGHTS_TYPE *mfcc_weights_bins[MFCC_FILTER_BANK_COUNT] = {
    mfcc_weights_bin0,
    mfcc_weights_bin1,
    mfcc_weights_bin2,
    mfcc_weights_bin3,
    mfcc_weights_bin4,
    mfcc_weights_bin5,
    mfcc_weights_bin6,
    mfcc_weights_bin7,
    mfcc_weights_bin8,
    mfcc_weights_bin9,
    mfcc_weights_bin10,
    mfcc_weights_bin11,
    mfcc_weights_bin12,
    mfcc_weights_bin13,
    mfcc_weights_bin14,
    mfcc_weights_bin15,
    mfcc_weights_bin16,
    mfcc_weights_bin17,
    mfcc_weights_bin18,
    mfcc_weights_bin19,
    mfcc_weights_bin20,
    mfcc_weights_bin21,
    mfcc_weights_bin22,
    mfcc_weights_bin23,
    mfcc_weights_bin24,
    mfcc_weights_bin25,
    mfcc_weights_bin26,
    mfcc_weights_bin27,
    mfcc_weights_bin28,
    mfcc_weights_bin29,
    mfcc_weights_bin30,
    mfcc_weights_bin31,
    mfcc_weights_bin32,
    mfcc_weights_bin33,
    mfcc_weights_bin34,
    mfcc_weights_bin35,
    mfcc_weights_bin36,
    mfcc_weights_bin37,
    mfcc_weights_bin38,
    mfcc_weights_bin39,
};

/*
 * DCT function is a matrix multiply of the mel powers [1xMFCC_FILTER_BANK_COUNT] and
 * a cosine function scaled for MEL_COEFFICIENT_COUNT [MFCC_FILTER_BANK_COUNTxMEL_COEFFICIENT_COUNT]
 * The function is simply cosine scaled between 0 and 1 (amplitude) and scaled
 * between 0 and Pi across the input.
 *
 * the format in the array is Q15.8
 *
 * 32 rows are provided for up to 32 coefficients, but not all have to be used.
 */

static  MEL_DCT_TYPE mel_dct_vectors[MFCC_DCT_ROW_COUNT][MFCC_FILTER_BANK_COUNT] = {
  {57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57},
  {57, 57, 56, 55, 54, 52, 50, 48, 45, 42, 39, 35, 32, 28, 24, 20, 16, 11, 7, 2, -2, -7, -11, -16, -20, -24, -28, -32, -35, -39, -42, -45, -48, -50, -52, -54, -55, -56, -57, -57},
  {57, 56, 53, 49, 44, 37, 30, 22, 13, 4, -4, -13, -22, -30, -37, -44, -49, -53, -56, -57, -57, -56, -53, -49, -44, -37, -30, -22, -13, -4, 4, 13, 22, 30, 37, 44, 49, 53, 56, 57},
  {57, 54, 48, 39, 28, 16, 2, -11, -24, -35, -45, -52, -56, -57, -55, -50, -42, -32, -20, -7, 7, 20, 32, 42, 50, 55, 57, 56, 52, 45, 35, 24, 11, -2, -16, -28, -39, -48, -54, -57},
  {57, 51, 40, 26, 9, -9, -26, -40, -51, -57, -57, -51, -40, -26, -9, 9, 26, 40, 51, 57, 57, 51, 40, 26, 9, -9, -26, -40, -51, -57, -57, -51, -40, -26, -9, 9, 26, 40, 51, 57},
  {56, 48, 32, 11, -11, -32, -48, -56, -56, -48, -32, -11, 11, 32, 48, 56, 56, 48, 32, 11, -11, -32, -48, -56, -56, -48, -32, -11, 11, 32, 48, 56, 56, 48, 32, 11, -11, -32, -48, -56},
  {56, 44, 22, -4, -30, -49, -57, -53, -37, -13, 13, 37, 53, 57, 49, 30, 4, -22, -44, -56, -56, -44, -22, 4, 30, 49, 57, 53, 37, 13, -13, -37, -53, -57, -49, -30, -4, 22, 44, 56},
  {55, 39, 11, -20, -45, -57, -52, -32, -2, 28, 50, 57, 48, 24, -7, -35, -54, -56, -42, -16, 16, 42, 56, 54, 35, 7, -24, -48, -57, -50, -28, 2, 32, 52, 57, 45, 20, -11, -39, -55},
  {54, 34, 0, -34, -54, -54, -34, 0, 34, 54, 54, 34, 0, -34, -54, -54, -34, 0, 34, 54, 54, 34, 0, -34, -54, -54, -34, 0, 34, 54, 54, 34, 0, -34, -54, -54, -34, 0, 34, 54},
  {54, 28, -11, -45, -57, -42, -7, 32, 55, 52, 24, -16, -48, -57, -39, -2, 35, 56, 50, 20, -20, -50, -56, -35, 2, 39, 57, 48, 16, -24, -52, -55, -32, 7, 42, 57, 45, 11, -28, -54},
  {53, 22, -22, -53, -53, -22, 22, 53, 53, 22, -22, -53, -53, -22, 22, 53, 53, 22, -22, -53, -53, -22, 22, 53, 53, 22, -22, -53, -53, -22, 22, 53, 53, 22, -22, -53, -53, -22, 22, 53},
  {52, 16, -32, -57, -42, 2, 45, 56, 28, -20, -54, -50, -11, 35, 57, 39, -7, -48, -55, -24, 24, 55, 48, 7, -39, -57, -35, 11, 50, 54, 20, -28, -56, -45, -2, 42, 57, 32, -16, -52},
  {51, 9, -40, -57, -26, 26, 57, 40, -9, -51, -51, -9, 40, 57, 26, -26, -57, -40, 9, 51, 51, 9, -40, -57, -26, 26, 57, 40, -9, -51, -51, -9, 40, 57, 26, -26, -57, -40, 9, 51},
  {50, 2, -48, -52, -7, 45, 54, 11, -42, -55, -16, 39, 56, 20, -35, -57, -24, 32, 57, 28, -28, -57, -32, 24, 57, 35, -20, -56, -39, 16, 55, 42, -11, -54, -45, 7, 52, 48, -2, -50},
  {49, -4, -53, -44, 13, 56, 37, -22, -57, -30, 30, 57, 22, -37, -56, -13, 44, 53, 4, -49, -49, 4, 53, 44, -13, -56, -37, 22, 57, 30, -30, -57, -22, 37, 56, 13, -44, -53, -4, 49},
  {48, -11, -56, -32, 32, 56, 11, -48, -48, 11, 56, 32, -32, -56, -11, 48, 48, -11, -56, -32, 32, 56, 11, -48, -48, 11, 56, 32, -32, -56, -11, 48, 48, -11, -56, -32, 32, 56, 11, -48},
  {46, -18, -57, -18, 46, 46, -18, -57, -18, 46, 46, -18, -57, -18, 46, 46, -18, -57, -18, 46, 46, -18, -57, -18, 46, 46, -18, -57, -18, 46, 46, -18, -57, -18, 46, 46, -18, -57, -18, 46},
  {45, -24, -56, -2, 55, 28, -42, -48, 20, 57, 7, -54, -32, 39, 50, -16, -57, -11, 52, 35, -35, -52, 11, 57, 16, -50, -39, 32, 54, -7, -57, -20, 48, 42, -28, -55, 2, 56, 24, -45},
  {44, -30, -53, 13, 57, 4, -56, -22, 49, 37, -37, -49, 22, 56, -4, -57, -13, 53, 30, -44, -44, 30, 53, -13, -57, -4, 56, 22, -49, -37, 37, 49, -22, -56, 4, 57, 13, -53, -30, 44},
  {42, -35, -48, 28, 52, -20, -55, 11, 57, -2, -57, -7, 56, 16, -54, -24, 50, 32, -45, -39, 39, 45, -32, -50, 24, 54, -16, -56, 7, 57, 2, -57, -11, 55, 20, -52, -28, 48, 35, -42},
  {40, -40, -40, 40, 40, -40, -40, 40, 40, -40, -40, 40, 40, -40, -40, 40, 40, -40, -40, 40, 40, -40, -40, 40, 40, -40, -40, 40, 40, -40, -40, 40, 40, -40, -40, 40, 40, -40, -40, 40},
  {39, -45, -32, 50, 24, -54, -16, 56, 7, -57, 2, 57, -11, -55, 20, 52, -28, -48, 35, 42, -42, -35, 48, 28, -52, -20, 55, 11, -57, -2, 57, -7, -56, 16, 54, -24, -50, 32, 45, -39},
  {37, -49, -22, 56, 4, -57, 13, 53, -30, -44, 44, 30, -53, -13, 57, -4, -56, 22, 49, -37, -37, 49, 22, -56, -4, 57, -13, -53, 30, 44, -44, -30, 53, 13, -57, 4, 56, -22, -49, 37},
  {35, -52, -11, 57, -16, -50, 39, 32, -54, -7, 57, -20, -48, 42, 28, -55, -2, 56, -24, -45, 45, 24, -56, 2, 55, -28, -42, 48, 20, -57, 7, 54, -32, -39, 50, 16, -57, 11, 52, -35},
  {34, -54, 0, 54, -34, -34, 54, 0, -54, 34, 34, -54, 0, 54, -34, -34, 54, 0, -54, 34, 34, -54, 0, 54, -34, -34, 54, 0, -54, 34, 34, -54, 0, 54, -34, -34, 54, 0, -54, 34},
  {32, -56, 11, 48, -48, -11, 56, -32, -32, 56, -11, -48, 48, 11, -56, 32, 32, -56, 11, 48, -48, -11, 56, -32, -32, 56, -11, -48, 48, 11, -56, 32, 32, -56, 11, 48, -48, -11, 56, -32},
  {30, -57, 22, 37, -56, 13, 44, -53, 4, 49, -49, -4, 53, -44, -13, 56, -37, -22, 57, -30, -30, 57, -22, -37, 56, -13, -44, 53, -4, -49, 49, 4, -53, 44, 13, -56, 37, 22, -57, 30},
  {28, -57, 32, 24, -57, 35, 20, -56, 39, 16, -55, 42, 11, -54, 45, 7, -52, 48, 2, -50, 50, -2, -48, 52, -7, -45, 54, -11, -42, 55, -16, -39, 56, -20, -35, 57, -24, -32, 57, -28},
  {26, -57, 40, 9, -51, 51, -9, -40, 57, -26, -26, 57, -40, -9, 51, -51, 9, 40, -57, 26, 26, -57, 40, 9, -51, 51, -9, -40, 57, -26, -26, 57, -40, -9, 51, -51, 9, 40, -57, 26},
  {24, -55, 48, -7, -39, 57, -35, -11, 50, -54, 20, 28, -56, 45, -2, -42, 57, -32, -16, 52, -52, 16, 32, -57, 42, 2, -45, 56, -28, -20, 54, -50, 11, 35, -57, 39, 7, -48, 55, -24},
  {22, -53, 53, -22, -22, 53, -53, 22, 22, -53, 53, -22, -22, 53, -53, 22, 22, -53, 53, -22, -22, 53, -53, 22, 22, -53, 53, -22, -22, 53, -53, 22, 22, -53, 53, -22, -22, 53, -53, 22},
  {20, -50, 56, -35, -2, 39, -57, 48, -16, -24, 52, -55, 32, 7, -42, 57, -45, 11, 28, -54, 54, -28, -11, 45, -57, 42, -7, -32, 55, -52, 24, 16, -48, 57, -39, 2, 35, -56, 50, -20},
};

/*
 * Quantization Scaling
 *
 * Output of the DCT rounding is a qx.7
 * For tflite this needs to be scaled according to the quantization parameters.
 * Those parameters are stored in the tflite file and are available at run time,
 * but for now we'll redefine them here.
 */

#define MFCC_QUANT_SCALING_FACTOR_INV_QP8 ((int32_t) (256 / 1.07999))
#define MFCC_QUANT_ZP_INV_QP8 ((int32_t) (256 * 256 * 101))

#endif
