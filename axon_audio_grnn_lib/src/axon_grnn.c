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
#include <assert.h>
#include "axon_api.h"
#include "axon_dep.h"
// #include "axon_grnn_weights.h"
#include "axon_grnn.h"
#include "axon_logging_api.h"

// #include "axon_logging.h"

/*
 * These are the weights and biases needed by grnn. They will be specific to the training set.
 */
extern const grnn_weight_type SIGMOID_NU_PLUS_ZETA_1Q15;
extern const grnn_weight_type MINUS_SIGMOID_ZETA_1Q7;
extern const uint8_t kGrnnInputFcWeightsQ;
extern grnn_weight_type GRNN_INPUT_FC_WEIGHTS[GRNN_INPUT_WT_HT][GRNN_INPUT_HT];
extern grnn_weight_type GRNN_INPUT_FC_BIAS[GRNN_INPUT_WT_HT];
extern const uint8_t kGrnnHiddenFcWeightsQ;
extern grnn_weight_type GRNN_HIDDEN_FC_WEIGHTS[GRNN_HIDDEN_WT_HT][GRNN_HIDDEN_HT];
extern grnn_weight_type GRNN_HIDDEN_FC_BIAS[GRNN_HIDDEN_WT_HT];
extern grnn_weight_type GRNN_FINAL_FC_WEIGHTS[GRNN_FINAL_FC_HEIGHT][GRNN_FINAL_FC_WIDTH];
extern grnn_weight_type GRNN_FINAL_FC_BIAS[GRNN_FINAL_FC_HEIGHT];

#define DEBUG_VECTORS_GRNN_UNIT 0 // 1 prints input at beginning and h(t) at end. 2 prints all intermediate calculations.
#define DEBUG_VECTORS_GRNN_FINAL 0

/*
 * Operations performed per-frame.
 */
typedef enum {
  kGrnnAxonPerFrameOpFirst,
  kGrnnAxonOpInputWeightsMatrixMult = kGrnnAxonPerFrameOpFirst,   // multiply input and input weights into buff_z[]
  kGrnnAxonOpHiddenWeightsMatrixMult,   // multiply hidden and hidden weights into buff_h_hat[]
  kGrnnAxonOpInputPlusHiddenXpy,       // add the results of previous 2 ops into Tmp
  kGrnnAxonOpMemCpyBg,
  kGrnnAxonOpAddInputBiasXpySigmoid,       // add input bias to previous and take sigmoid. Result is Z(t) in buff_z[]
  kGrnnAxonOpMemCpyBh,
  kGrnnAxonOpAddHiddenBiasXpySigmoid,       // add hidden bias to previous and take sigmoid. Result is h_hat(t) in buff_h_hat[]
  kGrnnAxonOpHiddenTimesZXty,   // multiply Zt(in buff_z) with ht-1 (in buff_h), store in buff_h[].
  kGrnnAxonOp1MinusZAxpb,   // multiply Zt(in buff_z) by -1(A) and add 1(B), store in buff_z[]
  kGrnnAxonOp1MinusZTimesHXty, // multiply 1-Zt (in buff_z) by h_hat (in buff_h_hat) and store in buff_z.
  kGrnnAxonOpZtimesHPlus1minusZTimesHhatXpy, // add (1-Zt)*h_hat (in buff_z) to Z*Ht-1 (in buff_h), final h stored in buff_h
  kGrnnAxonPerFrameOpCount, // 9 operations
} GrnnAxonPerFrameOperationEnum;

#define IS_SLICE_MEMCPY_OP(OP_NDX) ((OP_NDX==kGrnnAxonOpMemCpyBg)||(OP_NDX==kGrnnAxonOpMemCpyBh))
/*
 * Final classification operations
 */
typedef enum {
  kGrnnAxonOpFinalFirst,
  kGrnnAxonOpFinalWeightsMatrixMult = kGrnnAxonOpFinalFirst, // multiply final h stored in buff_h with GRNN_FINAL_FC_WEIGHTS into buff_final_outputs
  kGrnnAxonOpFinalMemCpyBf,
  kGrnnAxonOpFinalBiasesXpy,                                 // add biases in GRNN_FINAL_FC_BIAS to result of prior op.
  kGrnnAxonOpFinalCount //
} GrnnAxonFinalOperationEnum;

