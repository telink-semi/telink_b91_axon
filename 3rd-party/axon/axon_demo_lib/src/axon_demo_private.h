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
 * This file has defines/declarations that are used within the demo but not needed outside
 * the demo library.
 */

#include <stdint.h>
#include "axon_api.h"
#pragma once

/*
 * Handle to the axon driver.
 */
extern void *gl_axon_handle;

/*
 * Calling this with true means that AxonApiGetAsyncResult() will be called
 * in the Axon ISR. This is necessary for the queued batches demo as it relies
 * on chaining operations together in the interrupt context.
 */
void AxonAppSetChainAxonOpsInIsrEnabled(bool value);
/*
 * entry point to queued batches demo.
 */
int AxonDemoQueuedBatches();


/*
 * Sets up a sample FFT operation. If axon_op_handle is NULL, the operation
 * is executed discretely. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
AxonResultEnum axon_sample_op_FFT(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_of_FFT()
 */
AxonResultEnum axon_sample_op_FFT_verify(void *axon_instance);


/*
 * Sets up a sample  FIR operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_FIR(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_of_FIR()
 */
AxonResultEnum axon_sample_op_FIR_verify(void *axon_instance);

/*
 * Sets up a  sample matrix mult operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 *
 * In this case, the input data width matches the output data width at 16 bits.
 * User can also specify an activation function.
 */
int axon_sample_op_MATRIX_MULT_16_in_16_out(void *axon_instance, AxonOpHandle *axon_op_handle, AxonAfEnum activation_function);
/*
 * Verifies the results of axon_sample_op_MATRIX_MULT_16_in_16_out()
 */
AxonResultEnum axon_sample_op_MATRIX_MULT_16_in_16_out_verify(void *axon_instance, AxonAfEnum activation_function);


/*
 * Sets up a sample SQRT operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_SQRT(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_Sqrt()
 */
AxonResultEnum axon_sample_op_SQRT_verify(void *axon_instance);

/*
 * Sets up a sample EXP operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_EXP(void *axon_instance, AxonOpHandle *axon_op_handle);

/*
 * Verifies the results of axon_sample_op_EXP()
 */
AxonResultEnum axon_sample_op_EXP_verify(void *axon_instance);


/*
 * Sets up a sample LOGN operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_LOGN(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_LOGN()
 */
AxonResultEnum axon_sample_op_LOGN_verify(void *axon_instance);

/*
 * Sets up a sample Xpy operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XPY(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_XPY()
 */
AxonResultEnum axon_sample_op_XPY_verify(void *axon_instance);

/*
 * Sets up a sample Xmy operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XMY(void *axon_instance, AxonOpHandle *axon_op_handle);


/*
 * Verifies the results of axon_sample_op_XMY()
 */
AxonResultEnum axon_sample_op_XMY_verify(void *axon_instance);


/*
 * Sets up a sample Xspys operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XSPYS(void *axon_instance, AxonOpHandle *axon_op_handle);

/*
 * Verifies the results of axon_sample_op_XSPYS_verify()
 */
AxonResultEnum axon_sample_op_XSPYS_verify(void *axon_instance);

/*
 * Sets up a sample Xsmys operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XSMYS(void *axon_instance, AxonOpHandle *axon_op_handle);

/*
 * Verifies the results of axon_sample_op_XSMYS()
 */
AxonResultEnum axon_sample_op_XSMYS_verify(void *axon_instance);

/*
 * Sets up a sample Xty operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XTY(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_XTY()
 */
AxonResultEnum axon_sample_op_XTY_verify(void *axon_instance);
/*
 * Sets up a sample Xtx operation with the "x" stride = 2. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XTY_stride2(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_XTY_stride2()
 */
AxonResultEnum axon_sample_op_XTY_stride2_verify(void *axon_instance);

/*
 * Sets up a sample Axpby operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_AXPBY(void *axon_instance, AxonOpHandle *axon_op_handle);

/*
 * Verifies the results of axon_sample_op_AXPBY()
 */
AxonResultEnum axon_sample_op_AXPBY_verify(void *axon_instance);

/*
 * Sets up a sample AXPB operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_AXPB(void *axon_instance, AxonOpHandle *axon_op_handle);

/*
 * Verifies the results of axon_sample_op_AXPBY()
 */
AxonResultEnum axon_sample_op_AXPB_verify(void *axon_instance);

/*
 * Sets up a sample AxpbyPtr operation (A and B values are passed as pointers
 * allowing them to be modified w/o requiring the operations to be redefined).
 * If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_AXPBYPTR(void *axon_instance, AxonOpHandle *axon_op_handle);

/*
 * Verifies the results of axon_sample_op_AXPBYPTR()
 */
AxonResultEnum axon_sample_op_AXPBYPTR_verify(void *axon_instance);

