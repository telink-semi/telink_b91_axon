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
#include "axon_kws_model_lstm_1fc_api.h"  
#include "axon_kws_model_lstm_1fc_const.h"
#include "axon_logging_api.h"

//NOTE : LSTM based models always have a final FC layer, the precompiler can define a final FC output length which can then be compared here!!!
/*
* make sure there is agreement between the internal model dimensions and the API stated model dimensions.
*/
static_assert(LSTM_1FC_L1_INPUT_LENGTH==LSTM_1FC_INPUT_LENGTH, "LSTM_1FC INPUT LENGTH MISMATCH!!!");
static_assert(LSTM_1FC_L1_FC_L1_OUTPUT_LENGTH==LSTM_1FC_OUTPUT_LENGTH, "LSTM_1FC OUTPUT LENGTH MISMATCH!!!");

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
 * If DEBUG_STOP_LAYER > 0, specify DEBUG_STOP_STEP to specify
 * the stopping point within that layer.
 *
 */
#define DEBUG_STOP_LAYER -1
#define DEBUG_STOP_STEP kLstmDontStop

//For each LSTM cell there will be the following ops

//NOTE :  the max number of the ops needed for each of the FC is 10, so that is scalable
#define MAX_AXON_OPS_NEEDED_PER_FC_LAYER  10
#define LSTM_1FC_L1_HIDDEN_LAYER_LENGTH          ((LSTM_1FC_L1_OUTPUT_LENGTH>>2)) //400/4

#define PER_LSTM_CELL_AXON_OP_CNT    12 //determined inside the driver code.
#define PER_FC_AXON_OP_CNT           MAX_AXON_OPS_NEEDED_PER_FC_LAYER //since only one FC layer right now, this will change with the number of FC layers needed

#define NO_OF_LSTM_CELLS           (1)
#define NO_OF_FC_LAYERS            (1)

#define MAX_LSTM_CELL_OPS          (PER_LSTM_CELL_AXON_OP_CNT*NO_OF_LSTM_CELLS)
#define MAX_FC_LAYER_OPS           (PER_FC_AXON_OP_CNT*NO_OF_FC_LAYERS)

#define TOTAL_OP_HANDLES       (MAX_LSTM_CELL_OPS+MAX_FC_LAYER_OPS)

/*
 * internal struct for the LSTM_1FC LSTM based model
 */
static struct {
  void *axon_handle;
  void (*result_callback_function)(AxonResultEnum result);
  AxonOpHandle axon_op_handles[TOTAL_OP_HANDLES];
  uint8_t axon_op_handle_count;
  uint8_t lstm_cell_op_handle_count;
  uint8_t fc_layers_op_handle_count;
  uint8_t slice_count;   // total number of slices to process
  uint8_t slice_ndx;     // current slice being processed.
  //int32_t max_input_feature; /*Not needed as we are now normalizing using fixed values*/
  //int32_t min_input_feature; /*Not needed as we are now normalizing using fixed values*/
} lstm_1fc_retained_info;

/*
 * queued ops struct doesn't need to be
 * in retained memory.
 */
static AxonMgrQueuedOpsStruct lstm_1fc_queued_ops;

/*
 * RAM buffers
 */
/*
*  io buffer will have int8 operations, needs to be 16byte aligned
*  io buffer needs to be the larger of LSTM_1FC_INPUT_LENGTH and the LSTM_1FC_L1_OUTPUT_LENGTH
*/
#define LSTM_1FC_IO_BUFFER_SIZE (LSTM_1FC_L1_INPUT_LENGTH/4 > LSTM_1FC_L1_OUTPUT_LENGTH ? LSTM_1FC_L1_INPUT_LENGTH/4 :  LSTM_1FC_L1_OUTPUT_LENGTH)
_Alignas(16) int32_t lstm_1fc_io_buffer[LSTM_1FC_IO_BUFFER_SIZE];
// buf1 & 2 are 24 bit operations only, don't need special alignment
int32_t lstm_1fc_buff1[LSTM_1FC_L1_OUTPUT_LENGTH];
int32_t lstm_1fc_buff2[LSTM_1FC_L1_OUTPUT_LENGTH];