#define IS_FINAL_MEMCPY_OP(OP_NDX) (OP_NDX==kGrnnAxonOpFinalMemCpyBf)


#ifndef RETAINED_MEMORY_SECTION_ATTRIBUTE
static_assert(0, "PLEASE DEFINE RETAINED_MEMORY_SECTION_ATTRIBUTE IN BUILD SYSTEM");
#endif

/*
 * axon operation handles need to be stored in retained memory.
 */
RETAINED_MEMORY_SECTION_ATTRIBUTE
static AxonOpHandle grnn_perframe_op_handles[kGrnnAxonPerFrameOpCount];

RETAINED_MEMORY_SECTION_ATTRIBUTE
static AxonOpHandle grnn_final_op_handles[kGrnnAxonOpFinalCount];


static grnn_weight_type _Alignas(8) buff_bias[GRNN_HIDDEN_HT]; // holds biases(Bg, Bh, Bf)
static grnn_weight_type _Alignas(8) buff_h[GRNN_HIDDEN_HT]; // holds hidden layer between calls
static grnn_weight_type _Alignas(8) buff_i[GRNN_INPUT_HT]; // holds input layer
static grnn_weight_type _Alignas(8) buff_z[GRNN_HIDDEN_HT]; // holds Z layer
static grnn_weight_type _Alignas(8) buff_h_hat[GRNN_HIDDEN_HT]; // holds h_hat layer
static grnn_weight_type _Alignas(8) buff_tmp[GRNN_HIDDEN_HT]; // temporary buffer
static grnn_weight_type _Alignas(8) buff_final_outputs[GRNN_CLASS_COUNT]; // holds the final result

RETAINED_MEMORY_SECTION_ATTRIBUTE
static struct {
  void *axon_handle;
  AxonResultEnum result;
  void (*result_callback_function)(AxonResultEnum result);
  uint8_t slice_count; // total number of slices to process
  uint8_t slice_ndx; // current slice being processed.
}grnn_state_info;

/*
 * queued ops struct doesn't need to be
 * in retained memory.
 */
static AxonMgrQueuedOpsStruct grnn_queued_ops;

static uint32_t max_in_array(grnn_weight_type *array, uint32_t size, int32_t *margin)
{
  uint32_t i, max_index;
  int32_t max;
  int32_t max_2nd;

  max = array[0];
  max_2nd = array[0];

  max_index = 0;
  for (i=1; i<size; i++)
  {
    if (array[i] > max) {
      max_2nd = max;
      max = array[i];
      max_index = i;
    } else if (array[i] > max_2nd) {
      max_2nd=array[i];
    }
  }
  if (margin != NULL) {
    *margin = max-max_2nd;
  }

  return max_index;
}

/*
 * Define all the per-frame and final operations operations.
 * Handles will be placed in grnn_perframe_op_handles
 */
