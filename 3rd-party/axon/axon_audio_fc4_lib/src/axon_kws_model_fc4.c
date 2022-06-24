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
#include "axon_kws_model_fc4_api.h"
#include "axon_kws_model_fc4_const.h"
#include "axon_logging_api.h"

/*
* make sure there is agreement between the internal model dimensions and the API stated model dimensions.
*/
static_assert(FC4_L1_INPUT_LENGTH==FC4_INPUT_LENGTH, "FC4 INPUT LENGTH MISMATCH!!!");
static_assert(FC4_L4_OUTPUT_LENGTH==FC4_OUTPUT_LENGTH, "FC4 OUTPUT LENGTH MISMATCH!!!");

/*
 * Layer testing
 * The model can be configured to do a partial inference, stopping at any layer, and out various
 * points within that layer.
 * The output printed will then be the stopping point of inference. This provides a way to debug bit-exact
 * issues with the model.
 *
 * To enable specify a DEBUG_STOP_LAYER number
 *   -1 disables partial inference,
 *   0 stops after input quantization,
 *   everything else stops at that layer.
 *
 * If DEBUG_STTOP_LAYER > 0, specify DEBUG_STOP_STEP to specify
 * the stopping point within that layer.
 *
 * You must also specify the vector output length for printing and a label in
 * FC4_DEBUG_OUTPUT_LENGTH and FC4_DEBUG_FINAL_STEP_NAME, respectively.
 *
 */
#define DEBUG_STOP_LAYER -1
#define DEBUG_STOP_STEP kDontStop

#if DEBUG_STOP_LAYER==0
// input quantization is 16bit
# define FC4_DEBUG_PRINT_OUTPUT print_int16_vector
# define FC4_DEBUG_OUTPUT_LENGTH FC4_L1_INPUT_LENGTH
# define FC4_DEBUG_FINAL_STEP_NAME FC4_INPUT_QUANT
#elif DEBUG_STOP_LAYER>0
// everything else is 32 bit.
# define FC4_DEBUG_PRINT_OUTPUT print_int32_vector
// it's your job to provide the length of the output. The length is simply the layer's output length.
// # define FC4_DEBUG_OUTPUT_LENGTH FC4_L<DEBUG_STOP_LAYER>_OUTPUT_LENGTH
# define FC4_DEBUG_OUTPUT_LENGTH FC4_L1_OUTPUT_LENGTH
// it's your job to provide give an appropriate label.
// # define FC4_DEBUG_FINAL_STEP_NAME "FC4_L<DEBUG_STOP_LAYER>_<DEBUG_STOP_STEP>"
# define FC4_DEBUG_FINAL_STEP_NAME "FC4_L1_OUTPUT"
#endif

RETAINED_MEMORY_SECTION_ATTRIBUTE
static struct {
# define FC4_AXON_OP_HANDLE_COUNT 40 // 10 per layer.
  AxonOpHandle fc4_axon_op_handles[FC4_AXON_OP_HANDLE_COUNT];
  void *axon_handle;
  void (*result_callback_function)(AxonResultEnum result);
  int32_t *io_buffer;
  uint8_t fc4_op_handle_count;
} fc4_retained_info;



