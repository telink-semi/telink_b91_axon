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
#include "axon_mel32_weights.h"

static_assert(AXON_AUDIO_FEATURE_FILTERBANK_COUNT==32, "AXON_AUDIO_FEATURE_FILTERBANK_COUNT MUST BE 32!!!!");

/*
 * Window function is a simple vector multiply of the input vector
 * The function is (almost) simply cosine scaled between 0 and 1 (amplitude) and scaled
 * between 0 and Pi across the input.
 * For taps "n" from 0 to 511, Tap(n) = .54 * .46 * cos(2*Pi * n/511)
 *
 * the format in the array is Q15.8
 * In excel:
 * 0.54-0.46*COS(2*PI()*MOD(A505,512)/(512-1))
 */
const int32_t mel32_window[] = {
    20,20,21,21,21,21,21,21,21,21,21,22,22,22,22,22,23,23,23,24,24,24,25,25,26,26,26,27,27,28,28,29,29,30,31,31,32,32,33,34,34,35,36,37,37,38,39,40,40,41,42,43,44,45,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,63,64,65,66,67,68,69,71,72,73,74,75,77,78,79,80,82,83,84,86,87,88,89,91,92,93,95,96,97,99,100,102,103,104,106,107,109,110,111,113,114,116,117,118,120,121,123,124,126,127,128,130,131,133,134,136,137,139,140,141,143,144,146,147,149,150,152,153,154,156,157,159,160,162,163,164,166,167,169,170,171,173,174,176,177,178,180,181,182,184,185,186,188,189,190,192,193,194,195,197,198,199,200,202,203,204,205,206,208,209,210,211,212,213,214,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,231,232,233,234,235,236,236,237,238,239,240,240,241,242,242,243,244,244,245,246,246,247,247,248,248,249,249,250,250,251,251,252,252,252,253,253,253,254,254,254,254,255,255,255,255,255,255,256,256,256,256,256,256,256,256,256,256,256,256,256,256,255,255,255,255,255,255,254,254,254,254,253,253,253,252,252,252,251,251,250,250,249,249,248,248,247,247,246,246,245,244,244,243,242,242,241,240,240,239,238,237,236,236,235,234,233,232,231,231,230,229,228,227,226,225,224,223,222,221,220,219,218,217,216,214,213,212,211,210,209,208,206,205,204,203,202,200,199,198,197,195,194,193,192,190,189,188,186,185,184,182,181,180,178,177,176,174,173,171,170,169,167,166,164,163,162,160,159,157,156,154,153,152,150,149,147,146,144,143,141,140,139,137,136,134,133,131,130,128,127,126,124,123,121,120,118,117,116,114,113,111,110,109,107,106,104,103,102,100,99,97,96,95,93,92,91,89,88,87,86,84,83,82,80,79,78,77,75,74,73,72,71,69,68,67,66,65,64,63,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,45,44,43,42,41,40,40,39,38,37,37,36,35,34,34,33,32,32,31,31,30,29,29,28,28,27,27,26,26,26,25,25,24,24,24,23,23,23,22,22,22,22,22,21,21,21,21,21,21,21,21,21,20,20,
};

#define MEL32_BIN0_1ST_TAP 1
#define MEL32_BIN0_TAP_COUNT 4
#define MEL32_COEFS_BIN0     256,128, 0, 0,

#define MEL32_BIN1_1ST_TAP 2
#define MEL32_BIN1_TAP_COUNT 4
#define MEL32_COEFS_BIN1   128,256,128,0,

#define MEL32_BIN2_1ST_TAP 4
#define MEL32_BIN2_TAP_COUNT 4
#define MEL32_COEFS_BIN2     128,256,171,85,

#define MEL32_BIN3_1ST_TAP 6
#define MEL32_BIN3_TAP_COUNT 4
#define MEL32_COEFS_BIN3   85,171,256,128,

#define MEL32_BIN4_1ST_TAP 9
#define MEL32_BIN4_TAP_COUNT 4
#define MEL32_COEFS_BIN4   128,256,171,85,

#define MEL32_BIN5_1ST_TAP 11
#define MEL32_BIN5_TAP_COUNT 4
#define MEL32_COEFS_BIN5   85,171,256,128,

#define MEL32_BIN6_1ST_TAP 14
#define MEL32_BIN6_TAP_COUNT 4
#define MEL32_COEFS_BIN6   128,256,171,85,