static AxonResultEnum axon_grnn_define_ops() {
  AxonResultEnum result;
  AxonInputStruct axon_input;

  /*
   * Common settings are width, stride, and AF, and dimensions.
   */
  axon_input.data_packing = kAxonDataPackingEnabled;
  axon_input.data_width = kAxonDataWidth16;
  axon_input.output_af = kAxonAfDisabled;
  axon_input.x_stride = kAxonStride1;
  axon_input.y_stride = kAxonStride1;
  axon_input.q_stride = kAxonStride1;

  /*
   * kGrnnAxonOpInputWeightsMatrixMult
   * MatrixMultiply of inputs (buff_i) and input weights (GRNN_INPUT_FC_WEIGHTS), stored in buff_z
   */
  axon_input.data_width = kAxonDataWidth8to16;
  axon_input.length = GRNN_INPUT_HT;
  axon_input.y_length = GRNN_INPUT_WT_HT;
  axon_input.x_in = (int32_t*)buff_i; // q4.11
  axon_input.y_in = (int32_t*)GRNN_INPUT_FC_WEIGHTS; //q1.GRNN_INPUT_FC_WEIGHTS_Q
  axon_input.output_rounding = kAxonRoundingNone+kGrnnInputFcWeightsQ;
  axon_input.q_out = (int32_t*)buff_z; // q4.11
  if (kAxonResultSuccess > (result=AxonApiDefineOpMatrixMult(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOpInputWeightsMatrixMult]))) {
    return result;
  }

  /*
   * kGrnnAxonOpHiddenWeightsMatrixMult
   * MatrixMultiply of hidden (buff_h) and hidden weights (GRNN_INPUT_FC_WEIGHTS), stored in buff_h_hat
   */
  axon_input.data_width = kAxonDataWidth8to16;
  axon_input.length = GRNN_HIDDEN_HT; // Height of X, width of Y
  axon_input.y_length = GRNN_HIDDEN_HT;      // Height of Y, length of q
  axon_input.x_in = (int32_t*)buff_h; // q4.11
  axon_input.y_in = (int32_t*)GRNN_HIDDEN_FC_WEIGHTS; //q0.GRNN_HIDDEN_FC_WEIGHTS_Q
  axon_input.output_rounding = kAxonRoundingNone+kGrnnHiddenFcWeightsQ;
  axon_input.q_out = (int32_t*)buff_h_hat; //q4.11
  if (kAxonResultSuccess > (result=AxonApiDefineOpMatrixMult(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOpHiddenWeightsMatrixMult]))) {
    return result;
  }

  /*
   * kGrnnAxonOpInputPlusHiddenXpy,       // add the results of previous 2 ops into buff_tmp
   */
  axon_input.data_width = kAxonDataWidth16;
  axon_input.length = GRNN_HIDDEN_HT;
  axon_input.x_in = (int32_t*)buff_z; // q4.11
  axon_input.y_in = (int32_t*)buff_h_hat; // q4.11
  axon_input.output_rounding = kAxonRoundingNone;
  axon_input.q_out = (int32_t*)buff_tmp; // q5.11
  if (kAxonResultSuccess > (result=AxonApiDefineOpXpy(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOpInputPlusHiddenXpy]))) {
    return result;
  }

  //insert pseudo ops to copy Bg from FLASH to RAM
  axon_input.data_width = kAxonDataWidth16;
  axon_input.data_packing = kAxonDataPackingEnabled;
  axon_input.x_in = (int32_t*)GRNN_INPUT_FC_BIAS;
  axon_input.q_out = (int32_t*)buff_bias;
  axon_input.length = GRNN_HIDDEN_HT;
  axon_input.y_length = 0; //num of padding to make the total length multiply of 2 or 4 required by the following axon operations.
  // populate the op_handle
  if (kAxonResultSuccess > (result = AxonApiDefineOpMemCpy(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOpMemCpyBg]))) {
    return result;
  }

  /*
   * kGrnnAxonOpAddInputBiasXpySigmoid,       // add input bias to previous and take sigmoid. Result is Z(t) in buff_z[]
   */
  axon_input.data_width = kAxonDataWidth16;
  axon_input.length = GRNN_HIDDEN_HT;
  axon_input.x_in = (int32_t*)buff_tmp; // q4.11
  axon_input.y_in = (int32_t*)buff_bias; // q4.11
  axon_input.output_rounding = kAxonRoundingNone+3;
  axon_input.output_af = kAxonAfQuantSigmoid;
  axon_input.q_out = (int32_t*)buff_z; // q1.8 => sigmoid expects a q7.8, returns a q1.8
  if (kAxonResultSuccess > (result=AxonApiDefineOpXpy(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOpAddInputBiasXpySigmoid]))) {
    return result;
  }

  //insert pseudo ops to copy Bh from FLASH to RAM
  axon_input.data_width = kAxonDataWidth16;
  axon_input.data_packing = kAxonDataPackingEnabled;
  axon_input.x_in = (int32_t*)GRNN_HIDDEN_FC_BIAS;
  axon_input.q_out = (int32_t*)buff_bias;
  axon_input.length = GRNN_HIDDEN_HT;
  axon_input.y_length = 0; //num of padding to make the total length multiply of 2 or 4 required by the following axon operations.
  // populate the op_handle
  if (kAxonResultSuccess > (result = AxonApiDefineOpMemCpy(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOpMemCpyBh]))) {
    return result;
  }

  /*
   * kGrnnAxonOpAddHiddenBiasXpySigmoid,       // add hidden bias to previous and take sigmoid. Result is h_hat(t) in buff_h_hat[]
   */
  axon_input.data_width = kAxonDataWidth16;
  axon_input.length = GRNN_HIDDEN_HT;
  axon_input.x_in = (int32_t*)buff_tmp; // q4.11
  axon_input.y_in = (int32_t*)buff_bias; // q4.11
  axon_input.output_rounding = kAxonRoundingNone+3;
  axon_input.output_af = kAxonAfQuantSigmoid;
  axon_input.q_out = (int32_t*)buff_h_hat; // q1.8 => sigmoid expects a q7.8, returns a q1.8
  if (kAxonResultSuccess > (result=AxonApiDefineOpXpy(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOpAddHiddenBiasXpySigmoid]))) {
    return result;
  }

  /*
   * kGrnnAxonOpHiddenTimesZXty,   // multiply Zt(in buff_z) with ht-1 (in buff_h), store in buff_h[].
   */
  axon_input.data_width = kAxonDataWidth16;
  axon_input.length = GRNN_HIDDEN_HT;
  axon_input.x_in = (int32_t*)buff_z; // q1.8
  axon_input.y_in = (int32_t*)buff_h; // q4.11
  axon_input.output_rounding = kAxonRoundingNone+8;
  axon_input.output_af = kAxonAfDisabled; // NO MORE SIGMOID
  axon_input.q_out = (int32_t*)buff_h; // q5.11
  if (kAxonResultSuccess > (result=AxonApiDefineOpXty(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOpHiddenTimesZXty]))) {
    return result;
  }

  /*
   * kGrnnAxonOp1MinusZAxpb,   // multiply Zt(in buff_z) by -SIGMOID_ZETA (A) and add SIGMOID_ZETA+SIGMOID_NU(B), store in buff_z[]
   *
   */
  axon_input.data_width = kAxonDataWidth16;
  axon_input.length = GRNN_HIDDEN_HT;
  axon_input.x_in = (int32_t*)buff_z; // q1.8
  axon_input.a_in = MINUS_SIGMOID_ZETA_1Q7;   // Q1.7
  axon_input.b_in = SIGMOID_NU_PLUS_ZETA_1Q15;  // q1.15
  axon_input.output_rounding = kAxonRoundingNone+7; // round qx.15 to qx.8
  axon_input.q_out = (int32_t*)buff_z; // q2.8
  if (kAxonResultSuccess > (result=AxonApiDefineOpAxpb(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOp1MinusZAxpb]))) {
    return result;
  }

  /*
   * kGrnnAxonOp1MinusZTimesHXty, // multiply 1-Zt (in buff_z) by h_hat (in buff_h_hat) and store in buff_z.
   */
  axon_input.data_width = kAxonDataWidth16;
  axon_input.length = GRNN_HIDDEN_HT;
  axon_input.x_in = (int32_t*)buff_z; // q1.8
  axon_input.y_in = (int32_t*)buff_h_hat; // q1.8
  axon_input.output_rounding = kAxonRoundingNone+5;
  axon_input.q_out = (int32_t*)buff_z; // q2.11
  if (kAxonResultSuccess > (result=AxonApiDefineOpXty(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOp1MinusZTimesHXty]))) {
    return result;
  }

  /*
   * kGrnnAxonOpZtimesHPlus1minusZTimesHhatXpy, // add (1-Zt)*h_hat (in buff_z) to Z*Ht-1 (in buff_h), final h stored in buff_h
   */
  axon_input.data_width = kAxonDataWidth16;
  axon_input.length = GRNN_HIDDEN_HT;
  axon_input.x_in = (int32_t*)buff_z; // q3.11
  axon_input.y_in = (int32_t*)buff_h; // q4.11
  axon_input.output_rounding = kAxonRoundingNone;
  axon_input.q_out = (int32_t*)buff_h; // q4.11
  if (kAxonResultSuccess > (result=AxonApiDefineOpXpy(grnn_state_info.axon_handle, &axon_input, &grnn_perframe_op_handles[kGrnnAxonOpZtimesHPlus1minusZTimesHhatXpy]))) {
    return result;
  }

  /*
   * Done w/ the per-frame operations, define the final operations.
   */

  /*
   *  kGrnnAxonOpFinalWeightsMatrixMult, multiply final h stored in buff_h with GRNN_FINAL_FC_WEIGHTS into buff_final_outputs
   */
  axon_input.data_width = kAxonDataWidth8to16;
  axon_input.length = GRNN_HIDDEN_HT;
  axon_input.y_length = GRNN_FINAL_FC_HEIGHT;
  axon_input.x_in = (int32_t*)buff_h; // q4.11
  axon_input.y_in = (int32_t*)GRNN_FINAL_FC_WEIGHTS; //q2.5
  axon_input.output_rounding = kAxonRoundingNone+GRNN_FINAL_FC_WEIGHT_RIGHT_SHIFT;
  axon_input.q_out = (int32_t*)buff_final_outputs; // q5.6
  if (kAxonResultSuccess > (result=AxonApiDefineOpMatrixMult(grnn_state_info.axon_handle, &axon_input, &grnn_final_op_handles[kGrnnAxonOpFinalWeightsMatrixMult]))) {
    return result;
  }

  //insert pseudo ops to copy GRNN_FINAL_FC_BIAS from FLASH to RAM
  axon_input.data_width = kAxonDataWidth16;
  axon_input.data_packing = kAxonDataPackingEnabled;
  axon_input.x_in = (int32_t*)GRNN_FINAL_FC_BIAS;
  axon_input.q_out = (int32_t*)buff_bias;
  axon_input.length = GRNN_FINAL_FC_HEIGHT;
  axon_input.y_length = 0; //num of padding to make the total length multiply of 2 or 4 required by the following axon operations.
  // populate the op_handle
  if (kAxonResultSuccess > (result = AxonApiDefineOpMemCpy(grnn_state_info.axon_handle, &axon_input, &grnn_final_op_handles[kGrnnAxonOpFinalMemCpyBf]))) {
    return result;
  }

  /*
   * kGrnnAxonOpFinalBiasesXpy, add biases in GRNN_FINAL_FC_BIAS to result of prior op.
   */
  axon_input.data_width = kAxonDataWidth16;
  axon_input.length = GRNN_FINAL_FC_HEIGHT;
  axon_input.x_in = (int32_t*)buff_final_outputs; // q5.6
  axon_input.y_in = (int32_t*)buff_bias; //q1.6
  axon_input.output_rounding = kAxonRoundingNone;
  axon_input.q_out = (int32_t*)buff_final_outputs; // q5.6
  if (kAxonResultSuccess > (result=AxonApiDefineOpXpy(grnn_state_info.axon_handle, &axon_input, &grnn_final_op_handles[kGrnnAxonOpFinalBiasesXpy]))) {
    return result;
  }

  return result;
}