/*
 * Sets up a sample AxpbPtroperation(A and B values are passed as pointers
 * allowing them to be modified w/o requiring the operations to be redefined).
 * If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_AXPBPTR(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_AXPBPTR()
 */
AxonResultEnum axon_sample_op_AXPBPTR_verify(void *axon_instance);

/*
 * Sets up a sample Xs operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_XS(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_XS()
 */
AxonResultEnum axon_sample_op_XS_verify(void *axon_instance);
/*
 * Sets up a sample Relu operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_RELU(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_RELU()
 */
AxonResultEnum axon_sample_op_RELU_verify(void *axon_instance);

/*
 * Sets up a sample Acorr operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_ACORR(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_ACORR()
 */
AxonResultEnum axon_sample_op_ACORR_verify(void *axon_instance);
/*
 * Sets up a sample Mar operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_MAR(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_MAR()
 */
AxonResultEnum axon_sample_op_MAR_verify(void *axon_instance);

/*
 * Sets up a sample L2norm operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_L2NORM(void *axon_instance, AxonOpHandle *axon_op_handle);
/*
 * Verifies the results of axon_sample_op_L2NORM()
 */
AxonResultEnum axon_sample_op_L2NORM_verify(void *axon_instance);

/*
 * Sets up a sample Acc operation. If axon_op_handle is NULL, the operation
 * is executed directly. If it is not NULL, the operations is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_ACC(void *axon_instance, AxonOpHandle *axon_op_handle);

/*
 * Verifies the results of axon_sample_op_ACC()
 */
AxonResultEnum axon_sample_op_ACC_verify(void *axon_instance);

/*
 * Sets up a sample memcpy operation. If axon_op_handle is NULL, user is directed to use a memcpy. If it is not NULL, the operation is defined and can be run
 * as part of a sequence.
 */
int axon_sample_op_MEMCPY(void *axon_instance, AxonOpHandle *axon_op_handle);

/*
 * Verifies the results of axon_sample_op_MEMCPY()
 */
AxonResultEnum axon_sample_op_ACC_MEMCPY(void *axon_instance);

/********************************************************************************
 * utilities
 ********************************************************************************/
/*
 * Logging function.
 */
extern void axon_printf(void *axon_handle, char *fmt_string, ...);

/*
 * Verifies output[] == expected_output[] with each value being within margin of each other.
 */
extern int verify_vectors(char *msg, int32_t *output, int32_t* expected_output, uint32_t count, uint32_t margin);

  /*
   * verifies int16_t vectors
   */
int verify_vectors_16(char *msg, int16_t *output, int16_t* expected_output, uint32_t count, uint32_t margin);
/*
 * verifies int8_t vectors
 */
int verify_vectors_8(char *msg, int8_t *output, int8_t* expected_output, uint32_t count, uint32_t margin);

/*
 * demo vector forward declarations.
 * The values for these vectors are in axon_demo_vectors.c
 */
/*
 * matrix multiply 8 bit
 */
#define MATRIX_MULT_8BIT_VECTOR_LENGTH 23

extern int8_t matrix_mult_input_x8[MATRIX_MULT_8BIT_VECTOR_LENGTH];
#define MATRIX_MULT_8BIT_MATRIX_HEIGHT 5

extern const int8_t matrix_mult_input_y8[MATRIX_MULT_8BIT_MATRIX_HEIGHT][MATRIX_MULT_8BIT_VECTOR_LENGTH];
// 8 bit in and out
extern int8_t matrix_mult_output_q8[MATRIX_MULT_8BIT_MATRIX_HEIGHT];

extern const int8_t matrix_mult_expected_output8[MATRIX_MULT_8BIT_MATRIX_HEIGHT];

//8 bit in and 32 bit out
extern int32_t matrix_mult_output_q32[MATRIX_MULT_8BIT_MATRIX_HEIGHT];

extern const int32_t matrix_mult_expected_output32[MATRIX_MULT_8BIT_MATRIX_HEIGHT];

/*
 * matrix multiply 16 bit
 */
#define MATRIX_MULT_VECTOR_LENGTH 24

extern int16_t matrix_mult_input_x[MATRIX_MULT_VECTOR_LENGTH];
#define MATRIX_MULT_MATRIX_HEIGHT 6

extern const int16_t matrix_mult_input_y[MATRIX_MULT_MATRIX_HEIGHT][MATRIX_MULT_VECTOR_LENGTH];

extern int16_t matrix_mult_output_q[MATRIX_MULT_MATRIX_HEIGHT];

extern const int16_t matrix_mult_expected_output[MATRIX_MULT_MATRIX_HEIGHT];

extern const int16_t matrix_mult_sigmoid_expected_output[MATRIX_MULT_MATRIX_HEIGHT];

extern const int16_t matrix_mult_tanh_expected_output[MATRIX_MULT_MATRIX_HEIGHT];