#define MEL32_BIN7_1ST_TAP 16
#define MEL32_BIN7_TAP_COUNT 8
#define MEL32_COEFS_BIN7   85,171,256,192,128,64,0,0,

#define MEL32_BIN8_1ST_TAP 19
#define MEL32_BIN8_TAP_COUNT 8
#define MEL32_COEFS_BIN8   64,128,192,256,171,85,0,0,

#define MEL32_BIN9_1ST_TAP 23
#define MEL32_BIN9_TAP_COUNT 8
#define MEL32_COEFS_BIN9   85,171,256,192,128,64,0,0,

#define MEL32_BIN10_1ST_TAP 26
#define MEL32_BIN10_TAP_COUNT 8
#define MEL32_COEFS_BIN10   64,128,192,256,192,128,64,0,

#define MEL32_BIN11_1ST_TAP 30
#define MEL32_BIN11_TAP_COUNT 8
#define MEL32_COEFS_BIN11   64,128,192,256,205,154,102,51,

#define MEL32_BIN12_1ST_TAP 34
#define MEL32_BIN12_TAP_COUNT 8
#define MEL32_COEFS_BIN12   51,102,154,205,256,192,128,64,

#define MEL32_BIN13_1ST_TAP 39
#define MEL32_BIN13_TAP_COUNT 12
#define MEL32_COEFS_BIN13   64,128,192,256,213,171,128,85,43,0,0,0,

#define MEL32_BIN14_1ST_TAP 43
#define MEL32_BIN14_TAP_COUNT 12
#define MEL32_COEFS_BIN14   43,85,128,171,213,256,205,154,102,51,0,0,

#define MEL32_BIN15_1ST_TAP 49
#define MEL32_BIN15_TAP_COUNT 12
#define MEL32_COEFS_BIN15   51,102,154,205,256,213,171,128,85,43,0,0,

#define MEL32_BIN16_1ST_TAP 54
#define MEL32_BIN16_TAP_COUNT 12
#define MEL32_COEFS_BIN16   43,85,128,171,213,256,219,183,146,110,73,37,

#define MEL32_BIN17_1ST_TAP 60
#define MEL32_BIN17_TAP_COUNT 16
#define MEL32_COEFS_BIN17   37,73,110,146,183,219,256,219,183,146,110,73,37,0,0,0,

#define MEL32_BIN18_1ST_TAP 67
#define MEL32_BIN18_TAP_COUNT 16
#define MEL32_COEFS_BIN18   37,73,110,146,183,219,256,219,183,146,110,73,37,0,0,0,

#define MEL32_BIN19_1ST_TAP 74
#define MEL32_BIN19_TAP_COUNT 16
#define MEL32_COEFS_BIN19   37,73,110,146,183,219,256,228,199,171,142,114,85,57,28,0,

#define MEL32_BIN20_1ST_TAP 81
#define MEL32_BIN20_TAP_COUNT 16
#define MEL32_COEFS_BIN20   28,57,85,114,142,171,199,228,256,224,192,160,128,96,64,32,

#define MEL32_BIN21_1ST_TAP 90
#define MEL32_BIN21_TAP_COUNT 20
#define MEL32_COEFS_BIN21   32,64,96,128,160,192,224,256,230,205,179,154,128,102,77,51,26,0,0,0,

#define MEL32_BIN22_1ST_TAP 98
#define MEL32_BIN22_TAP_COUNT 20
#define MEL32_COEFS_BIN22   26,51,77,102,128,154,179,205,230,256,230,205,179,154,128,102,77,51,26,0,

#define MEL32_BIN23_1ST_TAP 108
#define MEL32_BIN23_TAP_COUNT 20
#define MEL32_COEFS_BIN23   26,51,77,102,128,154,179,205,230,256,233,209,186,163,140,116,93,70,47,23,

#define MEL32_BIN24_1ST_TAP 118
#define MEL32_BIN24_TAP_COUNT 24
#define MEL32_COEFS_BIN24   23,47,70,93,116,140,163,186,209,233,256,235,213,192,171,149,128,107,85,64,43,21,0,0,

#define MEL32_BIN25_1ST_TAP 129
#define MEL32_BIN25_TAP_COUNT 24
#define MEL32_COEFS_BIN25   21,43,64,85,107,128,149,171,192,213,235,256,236,217,197,177,158,138,118,98,79,59,39,20,