/*
 * The cell state buffer is required for storing the c_t which is used in the consecutive slices
 */
int32_t ct_buff[LSTM_1FC_L1_HIDDEN_LAYER_LENGTH]; //FIXME this should be the maximum of all the hidden layers required in the LSTM model

/*
 * arrays sized to the audio features height to
 * normalize the inputs for the next slice
 */
int32_t input_buffer[AUDIO_INPUT_FEATURE_HEIGHT] = {0};

static int axon_kws_model_lstm_1fc_prepare(void * axon_handle,
    AxonOpHandle axon_op_handles[],
    uint8_t *op_handle_count,//redundant??
    uint8_t *lstm_cell_op_count,
    uint8_t *fc_layers_op_count,
    int32_t *io_buffer,
    uint16_t io_buffer_length,
    int32_t *buff1,
    int32_t *buff2,
    uint16_t buff1_length,
    uint16_t buff2_length,
    int32_t *ct_buff,
    uint16_t ct_buff_length){


  AxonResultEnum result;
  uint8_t tmp_op_handle_cnt;
  uint8_t total_ops_needed = 0;
  tmp_op_handle_cnt=*op_handle_count-total_ops_needed;

  // define LSTM cell Layer1

  result = AxonApiDefineOpListLstmCellWithStopStep(
      axon_handle,
      LSTM_1FC_L1_INPUT_LENGTH,
      LSTM_1FC_L1_OUTPUT_LENGTH,
      LSTM_1FC_L1_INPUT_BITWIDTH,
      io_buffer,
      io_buffer_length,
      lstm_1fc_l1_weights,
      lstm_1fc_l1_bias_prime,
      LSTM_1FC_L1_BIAS_ADD_MULTIPLIER,
      LSTM_1FC_L1_BIAS_ADD_ROUNDING,
      LSTM_1FC_L1_ACTIVATION_FUNCTION,
      LSTM_1FC_L1_RECURRENT_ACTIVATION_FUNCTION,
      LSTM_1FC_L1_MULTIPLIER_ROUNDING,//multiplier rounding
      //0,//addition rounding
      LSTM_1FC_L1_HIDDEN_MULTIPLIER_ROUNDING,//hidden multiplier rounding
      LSTM_1FC_L1_HIDDEN_LAYER_LENGTH,
      LSTM_1FC_L1_HIDDENSTATE_QUANTIZE_INV_SCALING_FACTOR,
      LSTM_1FC_L1_HIDDENSTATE_QUANTIZE_ZERO_POINT,
      LSTM_1FC_L1_HIDDENSTATE_QUANTIZE_INV_SCALING_FACTOR_SHIFT,
      buff1,
      ct_buff,
      buff1_length,
      ct_buff_length,
      axon_op_handles+total_ops_needed,
      &tmp_op_handle_cnt, /**< Provided by the user as the length of axon_op_handles, returned with the number of handles actually used.*/
      DEBUG_STOP_STEP);

  if (kAxonResultSuccess != result) {
    axon_printf(axon_handle, "Define LSTM_1FC_L1 failed! %d\r\n" , result);
    AxonApiFreeOpHandles(axon_handle, total_ops_needed, axon_op_handles);
    return result;
  }

  total_ops_needed += tmp_op_handle_cnt;

  //LSTM cell defn is over, get the count of ops used in the LSTM cell
  *lstm_cell_op_count = total_ops_needed;

  //get the remaining op_handle count
  tmp_op_handle_cnt=*op_handle_count-total_ops_needed; //tmp_op_handles now has the remaining op_handles, which can be passed to the multiple FC layers.

  //START DEFINITIONS OF THE FC LAYERS
  
 if ((DEBUG_STOP_LAYER<0) || (DEBUG_STOP_LAYER >= 1)) {
    // define FC layer1
    result = AxonApiDefineOpListFullyConnectedWithStopStep(axon_handle,
        LSTM_1FC_L1_FC_L1_INPUT_LENGTH,
        LSTM_1FC_L1_FC_L1_OUTPUT_LENGTH,
        LSTM_1FC_L1_FC_L1_INPUT_BITWIDTH,     /**< nominally 8bit but unsaturated, and unpacked */
        io_buffer,
        io_buffer_length,     /**< length of buf1. Must be >= max(input_length, output_length) */
        lstm_1fc_l1_fc_l1_weights,
        lstm_1fc_l1_fc_l1_bias_prime,
        LSTM_1FC_L1_FC_L1_BIAS_ADD_MULTIPLIER,
        LSTM_1FC_L1_FC_L1_BIAS_ADD_ROUNDING,
        LSTM_1FC_L1_FC_L1_ACTIVATION_FUNCTION,
        NULL,//LSTM_1FC_L1_FC_L1_normalization_mult,
        NULL,//LSTM_1FC_L1_FC_L1_NORM_MULT_ROUNDING,
        NULL,//lstm_1fc_l1_fc_l1_normalization_add,
        NULL,//LSTM_1FC_L1_FC_L1_NORM_ADD_ROUNDING,
        LSTM_1FC_L1_FC_L1_QUANTIZE_MULTIPLIER,
        LSTM_1FC_L1_FC_L1_QUANTIZE_ADD,
        LSTM_1FC_L1_FC_L1_QUANTIZE_ROUNDING,
        LSTM_1FC_L1_FC_L1_QUANTIZE_STANDALONE_ADD, /**< used only when non-0. Only necessary when quantize_add cannot be provided in sufficient precision */
        buff1,
        buff2,
        buff1_length,            /**< length of buff1. Must be >= output_length */
        buff2_length,            /**< length of buff2. Must be >= output_length */
        axon_op_handles+total_ops_needed,
        &tmp_op_handle_cnt,
        kDontStop);

    if (kAxonResultSuccess != result) {
      axon_printf(axon_handle, "Define LSTM_1FC_L1_FC_L1 failed! %d\r\n" , result);
      AxonApiFreeOpHandles(axon_handle, total_ops_needed, axon_op_handles);
      return result;
    }

    total_ops_needed += tmp_op_handle_cnt;
    tmp_op_handle_cnt=*op_handle_count-total_ops_needed;
  }


  *fc_layers_op_count = total_ops_needed - (*lstm_cell_op_count);
  //tmp now has the remaining op handles in it
  //if it is the last op_handle then we should update the op_handle_count accordingly
  *op_handle_count = total_ops_needed;//DO WE NEED THIS NOW?????
  return kAxonResultSuccess;
}