#define MEMCPY_VECTOR_LENGTH 16

extern int8_t memcpy_in[MEMCPY_VECTOR_LENGTH];

extern int8_t memcpy_out[MEMCPY_VECTOR_LENGTH];

/*
 * FIR vectors
 */
#define  FIR_DATA_LENGTH  20
#define  FIR_FILTER_LENGTH 12
extern int32_t fir_outputs[FIR_DATA_LENGTH];

extern int32_t fir_input_x[FIR_DATA_LENGTH];

extern int32_t fir_input_F[FIR_FILTER_LENGTH];

extern const int32_t fir_expected_outputs[FIR_DATA_LENGTH];

/*
 * SQRT vectors
 */
#define SQRT_EXP_LGN_DATA_LENGTH 8

extern int32_t sqrt_input_x[SQRT_EXP_LGN_DATA_LENGTH];

extern int32_t sqrt_outputs[SQRT_EXP_LGN_DATA_LENGTH];

/**
 * SQRT expected results
 */
extern const int32_t sqrt_expected_outputs[SQRT_EXP_LGN_DATA_LENGTH];

/*
 * EXP vectors
 */
extern int32_t exp_input_x[SQRT_EXP_LGN_DATA_LENGTH];

extern int32_t exp_outputs[SQRT_EXP_LGN_DATA_LENGTH];

extern int32_t exp_expected_outputs[SQRT_EXP_LGN_DATA_LENGTH];

/*
 * For logn, the exp expected values will be the input, and the exp input will
 * be the expected outputs
 */
extern int32_t logn_outputs[SQRT_EXP_LGN_DATA_LENGTH];


/*
 * All XY vector operations will use the same input vectors
 */
#define XY_VECTOR_OPS_DATA_LENGTH 8

extern int32_t xy_vector_op_x_in[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_y_in[XY_VECTOR_OPS_DATA_LENGTH];

// Xpy
extern const int32_t xy_vector_op_xpy_expected_out[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_xpy_out[XY_VECTOR_OPS_DATA_LENGTH];

// xmy
extern const int32_t xy_vector_op_xmy_expected_out[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_xmy_out[XY_VECTOR_OPS_DATA_LENGTH];

// xspys
extern const int32_t xy_vector_op_xspys_expected_out[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_xspys_out[XY_VECTOR_OPS_DATA_LENGTH];

// xsmys
extern const int32_t xy_vector_op_xsmys_expected_out[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_xsmys_out[XY_VECTOR_OPS_DATA_LENGTH];

// xty
extern const int32_t xy_vector_op_xty_expected_out[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_xty_out[XY_VECTOR_OPS_DATA_LENGTH];

// xty stride=2 on x and q
extern const int32_t xy_vector_op_xty_stride2_expected_out[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_xty_stride2_out[XY_VECTOR_OPS_DATA_LENGTH];

// axpby
extern int32_t axpby_a_in;
extern int32_t axpby_b_in;

extern const int32_t xy_vector_op_axpby_expected_out[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_axpby_out[XY_VECTOR_OPS_DATA_LENGTH];

// axpb will use same a & b as axpb
extern const int32_t xy_vector_op_axpb_expected_out[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_axpb_out[XY_VECTOR_OPS_DATA_LENGTH];

// xs
extern const int32_t xy_vector_op_xs_expected_out[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_xs_out[XY_VECTOR_OPS_DATA_LENGTH];

// relu.
extern int32_t xy_vector_op_relu_in[XY_VECTOR_OPS_DATA_LENGTH];

extern const int32_t xy_vector_op_relu_expected_out[XY_VECTOR_OPS_DATA_LENGTH];
extern int32_t xy_vector_op_relu_out[XY_VECTOR_OPS_DATA_LENGTH];

/*
 * Auto-correlation test vectors
 */
#define ACORR_VECTOR_LENGTH 8
#define ACORR_DELAY 6
extern int32_t acorr_delay;

extern int32_t acorr_input_x[ACORR_VECTOR_LENGTH];

extern const int32_t acorr_expected_out[ACORR_DELAY];
extern int32_t acorr_out[ACORR_DELAY];


/*
 * Mar test vectors.
 * Will use xy_vector_op_x_in, y_in
 */
extern int32_t mar_expected_out;
extern int32_t mar_out;

/*
 * Acc test vectors.
 * Will use xy_vector_op_x_in, y_in
 */
extern int32_t acc_expected_out;
extern int32_t acc_out;

/*
 * L2norm test vectors.
 * Will use xy_vector_op_x_in, y_in
 */
extern int32_t l2norm_expected_out;
extern int32_t l2norm_out;


/*
 * FFT 512 vectors
 */
extern int32_t fft_outputs[1024];


extern const int32_t fft_512_expected[1024];



extern int32_t fft_512_input[1024];