/*
 * API called once at start up
 */
AxonResultEnum AxonKwsModelGrnnPrepare(void *axon_handle, void (*result_callback_function)(AxonResultEnum result)) {
  grnn_state_info.result_callback_function = result_callback_function;
  grnn_state_info.axon_handle = axon_handle;

  return axon_grnn_define_ops();
}

/*
 * forward declaration
 */
static void  grnn_slice_ops_done_callback(AxonResultEnum result, void *callback_context);
/*
 * API function to calculate the hidden vector for each audio frame.
 */
AxonResultEnum grnn_process_frame() {
    const AudioInputFeatureType *audio_features_in;
    // get the slice data
    AxonKwsHostGetNextAudioFeatureSlice(&audio_features_in);

    memcpy(buff_i, audio_features_in, GRNN_INPUT_HT * sizeof(AudioInputFeatureType) );


#if (DEBUG_VECTORS_GRNN_UNIT > 0)
  print_int16_vector(grnn_state_info.axon_handle, "normalized audio_features_input", buff_i, GRNN_INPUT_HT, 1);
#endif

  // run the operations
#if DEBUG_VECTORS_GRNN_UNIT > 1
  AxonResultEnum result;
  print_int16_vector(grnn_state_info.axon_handle, "h(t-1)", buff_h, GRNN_HIDDEN_HT, 1);
  /*
   * DEBUG! ONE AT A TIME SO WE CAN EXAMINE RESULTS!
   */
  uint8_t op_cnt;
  for (uint32_t ndx=kGrnnAxonPerFrameOpFirst; ndx<kGrnnAxonPerFrameOpCount; ndx+=op_cnt) {
    // either execute 1 or 2 operations. 2 operations if the 1st op is a memcpy
    op_cnt = IS_SLICE_MEMCPY_OP(ndx) ? 2 : 1;
    /*
     * the very last op needs to be in queued batch mode, so that on-complete callback gets called.
     * It will log the result vector seaparately.
     */
    if ((ndx+op_cnt)==kGrnnAxonPerFrameOpCount) {
      // last operation, queue it up.
      grnn_queued_ops.op_handle_list = grnn_perframe_op_handles + ndx;
      grnn_queued_ops.callback_function = grnn_slice_ops_done_callback;
      grnn_queued_ops.callback_context = NULL;
      grnn_queued_ops.op_handle_count = 1;
      return AxonApiQueueOpsList(grnn_state_info.axon_handle,&grnn_queued_ops);
    }

    // intermediate operation, execute synchronously
    if (kAxonResultSuccess>(result=AxonApiExecuteOps(grnn_state_info.axon_handle, op_cnt, grnn_perframe_op_handles + ndx, kAxonAsyncModeSynchronous))) {
      return result; // error!
    }

    // print the results of each operation so they can be compared to ground truth.
    switch (ndx) {
    case kGrnnAxonOpInputWeightsMatrixMult:
      print_int16_vector(grnn_state_info.axon_handle, "X(t) dot Wf", buff_z, GRNN_INPUT_WT_HT, 1); break;
    case kGrnnAxonOpHiddenWeightsMatrixMult:
      print_int16_vector(grnn_state_info.axon_handle, "h(t-1) dot Wh", buff_h_hat, GRNN_HIDDEN_WT_HT, 1); break;
    case kGrnnAxonOpInputPlusHiddenXpy:
      print_int16_vector(grnn_state_info.axon_handle, "Input+Hidden", buff_tmp, GRNN_HIDDEN_WT_HT, 1); break;
    case kGrnnAxonOpAddInputBiasXpySigmoid:
      print_int16_vector(grnn_state_info.axon_handle, "Z(t)", buff_z, GRNN_HIDDEN_WT_HT, 1); break;
    case kGrnnAxonOpAddHiddenBiasXpySigmoid:
      print_int16_vector(grnn_state_info.axon_handle, "h_hat(t)", buff_h_hat, GRNN_HIDDEN_WT_HT, 1); break;
    case kGrnnAxonOpHiddenTimesZXty:
      print_int16_vector(grnn_state_info.axon_handle, "Z(t) x h(t-1)", buff_h, GRNN_HIDDEN_WT_HT, 1); break;
    case kGrnnAxonOp1MinusZAxpb:
      print_int16_vector(grnn_state_info.axon_handle, "1-Z(t)", buff_z, GRNN_HIDDEN_WT_HT, 1); break;
    case kGrnnAxonOp1MinusZTimesHXty:
      print_int16_vector(grnn_state_info.axon_handle, "(1-Z(t)) x h_hat", buff_z, GRNN_HIDDEN_WT_HT, 1); break;
    case kGrnnAxonOpZtimesHPlus1minusZTimesHhatXpy:
      print_int16_vector(grnn_state_info.axon_handle, "(Z x h(t-1)) + ((1-Z(t) x h_hat)", buff_h, GRNN_HIDDEN_WT_HT, 1); break;
    default:
      // AxonHostLog(grnn_state_info.axon_handle, "unexpected grnn op!");
      break;
    }

  }
  return result;
#else
  // do them all at once
  grnn_queued_ops.op_handle_list = grnn_perframe_op_handles + kGrnnAxonPerFrameOpFirst;
  grnn_queued_ops.callback_function = grnn_slice_ops_done_callback;
  grnn_queued_ops.callback_context = NULL;
  grnn_queued_ops.op_handle_count = kGrnnAxonPerFrameOpCount;
  return AxonApiQueueOpsList(grnn_state_info.axon_handle, &grnn_queued_ops);

#endif

}
/*
 * api function
 */
