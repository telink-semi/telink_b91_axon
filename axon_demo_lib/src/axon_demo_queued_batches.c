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
/**
 * This file gives a somewhat complicated demonstration of queue batches.
 *
 * The advantage of queued batches is that a batch can be queued at any time,
 * whether or not axon is busy with a batch. Also,each batch has its own callback function.
 *
 * This makes it easier to share Axon amongst different work-loads without having to coordinate
 * who gets axon when.
 *
 * Note that queued batches are not compatible with single batch nor discrete mode.
 *
 * This demo defines 6 operations and divides them into 3 batches;
 * batch 0 has op 0 (1 ops), batch 1 has ops 1-2(2 ops), batch 2 has ops 3-5 (3 ops).
 *
 * Each batch has it's own callback function, but they funnel into a common callback.
 *
 * The top level code defines the 6 operations, splits them up into the 3 batches, and
 * submits the 3 batches successively.
 *
 * As each batch completes, its callback is invoked. The callback will
 * - verify the results of each operation in the batch.
 * - increment the batch's counter.
 * - resubmit the batch if the counter is less than or equal to the batch number.
 *
 * The net result is that batch 0 runs once, batch 1 runs twice, and batch 2 runs 3 times.
 * The top level code polls the batch counters and returns when the counter values are 1, 2, and 3,
 * respectively, for batch 0, 1 and 2.
 */


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "axon_api.h"
#include "axon_dep.h"
#include "axon_demo_private.h"

/*
 * Identifies each batch in the callback functions.
 * Each batch will have it's position number of operations + 1 (ie, batch 0 has 1, batch 1 has 2, batch 2 has 3)
 */
enum {
  kQueuedBatch0,
  kQueuedBatch1,
  kQueuedBatch2,
  kQueuedBatchCnt
};

/*
 * For this demo, 6 operations are defined and split into 3 batches:
 */
enum {
  kQueuedBatchOps0_1stNdx, // batch 0: op 0
  kQueuedBatchOps1_1stNdx = kQueuedBatchOps0_1stNdx + (kQueuedBatch0+1), // batch 1: op 1-2
  kQueuedBatchOps2_1stNdx = kQueuedBatchOps1_1stNdx + (kQueuedBatch1+1), // batch 2: op 3-5
  kQueuedBatchOpsCnt = kQueuedBatchOps2_1stNdx + (kQueuedBatch2+1), // 6 total ops defined.
};

/*
 * state info for queued batch mode
 */
typedef struct {
  uint8_t failure_cnt;
  uint8_t resv[3];
  AxonOpHandle op_handles[kQueuedBatchOpsCnt]; // handles to operations that can be executed repeatedly

  AxonMgrQueuedOpsStruct queued_ops[kQueuedBatchCnt];
  uint8_t completed_cnt[kQueuedBatchCnt]; // each batch will be submitted the number of times as it's position (ie, 1, 2, & 3)
  struct { // this struct holds verification information for each of the operations.
    char *msg;
    int32_t *output;
    int32_t* expected_output;
    uint32_t count;
    uint32_t margin;
  } verify_params[kQueuedBatchOpsCnt];
} queued_batch_demo_state_struct;

static void queued_batch_ops_0_callback(AxonResultEnum result, void *callback_context);
static void queued_batch_ops_1_callback(AxonResultEnum result, void *callback_context);
static void queued_batch_ops_2_callback(AxonResultEnum result, void *callback_context);

queued_batch_demo_state_struct queued_batch_demo_state = {
    .queued_ops = {
        {.op_handle_list = queued_batch_demo_state.op_handles+kQueuedBatchOps0_1stNdx,.op_handle_count = kQueuedBatch0 + 1, .callback_context = (void *)kQueuedBatch0,.callback_function = queued_batch_ops_0_callback },
        {.op_handle_list = queued_batch_demo_state.op_handles+kQueuedBatchOps1_1stNdx,.op_handle_count = kQueuedBatch1 + 1, .callback_context = (void *)kQueuedBatch1,.callback_function = queued_batch_ops_1_callback },
        /* DEBUG! ELIMINATE THE MATRIX MULT OP */
        {.op_handle_list = queued_batch_demo_state.op_handles+kQueuedBatchOps2_1stNdx,.op_handle_count = kQueuedBatch2, .callback_context = (void *)kQueuedBatch2,.callback_function = queued_batch_ops_2_callback },
        // {.op_handle_list = queued_batch_demo_state.op_handles+kQueuedBatchOps2_1stNdx,.op_handle_count = kQueuedBatch2 + 1, .callback_context = (void *)kQueuedBatch2,.callback_function = queued_batch_ops_2_callback },
    },
    .verify_params = {
        {.msg="FFT", .output=fft_outputs, .expected_output=fft_512_expected, .count=1024, .margin=0},
        {.msg="FIR", .output=&fir_outputs[FIR_FILTER_LENGTH], .expected_output=&fir_expected_outputs[FIR_FILTER_LENGTH], .count=FIR_DATA_LENGTH - FIR_FILTER_LENGTH, .margin=0},
        {.msg="SQRT", .output=sqrt_outputs, .expected_output=sqrt_expected_outputs, .count=SQRT_EXP_LGN_DATA_LENGTH, .margin=0},
        {.msg="EXP", .output=exp_outputs, .expected_output=exp_expected_outputs, .count=SQRT_EXP_LGN_DATA_LENGTH, .margin=0},
        {.msg="LOGN", .output=logn_outputs, .expected_output=exp_input_x, .count=SQRT_EXP_LGN_DATA_LENGTH, .margin=2},
        {.msg="Matrix Mult", .output=matrix_mult_output_q, .expected_output=matrix_mult_sigmoid_expected_output, .count=MATRIX_MULT_MATRIX_HEIGHT/2, .margin=1}, // length div2 'cuz this is int16 output but we use int32 comparison
    },
};