#define MEL32_BIN26_1ST_TAP 141
#define MEL32_BIN26_TAP_COUNT 28
#define MEL32_COEFS_BIN26   20,39,59,79,98,118,138,158,177,197,217,236,256,238,219,201,183,165,146,128,110,91,73,55,37,18,0,0,

#define MEL32_BIN27_1ST_TAP 154
#define MEL32_BIN27_TAP_COUNT 32
#define MEL32_COEFS_BIN27   18,37,55,73,91,110,128,146,165,183,201,219,238,256,240,224,208,192,176,160,144,128,112,96,80,64,48,32,16,0,0,0,

#define MEL32_BIN28_1ST_TAP 168
#define MEL32_BIN28_TAP_COUNT 32
#define MEL32_COEFS_BIN28   16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,256,240,224,208,192,176,160,144,128,112,96,80,64,48,32,16,0,

#define MEL32_BIN29_1ST_TAP 184
#define MEL32_BIN29_TAP_COUNT 32
#define MEL32_COEFS_BIN29   16,32,48,64,80,96,112,128,144,160,176,192,208,224,240,256,241,226,211,196,181,166,151,136,120,105,90,75,60,45,30,15,

#define MEL32_BIN30_1ST_TAP 200
#define MEL32_BIN30_TAP_COUNT 36
#define MEL32_COEFS_BIN30  15,30,45,60,75,90,105,120,136,151,166,181,196,211,226,241,256,243,229,216,202,189,175,162,148,135,121,108,94,81,67,54,40,27,13,0,

#define MEL32_BIN31_1ST_TAP 217
#define MEL32_BIN31_TAP_COUNT 40
#define MEL32_COEFS_BIN31  13,27,40,54,67,81,94,108,121,135,148,162,175,189,202,216,229,243,256,244,232,219,207,195,183,171,158,146,134,122,110,98,85,73,61,49,37,24,12,0,

// GROUP 0 (248 Coefficients Total)
#define MEL32_COEFF_OFFSET_BIN0 0
#define MEL32_COEFF_OFFSET_BIN1 (MEL32_COEFF_OFFSET_BIN0 +MEL32_BIN0_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN2 (MEL32_COEFF_OFFSET_BIN1 +MEL32_BIN1_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN3 (MEL32_COEFF_OFFSET_BIN2 +MEL32_BIN2_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN4 (MEL32_COEFF_OFFSET_BIN3 +MEL32_BIN3_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN5 (MEL32_COEFF_OFFSET_BIN4 +MEL32_BIN4_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN6 (MEL32_COEFF_OFFSET_BIN5 +MEL32_BIN5_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN7 (MEL32_COEFF_OFFSET_BIN6 +MEL32_BIN6_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN8 (MEL32_COEFF_OFFSET_BIN7 +MEL32_BIN7_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN9 (MEL32_COEFF_OFFSET_BIN8 +MEL32_BIN8_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN10 (MEL32_COEFF_OFFSET_BIN9 +MEL32_BIN9_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN11 (MEL32_COEFF_OFFSET_BIN10 +MEL32_BIN10_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN12 (MEL32_COEFF_OFFSET_BIN11 +MEL32_BIN11_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN13 (MEL32_COEFF_OFFSET_BIN12 +MEL32_BIN12_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN14 (MEL32_COEFF_OFFSET_BIN13 +MEL32_BIN13_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN15 (MEL32_COEFF_OFFSET_BIN14 +MEL32_BIN14_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN16 (MEL32_COEFF_OFFSET_BIN15 +MEL32_BIN15_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN17 (MEL32_COEFF_OFFSET_BIN16 +MEL32_BIN16_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN18 (MEL32_COEFF_OFFSET_BIN17 +MEL32_BIN17_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN19 (MEL32_COEFF_OFFSET_BIN18 +MEL32_BIN18_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN30 (MEL32_COEFF_OFFSET_BIN19 +MEL32_BIN19_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN31 (MEL32_COEFF_OFFSET_BIN30 +MEL32_BIN30_TAP_COUNT)