AxonResultEnum AxonKwsModelGrnnInfer(uint8_t slice_count) {
  grnn_state_info.slice_count = slice_count;
  grnn_state_info.slice_ndx = 0;

  // clear out the hidden vector before starting.
  memset(buff_h, 0, sizeof(buff_h));

  return grnn_process_frame();
}


static void  grnn_result_ops_done_callback(AxonResultEnum result, void *callback_context) {
  // call the callers callback
  grnn_state_info.result_callback_function(result);
}


/*
 * start final classification after all slices have been processed.
 */
static AxonResultEnum grnn_calculate_results() {
  // run the operations
#if DEBUG_VECTORS_GRNN_FINAL > 1
  /*
   * DEBUG! ONE AT A TIME SO WE CAN EXAMINE RESULTS!
   */
  uint8_t op_cnt;
  for (uint32_t ndx=kGrnnAxonOpFinalFirst; ndx<kGrnnAxonOpFinalCount; ndx+=op_cnt) {
    /*
     * the very last op needs to be in queued batch mode, so that on-complete callback gets called.
     * It will log the result vector seaparately.
     */
    // either execute 1 or 2 operations. 2 operations if the 1st op is a memcpy
    op_cnt = IS_FINAL_MEMCPY_OP(ndx) ? 2 : 1;

    if ((ndx+op_cnt)==kGrnnAxonOpFinalCount) {
      // last operartion, queue it up.
      grnn_queued_ops.op_handle_list = grnn_perframe_op_handles + ndx;
      grnn_queued_ops.callback_function = grnn_result_ops_done_callback;
      grnn_queued_ops.callback_context = NULL;
      grnn_queued_ops.op_handle_count = op_cnt;
      return AxonApiQueueOpsList(grnn_state_info.axon_handle,&grnn_queued_ops);
    }


    // intermediate operation, execute synchronously
    if (kAxonResultSuccess>(result=AxonApiExecuteOps(grnn_state_info.axon_handle, op_cnt, grnn_final_op_handles+ndx, kAxonAsyncModeSynchronous))) {
      break; // error!
    }

    // print the results of each operation so they can be compared to ground truth.
    switch (ndx) {
    case kGrnnAxonOpFinalWeightsMatrixMult:
      print_int16_vector(grnn_state_info.axon_handle, "h(t) x w_final", buff_final_outputs, GRNN_CLASS_COUNT, 1); break;
    case kGrnnAxonOpFinalBiasesXpy:
      print_int16_vector(grnn_state_info.axon_handle, "grnn_outputs+b_final", buff_final_outputs, GRNN_CLASS_COUNT, 1); break;
    default:
      //AxonHostLog(grnn_state_info.axon_handle, "unexpected grnn op!");
      break;
    }
  }
#else
  // do them all at once
  grnn_queued_ops.op_handle_list = grnn_final_op_handles;
  grnn_queued_ops.callback_function = grnn_result_ops_done_callback;
  grnn_queued_ops.callback_context = NULL;
  grnn_queued_ops.op_handle_count = kGrnnAxonOpFinalCount;
  return AxonApiQueueOpsList(grnn_state_info.axon_handle,&grnn_queued_ops);

#endif
}