static int axon_kws_model_fc4_prepare(void *axon_handle,
    AxonOpHandle axon_op_handles[],
    uint8_t *op_handle_count,
    int32_t *io_buffer,
    uint16_t io_buffer_length,
    int32_t *buf1,
    int32_t *buf2,
    uint16_t buf1_length,
    uint16_t buf2_length) {

  // define each of the layers, appending the operations to each other.
  if (NULL==op_handle_count) {
    return kAxonResultFailureNullBuffer;
  }
  AxonResultEnum result;
  uint8_t tmp_op_handle_cnt;
  uint8_t total_ops_needed = 0;
  tmp_op_handle_cnt=*op_handle_count-total_ops_needed;


  if ((DEBUG_STOP_LAYER<0) || (DEBUG_STOP_LAYER >= 1)) {

    // define layer1
    result = AxonApiDefineOpListFullyConnectedWithStopStep(axon_handle,
        FC4_L1_INPUT_LENGTH,
        FC4_L1_OUTPUT_LENGTH,
        FC4_L1_INPUT_BITWIDTH,     /**< nominally 8bit but unsaturated, and unpacked */
        io_buffer,
        io_buffer_length,     /**< length of buf1. Must be >= max(input_length, output_length) */
        fc4_l1_weights,
        fc4_l1_bias_prime,
        FC4_L1_BIAS_ADD_MULTIPLIER,
        FC4_L1_BIAS_ADD_ROUNDING,
        FC4_L1_ACTIVATION_FUNCTION,
        fc4_l1_normalization_mult,
        FC4_L1_NORM_MULT_ROUNDING,
        fc4_l1_normalization_add,
        FC4_L1_NORM_ADD_ROUNDING,
        FC4_L1_QUANTIZE_MULTIPLIER,
        FC4_L1_QUANTIZE_ADD,
        FC4_L1_QUANTIZE_ROUNDING,
        FC4_L1_QUANTIZE_STANDALONE_ADD, /**< used only when non-0. Only necessary when quantize_add cannot be provided in sufficient precision */
        buf1,
        buf2,
        buf1_length,            /**< length of buf1. Must be >= output_length */
        buf2_length,            /**< length of buf2. Must be >= output_length */
        axon_op_handles+total_ops_needed,
        &tmp_op_handle_cnt,
        DEBUG_STOP_LAYER==1 ? DEBUG_STOP_STEP : kDontStop);

    if (kAxonResultSuccess != result) {
      axon_printf(axon_handle, "Define FC4_L1 failed! %d\r\n" , result);
      AxonApiFreeOpHandles(axon_handle, total_ops_needed, axon_op_handles);
      return result;
    }

    total_ops_needed += tmp_op_handle_cnt;
    tmp_op_handle_cnt=*op_handle_count-total_ops_needed;
  }

  if ((DEBUG_STOP_LAYER<0) || (DEBUG_STOP_LAYER >= 2)) {

    // define layer2
    result = AxonApiDefineOpListFullyConnectedWithStopStep(axon_handle,
        FC4_L2_INPUT_LENGTH,
        FC4_L2_OUTPUT_LENGTH,
        FC4_L2_INPUT_BITWIDTH,     /**< nominally 8bit but unsaturated, and unpacked */
        io_buffer,
        io_buffer_length,     /**< length of buf1. Must be >= max(input_length, output_length) */
        fc4_l2_weights,
        fc4_l2_bias_prime,
        FC4_L2_BIAS_ADD_MULTIPLIER,
        FC4_L2_BIAS_ADD_ROUNDING,
        FC4_L2_ACTIVATION_FUNCTION,
        fc4_l2_normalization_mult,
        FC4_L2_NORM_MULT_ROUNDING,
        fc4_l2_normalization_add,
        FC4_L2_NORM_ADD_ROUNDING,
        FC4_L2_QUANTIZE_MULTIPLIER,
        FC4_L2_QUANTIZE_ADD,
        FC4_L2_QUANTIZE_ROUNDING,
        FC4_L2_QUANTIZE_STANDALONE_ADD, /**< used only when non-0. Only necessary when quantize_add cannot be provided in sufficient precision */
        buf1,
        buf2,
        buf1_length,            /**< length of buf1. Must be >= output_length */
        buf2_length,            /**< length of buf2. Must be >= output_length */
        axon_op_handles+total_ops_needed,
        &tmp_op_handle_cnt,
        DEBUG_STOP_LAYER==2 ? DEBUG_STOP_STEP : kDontStop);

    if (kAxonResultSuccess != result) {
      axon_printf(axon_handle, "Define FC4_L2 failed! %d\r\n" , result);
      AxonApiFreeOpHandles(axon_handle, total_ops_needed, axon_op_handles);
      return result;
    }

    total_ops_needed += tmp_op_handle_cnt;
    tmp_op_handle_cnt=*op_handle_count-total_ops_needed;
  }

  if ((DEBUG_STOP_LAYER<0) || (DEBUG_STOP_LAYER >= 3)) {

    // define layer3
    result = AxonApiDefineOpListFullyConnectedWithStopStep(axon_handle,
        FC4_L3_INPUT_LENGTH,
        FC4_L3_OUTPUT_LENGTH,
        FC4_L3_INPUT_BITWIDTH,     /**< nominally 8bit but unsaturated, and unpacked */
        io_buffer,
        io_buffer_length,     /**< length of buf1. Must be >= max(input_length, output_length) */
        fc4_l3_weights,
        fc4_l3_bias_prime,
        FC4_L3_BIAS_ADD_MULTIPLIER,
        FC4_L3_BIAS_ADD_ROUNDING,
        FC4_L3_ACTIVATION_FUNCTION,
        fc4_l3_normalization_mult,
        FC4_L3_NORM_MULT_ROUNDING,
        fc4_l3_normalization_add,
        FC4_L3_NORM_ADD_ROUNDING,
        FC4_L3_QUANTIZE_MULTIPLIER,
        FC4_L3_QUANTIZE_ADD,
        FC4_L3_QUANTIZE_ROUNDING,
        FC4_L3_QUANTIZE_STANDALONE_ADD, /**< used only when non-0. Only necessary when quantize_add cannot be provided in sufficient precision */
        buf1,
        buf2,
        buf1_length,            /**< length of buf1. Must be >= output_length */
        buf2_length,            /**< length of buf2. Must be >= output_length */
        axon_op_handles+total_ops_needed,
        &tmp_op_handle_cnt,
        DEBUG_STOP_LAYER==3 ? DEBUG_STOP_STEP : kDontStop);

    if (kAxonResultSuccess != result) {
      axon_printf(axon_handle, "Define FC4_L3 failed! %d\r\n" , result);
      AxonApiFreeOpHandles(axon_handle, total_ops_needed, axon_op_handles);
      return result;
    }

    total_ops_needed += tmp_op_handle_cnt;
    tmp_op_handle_cnt=*op_handle_count-total_ops_needed;
  }

  if ((DEBUG_STOP_LAYER<0) || (DEBUG_STOP_LAYER >= 4)) {

    // define layer4
    result = AxonApiDefineOpListFullyConnectedWithStopStep(axon_handle,
        FC4_L4_INPUT_LENGTH,
        FC4_L4_OUTPUT_LENGTH,
        FC4_L4_INPUT_BITWIDTH,     /**< nominally 8bit but unsaturated, and unpacked */
        io_buffer,
        io_buffer_length,     /**< length of buf1. Must be >= max(input_length, output_length) */
        fc4_l4_weights,
        fc4_l4_bias_prime,
        FC4_L4_BIAS_ADD_MULTIPLIER,
        FC4_L4_BIAS_ADD_ROUNDING,
        FC4_L4_ACTIVATION_FUNCTION,
        fc4_l4_normalization_mult,
        FC4_L4_NORM_MULT_ROUNDING,
        fc4_l4_normalization_add,
        FC4_L4_NORM_ADD_ROUNDING,
        FC4_L4_QUANTIZE_MULTIPLIER,
        FC4_L4_QUANTIZE_ADD,
        FC4_L4_QUANTIZE_ROUNDING,
        FC4_L4_QUANTIZE_STANDALONE_ADD, /**< used only when non-0. Only necessary when quantize_add cannot be provided in sufficient precision */
        buf1,
        buf2,
        buf1_length,            /**< length of buf1. Must be >= output_length */
        buf2_length,            /**< length of buf2. Must be >= output_length */
        axon_op_handles+total_ops_needed,
        &tmp_op_handle_cnt,
        DEBUG_STOP_LAYER==4 ? DEBUG_STOP_STEP : kDontStop);

    if (kAxonResultSuccess != result) {
      axon_printf(axon_handle, "Define FC4_L4 failed! %d\r\n" , result);
      AxonApiFreeOpHandles(axon_handle, total_ops_needed, axon_op_handles);
      return result;
    }

    total_ops_needed += tmp_op_handle_cnt;
    tmp_op_handle_cnt=*op_handle_count-total_ops_needed;
  }


  // last 1, so return the total used.
  *op_handle_count = total_ops_needed;
  return kAxonResultSuccess;
}