static const char *lstm_1fc_labels[] = {
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


static_assert(sizeof(lstm_1fc_labels)/sizeof(lstm_1fc_labels[0])==LSTM_1FC_OUTPUT_LENGTH, "LSTM_1FC_LABELS[] MIS-SIZED!");


/*
* API level prepare; use our internal buffers.
*/
AxonResultEnum AxonKwsModelLstm1fcPrepare(void *axon_handle, void (*result_callback_function)(AxonResultEnum result)){
  // define the ops
  lstm_1fc_retained_info.axon_handle = axon_handle;
  lstm_1fc_retained_info.result_callback_function = result_callback_function;

  lstm_1fc_retained_info.axon_op_handle_count = TOTAL_OP_HANDLES;
  return  axon_kws_model_lstm_1fc_prepare(
      lstm_1fc_retained_info.axon_handle,
      lstm_1fc_retained_info.axon_op_handles,
      &lstm_1fc_retained_info.axon_op_handle_count,
      &lstm_1fc_retained_info.lstm_cell_op_handle_count,
      &lstm_1fc_retained_info.fc_layers_op_handle_count,
      lstm_1fc_io_buffer,
      LSTM_1FC_IO_BUFFER_SIZE, //TODO write a static assert for this
      lstm_1fc_buff1,
      lstm_1fc_buff2,
      LSTM_1FC_L1_OUTPUT_LENGTH,
      LSTM_1FC_L1_OUTPUT_LENGTH,
      ct_buff,
      LSTM_1FC_L1_HIDDEN_LAYER_LENGTH);
}

/*
 * get the input slice and normalize it
 */
static void lstm_1fc_get_input_slice_and_normalize(){
  //get the slice data
  AudioInputFeatureType *slice_ptr;
  AxonKwsHostGetNextAudioFeatureSlice(&slice_ptr);
  
  //and then normalize the input
  /*
   * NORMALIZING BETWEEN [-1,1](-128,127), which is also quantizing the inputs into 8 bits.
   */
  for(uint8_t ndx=0;ndx<AUDIO_INPUT_FEATURE_HEIGHT;ndx++){
    int32_t norm = (slice_ptr[ndx]-(LSTM_1FC_L1_INPUT_NORM_MIN_VALUE))>= (LSTM_1FC_L1_INPUT_NORM_MAX_VALUE) ? (LSTM_1FC_L1_INPUT_NORM_MAX_VALUE) : (slice_ptr[ndx]-(LSTM_1FC_L1_INPUT_NORM_MIN_VALUE));
    input_buffer[ndx] = ((-128)+(((LSTM_1FC_L1_INPUT_QUANTIZE_INV_SCALING_FACTOR)*norm))/(LSTM_1FC_L1_INPUT_NORM_MAX_VALUE));
  }
}

/*
* callback function invoked when the classification has completed
*/
static void lstm_1fc_classify_complete_callback(AxonResultEnum result, void *callback_context) {

  // invoke the caller's callback.
  lstm_1fc_retained_info.result_callback_function(result);
}

/*
 * start final classification after all slices have been processed.
 */
static AxonResultEnum lstm_1fc_calculate_results() {
  
  //copying the hidden vectors to the start of the io_buffer, so that the FC can use it as an input
  memcpy(lstm_1fc_io_buffer,&lstm_1fc_io_buffer[AUDIO_INPUT_FEATURE_HEIGHT],sizeof(AudioInputFeatureType)*LSTM_1FC_L1_FC_L1_INPUT_LENGTH);

  lstm_1fc_queued_ops.op_handle_list = lstm_1fc_retained_info.axon_op_handles+lstm_1fc_retained_info.lstm_cell_op_handle_count;
  lstm_1fc_queued_ops.callback_function = lstm_1fc_classify_complete_callback;
  lstm_1fc_queued_ops.callback_context = NULL;
  lstm_1fc_queued_ops.op_handle_count = lstm_1fc_retained_info.fc_layers_op_handle_count;
  return AxonApiQueueOpsList(lstm_1fc_retained_info.axon_handle,&lstm_1fc_queued_ops);
}

static void lstm_1fc_slice_ops_done_callback(AxonResultEnum result, void *callback_context) {
  if (++lstm_1fc_retained_info.slice_ndx<lstm_1fc_retained_info.slice_count) {
    // more slices to process
    lstm_1fc_process_frame();
  } else {
    // do the final calculation
    lstm_1fc_calculate_results();
  }
}

/*
 * API function to calculate the hidden vector for each audio frame.
 */
AxonResultEnum lstm_1fc_process_frame() {

  AxonResultEnum result;
  
  //copy the normalized input vector into the io_buffer
  memcpy(lstm_1fc_io_buffer,input_buffer,sizeof(AudioInputFeatureType)*AUDIO_INPUT_FEATURE_HEIGHT);

  lstm_1fc_queued_ops.op_handle_list = lstm_1fc_retained_info.axon_op_handles;
  lstm_1fc_queued_ops.callback_function = lstm_1fc_slice_ops_done_callback;
  lstm_1fc_queued_ops.callback_context = NULL;
  lstm_1fc_queued_ops.op_handle_count = lstm_1fc_retained_info.lstm_cell_op_handle_count;
  result = AxonApiQueueOpsList(lstm_1fc_retained_info.axon_handle, &lstm_1fc_queued_ops);
  if (result<kAxonResultSuccess) { //error
    return result;
  }
  
  if(lstm_1fc_retained_info.slice_ndx<(lstm_1fc_retained_info.slice_count-1)){
    //as we are calculating the normalized inputs for the next frame before the function is called,
    //we do not have to calculate it for the very last one as we have already done it
    //so we call this for N-1 times
    lstm_1fc_get_input_slice_and_normalize();
  }

  return kAxonResultSuccess;
}

/*
 * LSTM_1FC classification is done slice after slice
 */
AxonResultEnum AxonKwsModelLstm1fcInfer(uint8_t slice_count){
  lstm_1fc_retained_info.slice_count = slice_count;
  lstm_1fc_retained_info.slice_ndx = 0;


/*
 * USING FIXED MAX AND MIN VALUES
   //get the max and min of the current window which is calculated right before the classification starts
  AxonKwsHostGetMaxMinAudioFeatures(&lstm_1fc_retained_info.max_input_feature,&lstm_1fc_retained_info.min_input_feature);
*/

  //clearing out the input_buffer and the cell state buffer before starting a new inference!
  memset(lstm_1fc_io_buffer, 0, sizeof(AudioInputFeatureType)*LSTM_1FC_IO_BUFFER_SIZE);
  memset(ct_buff, 0, sizeof(AudioInputFeatureType)*LSTM_1FC_L1_HIDDEN_LAYER_LENGTH);

  //get the normalized input for the first time,
  //the same function is called right after the axon operations are queued
  lstm_1fc_get_input_slice_and_normalize();
  return lstm_1fc_process_frame();
}

/*
 * Generic KWS model function to get input feature information.
 */
AxonResultEnum AxonKwsModelLstm1fcGetInputAttributes(
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
    *bgfg_window_slice_cnt = LSTM_1FC_L1_INPUT_WIDTH;
  }
  if (NULL != which_variant) { /**< specify which audio feature variant is required*/
    *which_variant = LSTM_1FC_AUDIO_FEATURES_TYPE;
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
    *quantization_inv_scale_factor = 0;
  }
  if (NULL != quantization_inv_scale_factor_q_factor) { /** ...then right shifts by the inverse scaling factor q-factor... */
    *quantization_inv_scale_factor_q_factor = 0;
  }
  if (NULL != quantization_zero_point) {          /**< ...then adds the zero point */
    *quantization_zero_point = 0;
  }
  if (NULL != output_saturation_packing_width) { /**< final output will be saturated/packed to this width (24 does nothing) */
    *output_saturation_packing_width = AXON_AUDIO_FEATURES_DATA_WIDTH;
  }
  return kAxonResultSuccess;

}

/*
 * find the maxvalue in io_buffer and return its index
 */
static uint8_t axon_model_lstm_1fc_get_classification(const int32_t *io_buffer, const char **label) {
  int32_t max_value = *io_buffer;
  uint8_t max_value_ndx = 0;
  for (uint8_t ndx=1;ndx<LSTM_1FC_L1_FC_L1_OUTPUT_LENGTH;ndx++) {
    if (io_buffer[ndx]>max_value) {
      max_value = io_buffer[ndx];
      max_value_ndx = ndx;
    }
  }
  if (label) {
    *label = lstm_1fc_labels[max_value_ndx];
  }
  return max_value_ndx;
}

uint8_t AxonKwsModelLstm1fcGetClassification(int32_t *score, char **label) {
  uint8_t classification_ndx = axon_model_lstm_1fc_get_classification(lstm_1fc_io_buffer, label);
  if (NULL != score) {
    *score = lstm_1fc_io_buffer[classification_ndx];
  }

  return classification_ndx;
}

