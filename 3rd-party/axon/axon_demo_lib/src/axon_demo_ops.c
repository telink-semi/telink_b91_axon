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
/*
 * This file contains functions to either execute a discrete axon operation or to create
 * a descriptor for that same operation to be executed later.
 *
 * Each operation has a corresponding verify function that is executed immediately
 * after the operation (discrete mode) or can be executed after a batch operation.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "axon_api.h"
#include "axon_dep.h"
#include "axon_demo_private.h"



/*
 * helper function to print out:
 * define op <op_name>
 * or
 * execute op <op_name>
 *
 * Invoked by all the "axon_sample_op_<op_name> functions.
 */
static void print_sample_op_header(void *axon_instance, const char *op_name, bool execute_not_define) {
  if (execute_not_define) {
    axon_printf(axon_instance, "execute op %s... ", op_name);
  } else {
    axon_printf(axon_instance, "define op %s\r\n", op_name);
  }
}


/*
 * Sets up a sample FFT operation. If axon_op_handle is NULL, the operation
 * is executed discretely. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 *
 * populates fft_outputs[]
 * verified against fft_512_expected[]
 */
AxonResultEnum axon_sample_op_FFT(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  print_sample_op_header((AxonInstanceStruct*) axon_instance, "FFT", axon_op_handle==NULL);

  AxonInputStruct testInput;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.length = 512;
  testInput.x_in = fft_512_input;
  testInput.q_out = fft_outputs;
  testInput.output_rounding = 1;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  memset(fft_outputs, 0, sizeof(fft_outputs));

  if (NULL == axon_op_handle) {
    // execute discretely
    if (kAxonResultSuccess <= (result = AxonApiFft(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_FFT_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpFft(axon_instance, &testInput, axon_op_handle);
  }
  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_of_FFT()
 */
AxonResultEnum axon_sample_op_FFT_verify(void *axon_instance) {
  return verify_vectors("FFT", fft_outputs, fft_512_expected, 1024, 0) < kAxonResultSuccess;
}

/*
 * Sets up a sample FIR operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_FIR(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = FIR_DATA_LENGTH;
  testInput.y_length = FIR_FILTER_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = fir_input_x;
  testInput.y_in = fir_input_F;
  testInput.q_out = fir_outputs;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  memset(fir_outputs, 0, sizeof(fir_outputs));
  print_sample_op_header((AxonInstanceStruct*) axon_instance, "FIR", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiFir(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_FIR_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpFir(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_of_FIR()
 */
AxonResultEnum axon_sample_op_FIR_verify(void *axon_instance) {
  return verify_vectors("FIR", fir_outputs, fir_expected_outputs, FIR_DATA_LENGTH, 0);
}

/*
 * Sets up a  sample matrix mult operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 *
 * In this case, the input data width matches the output data width at 16 bits.
 */
int axon_sample_op_MATRIX_MULT_16_in_16_out(void *axon_instance, AxonOpHandle *axon_op_handle, AxonAfEnum activation_function) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = MATRIX_MULT_VECTOR_LENGTH;
  testInput.y_length = MATRIX_MULT_MATRIX_HEIGHT;
  testInput.data_width = kAxonDataWidth16;
  testInput.data_packing = kAxonDataPackingEnabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = activation_function;
  testInput.x_in = matrix_mult_input_x;
  testInput.y_in = matrix_mult_input_y;
  testInput.q_out = matrix_mult_output_q;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  memset(matrix_mult_output_q, 0, sizeof(matrix_mult_output_q));

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "matrix_mult 16in/16out", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiMatrixMult(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_MATRIX_MULT_16_in_16_out_verify(axon_instance, activation_function);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpMatrixMult(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_matrix_mult_16_in_16_out()
 */
AxonResultEnum axon_sample_op_MATRIX_MULT_16_in_16_out_verify(void *axon_instance, AxonAfEnum activation_function) {
  switch (activation_function) {
  case kAxonAfDisabled:
    return verify_vectors_16("matrix_mult_16_16", matrix_mult_output_q, matrix_mult_expected_output, MATRIX_MULT_MATRIX_HEIGHT, 0);
  case kAxonAfSigmoid:
    return verify_vectors_16("matrix_mult_16_16 sigmoid", matrix_mult_output_q, matrix_mult_sigmoid_expected_output, MATRIX_MULT_MATRIX_HEIGHT, 1);
  case kAxonAfTanh:
    return verify_vectors_16("matrix_mult_16_16 tanh", matrix_mult_output_q, matrix_mult_tanh_expected_output, MATRIX_MULT_MATRIX_HEIGHT, 2);
  default:
    while(1);
  }
}


#if 0
/*
 * Sets up a  sample matrix mult operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 *
 * In this case, the input data width matches the output data width at 16 bits.
 */
int axon_sample_op_matrix_mult_16_in_16_out(void *axon_instance, AxonOpHandle *axon_op_handle, AxonAfEnum activation_function) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  switch(test_type){
  case AxonMatMult16BitInAndOut:
    testInput.length = MATRIX_MULT_VECTOR_LENGTH;
    testInput.y_length = MATRIX_MULT_MATRIX_HEIGHT;
    testInput.data_width = kAxonDataWidth16;
    testInput.data_packing = kAxonDataPackingEnabled;
    testInput.output_rounding = kAxonRoundingNone;
    testInput.output_af = activation_function;
    testInput.x_in = matrix_mult_input_x;
    testInput.y_in = matrix_mult_input_y;
    testInput.q_out = matrix_mult_output_q;
    testInput.x_stride = kAxonStride1;
    testInput.y_stride = kAxonStride1;
    testInput.q_stride = kAxonStride1;

    AxonHostLog((AxonInstanceStruct*) axon_instance, "AXON test: matrix mult\r\n");
    memset(matrix_mult_output_q, 0, sizeof(matrix_mult_output_q));

    if (NULL == axon_op_handle) {
      if (kAxonResultSuccess > (result = AxonApiMatrixMult(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
        snprintf(axon_log_buffer, sizeof(axon_log_buffer), "matrix mult %d", result);
        AxonHostLog((AxonInstanceStruct*) axon_instance, axon_log_buffer);
      }
    } else {
      // populate the op_handle
      if (kAxonResultSuccess > (result = AxonApiDefineOpMatrixMult(axon_instance, &testInput, axon_op_handle))) {
        snprintf(axon_log_buffer, sizeof(axon_log_buffer), "matrix mult failed %d", result);
        AxonHostLog((AxonInstanceStruct*) axon_instance, axon_log_buffer);
      }
    }
    break;
  case AxonMatMult8BitInAndOut:
    testInput.length = MATRIX_MULT_8BIT_VECTOR_LENGTH;
    testInput.y_length = MATRIX_MULT_8BIT_MATRIX_HEIGHT;
    testInput.data_width = kAxonDataWidth8;
    testInput.data_packing = kAxonDataPackingEnabled;
    testInput.output_rounding = kAxonRoundingNone;
    testInput.output_af = activation_function;
    testInput.x_in = matrix_mult_input_x8;
    testInput.y_in = matrix_mult_input_y8;
    testInput.q_out = matrix_mult_output_q8;
    testInput.x_stride = kAxonStride1;
    testInput.y_stride = kAxonStride1;
    testInput.q_stride = kAxonStride1;

    AxonHostLog((AxonInstanceStruct*) axon_instance, "AXON test: matrix mult\r\n");
    memset(matrix_mult_output_q8, 0, sizeof(matrix_mult_output_q8));

    if (NULL == axon_op_handle) {
        snprintf(axon_log_buffer, sizeof(axon_log_buffer), "matrix mult FAILED. No sync version of this test", result);
        AxonHostLog((AxonInstanceStruct*) axon_instance, axon_log_buffer);
    }
    else
    {
      // populate the op_handle
      if (kAxonResultSuccess > (result = AxonApiDefineOpMatrixMult(axon_instance, &testInput, axon_op_handle))) {
        snprintf(axon_log_buffer, sizeof(axon_log_buffer), "matrix mult failed %d", result);
        AxonHostLog((AxonInstanceStruct*) axon_instance, axon_log_buffer);
      }
    }
    break;
  case AxonMatMult8BitIn32Out:
    testInput.length = MATRIX_MULT_8BIT_VECTOR_LENGTH;
    testInput.y_length = MATRIX_MULT_8BIT_MATRIX_HEIGHT;
    testInput.data_width = kAxonDataWidth8;
    testInput.data_packing = kAxonDataPackingEnabled;
    testInput.output_rounding = kAxonRoundingNone;
    testInput.output_af = activation_function;
    testInput.x_in = matrix_mult_input_x8;
    testInput.y_in = matrix_mult_input_y8;
    testInput.q_out = matrix_mult_output_q32;
    testInput.x_stride = kAxonStride1;
    testInput.y_stride = kAxonStride1;
    testInput.q_stride = kAxonStride1;

    AxonHostLog((AxonInstanceStruct*) axon_instance, "AXON test: matrix mult\r\n");
    memset(matrix_mult_output_q32, 0, sizeof(matrix_mult_output_q32));

    if (NULL == axon_op_handle) {
        snprintf(axon_log_buffer, sizeof(axon_log_buffer), "matrix mult FAILED. No sync version of this test", result);
        AxonHostLog((AxonInstanceStruct*) axon_instance, axon_log_buffer);
    }
    else
    {
      // populate the op_handle
      if (kAxonResultSuccess > (result = AxonApiDefineOpMatrixMult32BitOutput(axon_instance, &testInput, axon_op_handle))) {
        snprintf(axon_log_buffer, sizeof(axon_log_buffer), "matrix mult failed %d", result);
        AxonHostLog((AxonInstanceStruct*) axon_instance, axon_log_buffer);
      }
    }
    break;
  }
  return result;
}
#endif

/*
 * Sets up a sample SQRT operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_SQRT(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = SQRT_EXP_LGN_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = sqrt_input_x;
  testInput.q_out = sqrt_outputs;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  memset(sqrt_outputs, 0, sizeof(sqrt_outputs));

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "SQRT", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiSqrt(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_SQRT_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpSqrt(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_Sqrt()
 */
AxonResultEnum axon_sample_op_SQRT_verify(void *axon_instance) {
  return verify_vectors("SQRT", sqrt_outputs, sqrt_expected_outputs, SQRT_EXP_LGN_DATA_LENGTH, 0);
}

/*
 * Sets up a sample EXP operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_EXP(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = SQRT_EXP_LGN_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = exp_input_x;
  testInput.q_out = exp_outputs;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  memset(exp_outputs, 0, sizeof(exp_outputs));
  print_sample_op_header((AxonInstanceStruct*) axon_instance, "EXP", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiExp(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_EXP_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpExp(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }
  return result;
}

/*
 * Verifies the results of axon_sample_op_EXP()
 */
AxonResultEnum axon_sample_op_EXP_verify(void *axon_instance) {
  return verify_vectors("EXP", exp_outputs, exp_expected_outputs, SQRT_EXP_LGN_DATA_LENGTH, 0);
}

/*
 * Sets up a sample LOGN operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_LOGN(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = SQRT_EXP_LGN_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = exp_expected_outputs; // use the exp expected outputs so that logn expected results are the input
  testInput.q_out = logn_outputs;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  memset(exp_outputs, 0, sizeof(exp_outputs));
  print_sample_op_header((AxonInstanceStruct*) axon_instance, "LOGN", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiLogn(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_LOGN_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpLogn(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_LOGN()
 */
AxonResultEnum axon_sample_op_LOGN_verify(void *axon_instance) {
  return verify_vectors("LOGN", logn_outputs, exp_input_x, SQRT_EXP_LGN_DATA_LENGTH, 2);
}


/*
 * Sets up a sample Xpy operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XPY(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = xy_vector_op_y_in;
  testInput.q_out = xy_vector_op_xpy_out;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  memset(exp_outputs, 0, sizeof(exp_outputs));

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "XPY", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiXpy(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_XPY_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpXpy(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }
  return result;
}

/*
 * Verifies the results of axon_sample_op_XPY()
 */
AxonResultEnum axon_sample_op_XPY_verify(void *axon_instance) {
  return verify_vectors("XPY", xy_vector_op_xpy_out, xy_vector_op_xpy_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}

/*
 * Sets up a sample Xmy operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XMY(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = xy_vector_op_y_in;
  testInput.q_out = xy_vector_op_xmy_out;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "XMY", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiXmy(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_XMY_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpXmy(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }
  return result;
}

/*
 * Verifies the results of axon_sample_op_XMY()
 */
AxonResultEnum axon_sample_op_XMY_verify(void *axon_instance) {
  return verify_vectors("XMY", xy_vector_op_xmy_out, xy_vector_op_xmy_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}

/*
 * Sets up a sample Xspys operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XSPYS(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = xy_vector_op_y_in;
  testInput.q_out = xy_vector_op_xspys_out;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "XSPYS", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiXspys(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_XSPYS_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpXmy(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_XSPYS_verify()
 */
AxonResultEnum axon_sample_op_XSPYS_verify(void *axon_instance) {
  return verify_vectors("XSPYS", xy_vector_op_xspys_out, xy_vector_op_xspys_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}


/*
 * Sets up a sample Xsmys operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XSMYS(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = xy_vector_op_y_in;
  testInput.q_out = xy_vector_op_xsmys_out;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "XSMYS", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiXsmys(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_XSMYS_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpXsmys(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}
/*
 * Verifies the results of axon_sample_op_XSMYS()
 */
AxonResultEnum axon_sample_op_XSMYS_verify(void *axon_instance) {
  return verify_vectors("XSMYS", xy_vector_op_xsmys_out, xy_vector_op_xsmys_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}


/*
 * Sets up a sample Xty operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XTY(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = xy_vector_op_y_in;
  testInput.q_out = xy_vector_op_xty_out;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "XTY", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiXty(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_XTY_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpXty(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_XTY()
 */
AxonResultEnum axon_sample_op_XTY_verify(void *axon_instance) {
  return verify_vectors("XTY", xy_vector_op_xty_out, xy_vector_op_xty_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}


/*
 * Sets up a sample Xtx operation with the "x" stride = 2. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XTY_stride2(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH / 2;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = xy_vector_op_y_in;
  testInput.q_out = xy_vector_op_xty_stride2_out;
  testInput.x_stride = kAxonStride2;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride2;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "XTY (stride=2)", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiXty(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_XTY_stride2_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpXty(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_XTY_stride2()
 */
AxonResultEnum axon_sample_op_XTY_stride2_verify(void *axon_instance) {
  return verify_vectors("XTY (stride=2)", xy_vector_op_xty_stride2_out, xy_vector_op_xty_stride2_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}



/*
 * Sets up a sample Axpby operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_AXPBY(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = xy_vector_op_y_in;
  testInput.q_out = xy_vector_op_axpby_out;
  testInput.a_in = axpby_a_in;
  testInput.b_in = axpby_b_in;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "AXPBY", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiAxpby(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_AXPBY_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpAxpby(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}
/*
 * Verifies the results of axon_sample_op_AXPBY()
 */
AxonResultEnum axon_sample_op_AXPBY_verify(void *axon_instance) {
  return verify_vectors("AXPBY", xy_vector_op_axpby_out, xy_vector_op_axpby_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}


/*
 * Sets up a sample AXPB operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_AXPB(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = NULL;
  testInput.q_out = xy_vector_op_axpb_out;
  testInput.a_in = axpby_a_in;
  testInput.b_in = axpby_b_in;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "AXPB", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiAxpb(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_AXPB_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpAxpb(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_AXPBY()
 */
AxonResultEnum axon_sample_op_AXPB_verify(void *axon_instance) {
  return verify_vectors("AXPB", xy_vector_op_axpb_out, xy_vector_op_axpb_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}


/*
 * Sets up a sample AxpbyPtr operation (A and B values are passed as pointers
 * allowing them to be modified w/o requiring the operations to be redefined).
 * If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_AXPBYPTR(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = xy_vector_op_y_in;
  testInput.q_out = xy_vector_op_axpby_out;
  testInput.a_in = &axpby_a_in;
  testInput.b_in = &axpby_b_in;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "AXPBYPTR", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiAxpbyPointer(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_AXPBYPTR_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpAxpbyPointer(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_AXPBYPTR()
 */
AxonResultEnum axon_sample_op_AXPBYPTR_verify(void *axon_instance) {
  return verify_vectors("AXPBYPTR", xy_vector_op_axpby_out, xy_vector_op_axpby_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}

/*
 * Sets up a sample AxpbPtroperation(A and B values are passed as pointers
 * allowing them to be modified w/o requiring the operations to be redefined).
 * If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_AXPBPTR(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;

  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = NULL;
  testInput.q_out = xy_vector_op_axpb_out;
  testInput.a_in = &axpby_a_in;
  testInput.b_in = &axpby_b_in;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "AXPBPTR", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiAxpbPointer(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_AXPBPTR_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpAxpbPointer(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_AXPBPTR()
 */
AxonResultEnum axon_sample_op_AXPBPTR_verify(void *axon_instance) {
  return verify_vectors("AXPBPTR", xy_vector_op_axpb_out, xy_vector_op_axpb_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}

/*
 * Sets up a sample Xs operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XS(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = NULL;
  testInput.q_out = xy_vector_op_xs_out;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "XS", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiXs(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_XS_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpXs(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_XS()
 */
AxonResultEnum axon_sample_op_XS_verify(void *axon_instance) {
  return verify_vectors("XS", xy_vector_op_xs_out, xy_vector_op_xs_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}

/*
 * Sets up a sample Relu operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_RELU(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfRelu;
  testInput.x_in = xy_vector_op_relu_in;
  testInput.y_in = NULL;
  testInput.q_out = xy_vector_op_relu_out;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "RELU", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiRelu(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_RELU_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpRelu(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_RELU()
 */
AxonResultEnum axon_sample_op_RELU_verify(void *axon_instance) {
  return verify_vectors("RELU", xy_vector_op_relu_out, xy_vector_op_relu_expected_out, XY_VECTOR_OPS_DATA_LENGTH, 0);
}

/*
 * Sets up a sample Acorr operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_ACORR(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = ACORR_VECTOR_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = acorr_input_x;
  testInput.y_in = NULL;
  testInput.q_out = acorr_out;
  testInput.a_in = acorr_delay;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "ACORR", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiAcorr(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_ACORR_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpAcorr(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_ACORR()
 */
AxonResultEnum axon_sample_op_ACORR_verify(void *axon_instance) {
  return verify_vectors("ACORR", acorr_out, acorr_expected_out, acorr_delay, 0);
}

/*
 * Sets up a sample Mar operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_MAR(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = xy_vector_op_y_in;
  testInput.q_out = &mar_out;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "MAR", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiMar(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_MAR_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpMar(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_MAR()
 */
AxonResultEnum axon_sample_op_MAR_verify(void *axon_instance) {
  return verify_vectors("MAR", &mar_out, &mar_expected_out, 1, 0);
}

/*
 * Sets up a sample L2norm operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_L2NORM(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = NULL;
  testInput.q_out = &l2norm_out;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "L2NORM", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiL2norm(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_L2NORM_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpL2norm(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}

/*
 * Verifies the results of axon_sample_op_L2NORM()
 */
AxonResultEnum axon_sample_op_L2NORM_verify(void *axon_instance) {
  return verify_vectors("L2NORM", &l2norm_out, &l2norm_expected_out, 1, 0);
}

/*
 * Sets up a sample Acc operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_ACC(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;
  AxonInputStruct testInput;
  testInput.length = XY_VECTOR_OPS_DATA_LENGTH;
  testInput.data_width = kAxonDataWidth24;
  testInput.data_packing = kAxonDataPackingDisabled;
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.x_in = xy_vector_op_x_in;
  testInput.y_in = NULL;
  testInput.q_out = &acc_out;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "ACC", axon_op_handle==NULL);
  if (NULL == axon_op_handle) {
    if (kAxonResultSuccess <= (result = AxonApiAcc(axon_instance, &testInput, kAxonAsyncModeSynchronous))) {
      result = axon_sample_op_ACC_verify(axon_instance);
    }
  } else {
    // populate the op_handle
    result = AxonApiDefineOpAcc(axon_instance, &testInput, axon_op_handle);
  }

  if (kAxonResultSuccess > result) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED! %d\r\n", result);
  }

  return result;
}


/*
 * Verifies the results of axon_sample_op_ACC()
 */
AxonResultEnum axon_sample_op_ACC_verify(void *axon_instance) {
  return verify_vectors("ACC", &acc_out, &acc_expected_out, 1, 0);
}

/*
 * Sets up a sample memcpy operation. If axon_op_handle is NULL, user is directed to use a memcpy. If it is not NULL, the operation is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_MEMCPY(void *axon_instance, AxonOpHandle *axon_op_handle) {
  AxonResultEnum result;

  print_sample_op_header((AxonInstanceStruct*) axon_instance, "MEMCPY", axon_op_handle==NULL);

  if (NULL == axon_op_handle) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED. No discrete version of this operation.\r\n");
    return kAxonResultFailure;
  }

  AxonInputStruct testInput;

  // must set these but they are not used
  testInput.output_rounding = kAxonRoundingNone;
  testInput.output_af = kAxonAfDisabled;
  testInput.y_in = NULL;
  testInput.x_stride = kAxonStride1;
  testInput.y_stride = kAxonStride1;
  testInput.q_stride = kAxonStride1;
  testInput.data_width = kAxonDataWidth8;
  testInput.data_packing = kAxonDataPackingEnabled;
  testInput.x_in = memcpy_in;
  testInput.q_out = memcpy_out;
  testInput.length = MEMCPY_VECTOR_LENGTH;
  testInput.y_length = 0; // no additional 0 padding after the copy.

  // populate the op_handle
  if (kAxonResultSuccess > (result = AxonApiDefineOpMemCpy(axon_instance, &testInput, axon_op_handle))) {
    axon_printf((AxonInstanceStruct*) axon_instance, "FAILED %d\r\n", result);
  }
  return result;
}

/*
 * Verifies the results of axon_sample_op_MEMCPY()
 */
AxonResultEnum axon_sample_op_ACC_MEMCPY(void *axon_instance) {
  return verify_vectors_8("MEMCPY", memcpy_out, memcpy_in, MEMCPY_VECTOR_LENGTH,0);
}