static const char *fc4_labels[] = {
    "SILENCE", // 0
    "UNKNOWN", // 1
    "YES", // 2
    "NO", // 3
    "UP", // 4
    "DOWN", // 5
    "LEFT", // 6
    "RIGHT", // 7
    "ON", // 8
    "OFF", // 9
    "STOP", // 10
    "GO", // 11
};



static_assert(sizeof(fc4_labels)/sizeof(fc4_labels[0])==FC4_OUTPUT_LENGTH, "FC4_LABELS[] MIS-SIZED!");


/*
* RAM buffers for FC4 inference
*/
/*
*  io buffer will have int8 operations, needs to be 16byte aligned
*  io buffer needs to be the larger of FC4_INPUT_LENGTH and (FC4_MIDLAYER_LENGTH * 4)
*/
#define FC4_IO_BUFFER_SIZE (FC4_L1_INPUT_LENGTH/4 > FC4_L1_OUTPUT_LENGTH ? FC4_L1_INPUT_LENGTH/4 :  FC4_L1_OUTPUT_LENGTH)
_Alignas(16) int32_t fc4_io_buffer[FC4_IO_BUFFER_SIZE];
// buf1 & 2 are 24 bit operations only, don't need special alignment
int32_t fc4_buff1[FC4_L1_OUTPUT_LENGTH];
int32_t fc4_buff2[FC4_L1_OUTPUT_LENGTH];