// GROUP 1 (248 Coefficients Total)
#define MEL32_COEFF_OFFSET_BIN20 0
#define MEL32_COEFF_OFFSET_BIN21 (MEL32_COEFF_OFFSET_BIN20 +MEL32_BIN20_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN22 (MEL32_COEFF_OFFSET_BIN21 +MEL32_BIN21_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN23 (MEL32_COEFF_OFFSET_BIN22 +MEL32_BIN22_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN24 (MEL32_COEFF_OFFSET_BIN23 +MEL32_BIN23_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN25 (MEL32_COEFF_OFFSET_BIN24 +MEL32_BIN24_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN26 (MEL32_COEFF_OFFSET_BIN25 +MEL32_BIN25_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN27 (MEL32_COEFF_OFFSET_BIN26 +MEL32_BIN26_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN28 (MEL32_COEFF_OFFSET_BIN27 +MEL32_BIN27_TAP_COUNT)
#define MEL32_COEFF_OFFSET_BIN29 (MEL32_COEFF_OFFSET_BIN28 +MEL32_BIN28_TAP_COUNT)

// number of operations is group1
#define MEL32_COEFS_GROUP1_OP_CNT  22
// coefficients in group1 all joined together
static const int32_t mel32_coefs_group1[] = {
    MEL32_COEFS_BIN0 MEL32_COEFS_BIN1 MEL32_COEFS_BIN2 MEL32_COEFS_BIN3 MEL32_COEFS_BIN4 MEL32_COEFS_BIN5
    MEL32_COEFS_BIN6 MEL32_COEFS_BIN7 MEL32_COEFS_BIN8 MEL32_COEFS_BIN9 MEL32_COEFS_BIN10 MEL32_COEFS_BIN11
    MEL32_COEFS_BIN12 MEL32_COEFS_BIN13 MEL32_COEFS_BIN14 MEL32_COEFS_BIN15 MEL32_COEFS_BIN16 MEL32_COEFS_BIN17
    MEL32_COEFS_BIN18 MEL32_COEFS_BIN19 MEL32_COEFS_BIN30 MEL32_COEFS_BIN31
};

// number of operations is group2
#define MEL32_COEFS_GROUP2_OP_CNT  10
// coefficients in group2 all joined together
static const int32_t mel32_coefs_group2[] = {
    MEL32_COEFS_BIN20 MEL32_COEFS_BIN21 MEL32_COEFS_BIN22 MEL32_COEFS_BIN23
    MEL32_COEFS_BIN24 MEL32_COEFS_BIN25 MEL32_COEFS_BIN26 MEL32_COEFS_BIN27 MEL32_COEFS_BIN28 MEL32_COEFS_BIN29
};

/*
 * DCT table.
 * Matches SciPy type 2, orthogonal
 */
static const int32_t mel_dct_vectors[MFCC_FEATURE_COUNT][AXON_AUDIO_FEATURE_FILTERBANK_COUNT] = {
    {181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181},
    {256,253,248,241,231,220,206,190,172,152,132,109,86,62,38,13,-13,-38,-62,-86,-109,-132,-152,-172,-190,-206,-220,-231,-241,-248,-253,-256},
    {255,245,226,198,162,121,74,25,-25,-74,-121,-162,-198,-226,-245,-255,-255,-245,-226,-198,-162,-121,-74,-25,25,74,121,162,198,226,245,255},
    {253,231,190,132,62,-13,-86,-152,-206,-241,-256,-248,-220,-172,-109,-38,38,109,172,220,248,256,241,206,152,86,13,-62,-132,-190,-231,-253},
    {251,213,142,50,-50,-142,-213,-251,-251,-213,-142,-50,50,142,213,251,251,213,142,50,-50,-142,-213,-251,-251,-213,-142,-50,50,142,213,251},
    {248,190,86,-38,-152,-231,-256,-220,-132,-13,109,206,253,241,172,62,-62,-172,-241,-253,-206,-109,13,132,220,256,231,152,38,-86,-190,-248},
    {245,162,25,-121,-226,-255,-198,-74,74,198,255,226,121,-25,-162,-245,-245,-162,-25,121,226,255,198,74,-74,-198,-255,-226,-121,25,162,245},
    {241,132,-38,-190,-256,-206,-62,109,231,248,152,-13,-172,-253,-220,-86,86,220,253,172,13,-152,-248,-231,-109,62,206,256,190,38,-132,-241},
    {237,98,-98,-237,-237,-98,98,237,237,98,-98,-237,-237,-98,98,237,237,98,-98,-237,-237,-98,98,237,237,98,-98,-237,-237,-98,98,237},
    {231,62,-152,-256,-172,38,220,241,86,-132,-253,-190,13,206,248,109,-109,-248,-206,-13,190,253,132,-86,-241,-220,-38,172,256,152,-62,-231},
};