static void queued_batch_ops_common_callback(AxonResultEnum result, void *callback_context, uint8_t op_ndx, uint8_t op_cnt) {

  // context value provided to axon_driver was the batch number (0-2)
  uint8_t which_batch = (uint8_t)callback_context;

  axon_printf(gl_axon_handle, "queued batch callback: which batch %d\r\n",(int)which_batch );

  if (which_batch >= kQueuedBatchCnt) {
    queued_batch_demo_state.failure_cnt++;
    axon_printf(gl_axon_handle, "FAILED - bad context!\r\n");
    while(1);
    return;

  }

  if (result != kAxonResultSuccess) {
    // FAILURE!!!
    queued_batch_demo_state.failure_cnt++;
    axon_printf(gl_axon_handle, "AXON FAILED! %d\r\n", result);
    while(1);
    return;
  }
  queued_batch_demo_state.completed_cnt[which_batch]++;
  if ((which_batch+1)<queued_batch_demo_state.completed_cnt[which_batch]) {
    // FAILURE!!! CALLBACK IS CALLED TOO MANY TIMES!
    queued_batch_demo_state.failure_cnt++;
    axon_printf(gl_axon_handle, "CALLED BACK TOO MANY TIMES! %d\r\n", queued_batch_demo_state.completed_cnt[which_batch]);
    while(1);
    return;
  }

  // verify the results
  while(op_cnt--) {
    queued_batch_demo_state.failure_cnt+=(verify_vectors(
        queued_batch_demo_state.verify_params[op_ndx].msg,
        queued_batch_demo_state.verify_params[op_ndx].output,
        queued_batch_demo_state.verify_params[op_ndx].expected_output,
        queued_batch_demo_state.verify_params[op_ndx].count,
        queued_batch_demo_state.verify_params[op_ndx].margin) < kAxonResultSuccess);
    op_ndx++;
  }

  // queue it up again if more to do
  if (which_batch>queued_batch_demo_state.completed_cnt[which_batch]-1) {
    AxonApiQueueOpsList(gl_axon_handle, queued_batch_demo_state.queued_ops+which_batch);
  } else {
    AxonHostLog(gl_axon_handle, "Last queued batch completed.\r\n");
  }

}

/**
 * queued batch mode callbacks
 */
static void queued_batch_ops_0_callback(AxonResultEnum result, void *callback_context) {
  queued_batch_ops_common_callback(result, callback_context, kQueuedBatchOps0_1stNdx, kQueuedBatch0+1);
}
static void queued_batch_ops_1_callback(AxonResultEnum result, void *callback_context) {
  queued_batch_ops_common_callback(result, callback_context, kQueuedBatchOps1_1stNdx, kQueuedBatch1+1);
}
static void queued_batch_ops_2_callback(AxonResultEnum result, void *callback_context) {
  queued_batch_ops_common_callback(result, callback_context, kQueuedBatchOps2_1stNdx, kQueuedBatch2);
}

/*
 * This demonstrates queued batches; multiple operation sequences can be queued up, each
 * sequence with a dedicated callback function.
 */
int AxonDemoQueuedBatches() {
  int failure_cnt = 0;
  axon_printf(gl_axon_handle, "\r\nAxon Queued Batches START\r\n\r\n");

  /*
   * The demo adds new batches in the interrupt context.
   */
  AxonAppSetChainAxonOpsInIsrEnabled(1);
  // rest of the demo expects axon to be powered on.
  AxonHostAxonEnable(0);
  memset(queued_batch_demo_state.op_handles, 0, sizeof(queued_batch_demo_state.op_handles));
  memset(queued_batch_demo_state.completed_cnt, 0, sizeof(queued_batch_demo_state.completed_cnt));

  // define the 6 operations. The order has to match the order of the verification records!
  failure_cnt += (axon_sample_op_FFT(gl_axon_handle, queued_batch_demo_state.op_handles+0) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_FIR(gl_axon_handle, queued_batch_demo_state.op_handles+1) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_SQRT(gl_axon_handle, queued_batch_demo_state.op_handles+2) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_EXP(gl_axon_handle, queued_batch_demo_state.op_handles+3) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_LOGN(gl_axon_handle, queued_batch_demo_state.op_handles+4) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_MATRIX_MULT_16_in_16_out(gl_axon_handle, queued_batch_demo_state.op_handles+5, kAxonAfSigmoid) < kAxonResultSuccess);


  // queue up the 3 batches
  for (int lo_ndx=0; lo_ndx<kQueuedBatchCnt;lo_ndx++) {
    AxonApiQueueOpsList(gl_axon_handle, queued_batch_demo_state.queued_ops+lo_ndx);
  }

  // now wait for everything to finish...
  while(1) {
    int lo_ndx;
    // we're done when every batch has run it's position times+ 1 (batch 0 1 time, batch 1 2 times, batch 2 3 times...)
    for ( lo_ndx=0; lo_ndx<kQueuedBatchCnt;lo_ndx++) {
      if (queued_batch_demo_state.completed_cnt[lo_ndx] <= lo_ndx) {
        break;
      }
    }
    // if we made it to the end of the for loop then everything has completed.
    if (lo_ndx==kQueuedBatchCnt) {
      break;
    }
  }
  // free up the op handles
  AxonApiFreeOpHandles(gl_axon_handle, kQueuedBatchOpsCnt, queued_batch_demo_state.op_handles);

  /*
   * restore this to disabled.
   */
  AxonAppSetChainAxonOpsInIsrEnabled(0);


  axon_printf(gl_axon_handle, "\r\nAxon Queued Batches COMPLETE - %d failures\r\n\r\n", failure_cnt);
  return failure_cnt;
}