/*
* API level prepare; use our internal buffers.
*/
AxonResultEnum AxonKwsModelFc4Prepare(void *axon_handle, void (*result_callback_function)(AxonResultEnum result)){
  // define the ops
  fc4_retained_info.axon_handle = axon_handle;
  fc4_retained_info.result_callback_function = result_callback_function;

  fc4_retained_info.fc4_op_handle_count = FC4_AXON_OP_HANDLE_COUNT; // initialize to actual size,
  return  axon_kws_model_fc4_prepare(
        axon_handle,
        fc4_retained_info.fc4_axon_op_handles,
        &fc4_retained_info.fc4_op_handle_count,
        fc4_io_buffer,
        FC4_IO_BUFFER_SIZE,
        fc4_buff1,
        fc4_buff2,
        FC4_L1_OUTPUT_LENGTH,
        FC4_L1_OUTPUT_LENGTH);
}

/*
* callback function invoked when the fc4 classification has completed
*/
static void fc4_classify_complete_callback(AxonResultEnum result, void *callback_context) {

  // invoke the caller's callback.
  fc4_retained_info.result_callback_function(result);
}

/*
 * FC4 classification does not happen slice-by-slice. Instead we need to gather
 * all the audio features, (61x10), flatten/pack/saturate them, then invoke to FC4 model.
 */