/*
 * called when slice ops are complete
 */
static void  grnn_slice_ops_done_callback(AxonResultEnum result, void *callback_context) {
#if DEBUG_VECTORS_GRNN_UNIT > 0
  if (kAxonResultSuccess==grnn_state_info.result) {
    print_int16_vector(grnn_state_info.axon_handle, "h(t)", buff_h, GRNN_HIDDEN_HT, 1);
  }
#endif

  if (++grnn_state_info.slice_ndx<grnn_state_info.slice_count) {
    // more slices to process
    grnn_process_frame();
  } else {
    // do the final calculation
    grnn_calculate_results();
  }
}

uint8_t AxonKwsModelGrnnGetClassification(int32_t *score, char **label) {
  int32_t margin;
  uint8_t classification_ndx = max_in_array(buff_final_outputs, GRNN_CLASS_COUNT, &margin);
  if (NULL != label) {
    *label = AxonKwsGetClassificationLabel(classification_ndx);
  }
  if (NULL != score) {
    *score = buff_final_outputs[classification_ndx];
  }

  return classification_ndx;
}

/*
 * Generic KWS model function to get input feature information.
 */
AxonResultEnum AxonKwsModelGrnnGetInputAttributes(
    uint8_t *bgfg_window_slice_cnt,            /**< valid window width for background/foreground detection */
    AxonAudioFeatureVariantsEnum *which_variant, /**< specify which variant to produce */
    int32_t **normalization_means_q11p12,      /**< normalization subtracts means (as q11.12)... */
    int32_t **normalization_inv_std_devs,      /**< ...then multiplies by the inverse std deviation... */
    uint8_t *normalization_inv_std_devs_q_factor,  /**< ...then right shifts by the inverse std deviation q-factor */
    int32_t *quantization_inv_scale_factor,    /**< quantization multiplies by the inverse scaling factor... */
    uint8_t *quantization_inv_scale_factor_q_factor, /** ...then right shifts by the inverse scaling factor q-factor... */
    int8_t *quantization_zero_point,          /**< ...then adds the zero point */
    AxonDataWidthEnum *output_saturation_packing_width ) { /**< final output will be saturated/packed to this width (24 does nothing) */

  if (NULL != bgfg_window_slice_cnt) {            /**< valid window width for background/foreground detection */
    *bgfg_window_slice_cnt = AXON_AUDIO_FEATURES_SLICE_CNT;
  }
  if (NULL != which_variant) { /**< specify which variant to produce */
    *which_variant = kAxonAudioFeatureMel32;
  }
  if (NULL !=  normalization_means_q11p12) {      /**< normalization subtracts means (as q11.12)... */
    *normalization_means_q11p12 = GRNN_INPUT_NORMALIZATION_MEANS_11Q12;
  }
  if (NULL != normalization_inv_std_devs) {      /**< ...then multiplies by the inverse std deviation... */
    *normalization_inv_std_devs = GRNN_INPUT_NORMALIZATION_INV_STDS_0Q7;
  }

  if (NULL != normalization_inv_std_devs_q_factor) {  /**< ...then right shifts by the inverse std deviation q-factor */
    *normalization_inv_std_devs_q_factor = GRNN_INPUT_NORMALIZATION_INV_STDS_ROUNDING;
  }
  if (NULL != quantization_inv_scale_factor) {    /**< quantization multiplies by the inverse scaling factor... */
    *quantization_inv_scale_factor = 1;
  }
  if (NULL != quantization_inv_scale_factor_q_factor) { /** ...then right shifts by the inverse scaling factor q-factor... */
    *quantization_inv_scale_factor_q_factor = 0;
  }
  if (NULL != quantization_zero_point) {          /**< ...then adds the zero point */
    *quantization_zero_point = 0;
  }
  if (NULL != output_saturation_packing_width) { /**< final output will be saturated/packed to this width (24 does nothing) */
    *output_saturation_packing_width = kAxonDataWidth16;
  }
  return kAxonResultSuccess;

}