AxonResultEnum AxonKwsModelFc4Infer(uint8_t window_width) {
  // window_width has to equal our expected input length
  if (window_width != FC4_L1_INPUT_WIDTH) {
    AxonPrintf("FC4 inference invalid window length %d, %s\r\n", window_width );
    return -100;
  }
  static AxonMgrQueuedOpsStruct fc4_axon_queued_ops;

  // pack saturate from MFCC_FEATURE_TYPE to int8_t
  int8_t *io_buff = (int8_t *)fc4_io_buffer;
  for (uint16_t slice_ndx=0;slice_ndx<FC4_L1_INPUT_WIDTH;slice_ndx++) {
    const AudioInputFeatureType *slice_ptr;
    AxonKwsHostGetNextAudioFeatureSlice(&slice_ptr);

    memcpy(io_buff, slice_ptr, sizeof(AudioInputFeatureType) * AUDIO_INPUT_FEATURE_HEIGHT);
    io_buff+= AUDIO_INPUT_FEATURE_HEIGHT;
  }

  // prepare/submit the batch operation to axon
  fc4_axon_queued_ops.op_handle_list = fc4_retained_info.fc4_axon_op_handles;
  fc4_axon_queued_ops.op_handle_count = fc4_retained_info.fc4_op_handle_count;
  fc4_axon_queued_ops.callback_context = NULL;
  fc4_axon_queued_ops.callback_function = fc4_classify_complete_callback;
  // and submit!
  AxonApiQueueOpsList(fc4_retained_info.axon_handle, &fc4_axon_queued_ops);
}

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
    AxonDataWidthEnum *output_saturation_packing_width ) { /**< final output will be saturated/packed to this width (24 does nothing) */

  if (NULL != bgfg_window_slice_cnt) {            /**< valid window width for background/foreground detection */
    *bgfg_window_slice_cnt = FC4_L1_INPUT_WIDTH;
  }
  if (NULL != which_variant) { /**< specify which audio feature variant is required*/
    *which_variant = FC4_AUDIO_FEATURES_TYPE;
  }
  if (NULL !=  normalization_means_q11p12) {      /**< normalization subtracts means (as q11.12)... */
    *normalization_means_q11p12 = NULL;
  }
  if (NULL != normalization_inv_std_devs) {      /**< ...then multiplies by the inverse std deviation... */
    *normalization_inv_std_devs = NULL;
  }

  if (NULL != normalization_inv_std_devs_q_factor) {  /**< ...then right shifts by the inverse std deviation q-factor */
    *normalization_inv_std_devs_q_factor = 0;
  }
  if (NULL != quantization_inv_scale_factor) {    /**< quantization multiplies by the inverse scaling factor... */
    *quantization_inv_scale_factor = FC4_L1_INPUT_QUANTIZE_INV_SCALING_FACTOR;
  }
  if (NULL != quantization_inv_scale_factor_q_factor) { /** ...then right shifts by the inverse scaling factor q-factor... */
    *quantization_inv_scale_factor_q_factor = FC4_L1_INPUT_QUANTIZE_INV_SCALING_FACTOR_SHIFT;
  }
  if (NULL != quantization_zero_point) {          /**< ...then adds the zero point */
    *quantization_zero_point = FC4_L1_INPUT_QUANTIZE_ZERO_POINT;
  }
  if (NULL != output_saturation_packing_width) { /**< final output will be saturated/packed to this width (24 does nothing) */
    *output_saturation_packing_width = kAxonDataWidth8;
  }
  return kAxonResultSuccess;

}

/*
 * find the maxvalue in io_buffer and return its index
 */
static uint8_t axon_model_fc4_get_classification(const int32_t *io_buffer, const char **label) {
  int32_t max_value = *io_buffer;
  uint8_t max_value_ndx = 0;
  for (uint8_t ndx=1;ndx<FC4_L4_OUTPUT_LENGTH;ndx++) {
    if (io_buffer[ndx]>max_value) {
      max_value = io_buffer[ndx];
      max_value_ndx = ndx;
    }
  }
  if (label) {
    *label = fc4_labels[max_value_ndx];
  }
  return max_value_ndx;
}

uint8_t AxonKwsModelFc4GetClassification(int32_t *score, char **label) {
  uint8_t classification_ndx = axon_model_fc4_get_classification(fc4_io_buffer, label);
  if (NULL != score) {
    *score = fc4_io_buffer[classification_ndx];
  }

  return classification_ndx;
}
