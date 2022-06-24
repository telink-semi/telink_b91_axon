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
#include <stdarg.h>
#include <string.h>
#include "axon_api.h"
#include "axon_dep.h"
#include "axon_demo_private.h"

/*
 * enum for each type of matrix multiplication demo
 */
enum {
  AxonMatMult16BitInAndOut,
  AxonMatMult8BitInAndOut,
  AxonMatMult8BitIn32Out,
};

extern uint32_t AxonAppGetAsyncNotificationCount();

extern AxonInstanceStruct *gl_axon_instance;
void *gl_axon_handle;


void axon_app_gpio_irq_handler(void){
  //no handling
}
void axon_app_gpio_risc1_irq_handler(void){
  //no handling
}
void axon_app_timer0_irq_handler(void){
  //no handling
}

/*
 *
 */
int AxonAppPrepare(void *unused) {
  gl_axon_handle = gl_axon_instance;
  return kAxonResultSuccess;
}


/*
 * Sample for running ops discretely and synchronously.
 */
static int AxonDiscreteOpDemo() {
  int failure_cnt = 0;
  axon_printf(gl_axon_instance, "\r\nAxon Discrete Ops START\r\n\r\n");
  // make sure axon is powered on
  AxonHostAxonEnable(0);

  // don't chain ops in the Isr, we'll process async notification count.
  AxonAppSetChainAxonOpsInIsrEnabled(0);

  /*
   * FFT and FIR discretely and synchronously...
   */
  failure_cnt += axon_sample_op_FFT(gl_axon_instance, NULL) < kAxonResultSuccess;
  failure_cnt += axon_sample_op_FIR(gl_axon_instance, NULL) < kAxonResultSuccess;

  failure_cnt += axon_sample_op_MATRIX_MULT_16_in_16_out(gl_axon_instance, NULL, kAxonAfDisabled) < kAxonResultSuccess;
  failure_cnt += axon_sample_op_MATRIX_MULT_16_in_16_out(gl_axon_instance, NULL, kAxonAfSigmoid)< kAxonResultSuccess;
  failure_cnt += axon_sample_op_MATRIX_MULT_16_in_16_out(gl_axon_instance, NULL, kAxonAfTanh)< kAxonResultSuccess;

  // square root
  failure_cnt += (axon_sample_op_SQRT(gl_axon_instance, NULL) < kAxonResultSuccess);

  // exp
  failure_cnt += (axon_sample_op_EXP(gl_axon_instance, NULL) < kAxonResultSuccess);

  // logn.
  failure_cnt += (axon_sample_op_LOGN(gl_axon_instance, NULL) < kAxonResultSuccess);

  // Xpy.
  failure_cnt += (axon_sample_op_XPY(gl_axon_instance, NULL) < kAxonResultSuccess);

  // xmy.
  failure_cnt += (axon_sample_op_XMY(gl_axon_instance, NULL) < kAxonResultSuccess);

  // xspys.
  failure_cnt += (axon_sample_op_XSPYS(gl_axon_instance, NULL) < kAxonResultSuccess);

  // xsmys.
  failure_cnt += (axon_sample_op_XSMYS(gl_axon_instance, NULL) < kAxonResultSuccess);

  // xty.
  failure_cnt += (axon_sample_op_XTY(gl_axon_instance, NULL) < kAxonResultSuccess);

  // xty stride2.
  failure_cnt += (axon_sample_op_XTY_stride2(gl_axon_instance, NULL) < kAxonResultSuccess);

  // axpby.
  failure_cnt += (axon_sample_op_AXPBY(gl_axon_instance, NULL) < kAxonResultSuccess);

  // axpb.
  failure_cnt += (axon_sample_op_AXPB(gl_axon_instance, NULL) < kAxonResultSuccess);

  // axpbyPointer.
  failure_cnt += (axon_sample_op_AXPBYPTR(gl_axon_instance, NULL) < kAxonResultSuccess);

  // axpbPointer.
  failure_cnt += (axon_sample_op_AXPBPTR(gl_axon_instance, NULL) < kAxonResultSuccess);

  // xs.
  failure_cnt += (axon_sample_op_XS(gl_axon_instance, NULL) < kAxonResultSuccess);

  // relu.
  failure_cnt += (axon_sample_op_RELU(gl_axon_instance, NULL) < kAxonResultSuccess);

  // acorr
  failure_cnt += (axon_sample_op_ACORR(gl_axon_instance, NULL) < kAxonResultSuccess);

  // mar
  failure_cnt += (axon_sample_op_MAR(gl_axon_instance, NULL) < kAxonResultSuccess);

  // l2norm
  failure_cnt += (axon_sample_op_L2NORM(gl_axon_instance, NULL) < kAxonResultSuccess);

  // acc
  failure_cnt += (axon_sample_op_ACC(gl_axon_instance, NULL) < kAxonResultSuccess);

  axon_printf(gl_axon_instance, "\r\nAxon Discrete Ops COMPLETE - %d failures\r\n\r\n", failure_cnt);
  return failure_cnt;
}

/*
 * Demo for running a single batch of operations.
 * There are four variations:
 * 1) Asynchronous without WFI
 *    Asynchronous => driver starts the batch then returns immediately. Must wait
 *                    for the indication in the ISR that batch is complete.
 *    Without WFI => WFI is never executed so the CPU is polling the "signal" flag continuously.
 * 2) Asynchronous with WFI
 *    Asynchronous => driver starts the batch then returns immediately. Must wait
 *                    for the indication in the ISR that batch is complete.
 *    WFI => Execute WFI when waiting for the "signal" flag to be set. Saves power but
 *           the rest of the system must be idle.
 * 3) Synchronous without WFI
 *    Synchronous => driver does not return until the batch is completed.
 *    Without WFI => driver implements a polling loop without executing WFI between polls.
 *
 * 3) Synchronous with WFI
 *    Synchronous => driver does not return until the batch is completed.
 *    with WFI => driver executes WFI after each unsuccessful poll of the hardware status.
 */
int AxonSingleBatchDemo() {
  int failure_cnt = 0;
  AxonOpHandle op_handles[10]; // handles to operations that can be executed repeatedly
  AxonResultEnum result;
  axon_printf(gl_axon_instance, "\r\nAxon Single Batch START\r\n\r\n");
  // make sure axon is powered on
  AxonHostAxonEnable(0);
  // don't chain ops in the Isr, we'll process async notification count.
  AxonAppSetChainAxonOpsInIsrEnabled(0);

  // define 9 operations in advance. These will be used by all 4 demo variations.
  failure_cnt += (axon_sample_op_FFT(gl_axon_instance, &op_handles[0]) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_FIR(gl_axon_instance, &op_handles[1]) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_SQRT(gl_axon_instance, &op_handles[2]) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_EXP(gl_axon_instance, &op_handles[3]) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_LOGN(gl_axon_instance, &op_handles[4]) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_MEMCPY(gl_axon_instance, &op_handles[5]) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_MATRIX_MULT_16_in_16_out(gl_axon_instance, &op_handles[6], kAxonAfSigmoid) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_AXPBPTR(gl_axon_instance, &op_handles[7]) < kAxonResultSuccess);
  failure_cnt += (axon_sample_op_AXPBYPTR(gl_axon_instance, &op_handles[8]) < kAxonResultSuccess);

  // don't run if any failures.
  if (failure_cnt) {
    axon_printf(gl_axon_instance, "FAILED\r\n");
    return failure_cnt;
  }

  // outer loop: iterate through the 4 batch variations.
  for (uint8_t batch_mode = 0; batch_mode < 4; batch_mode++) {

    const char *msg;
    AxonAsyncModeEnum async_mode;
    switch (batch_mode) {
    case 0: // async, no wfi
      msg = "batch mode async without WFI\r\n";
      async_mode = kAxonAsyncModeAsynchronous;
      break;
    case 1:
      msg = "batch mode async with explicit WFI\r\n";
      async_mode = kAxonAsyncModeAsynchronous;
      break;
    case 2:
      msg = "batch mode sync without internal WFI\r\n";
      async_mode = kAxonAsyncModeSynchronous;
      break;
    case 3:
      msg = "batch mode sync with internal WFI\r\n";
      async_mode = kAxonAsyncModeSyncWithWfi;
      break;
    }

    axon_printf(gl_axon_instance, msg);

    /*
     * The ISR will increment AxonAppGetAsyncNotificationCount
     * every time it fires. Save the current value while axon is idle, then
     * poll for this number to change.
     */
    int pre_axon_async_notification_count = AxonAppGetAsyncNotificationCount();


    if (kAxonResultSuccess > (result = AxonApiExecuteOps(gl_axon_instance, 9, op_handles, async_mode))) {
      failure_cnt++;
      axon_printf(gl_axon_instance, "\r\n ExecuteOps FAILED (%d)\r\n", result);
      break;
    }

    // in asynchronous mode, need to poll for completion.
    if (kAxonAsyncModeAsynchronous == async_mode) {
      // in async mode we need to process interrupts.

      do {

        /*
         * Wait for the asynchronous notification
         */
        while (pre_axon_async_notification_count == AxonAppGetAsyncNotificationCount()) {
          if (batch_mode == 1) { // explicit WFI
            /*
             *  there is a race condition here. Don't enter WFI
             *  until we're sure that the Axon interrupt hasn't already fired.
             */
            uint32_t interrupt_state = AxonHostDisableInterrupts();
            if (pre_axon_async_notification_count == AxonAppGetAsyncNotificationCount()) {
              AxonHostWfi();
            }
            AxonHostRestoreInterrupts(interrupt_state);
          }
        }
        pre_axon_async_notification_count = AxonAppGetAsyncNotificationCount();

        /*
         * go get the results. Just because we got an interrupt doesn't mean axon
         * is complete. Query the driver to see if it is complete.
         */
        result = AxonApiGetAsyncResult(gl_axon_instance);
      } while (result == kAxonResultNotFinished);
    } // if kAxonAsyncModeAsynchronous


    // axon has completed.

    // verify the results of each of the ops in the batch
    failure_cnt += (axon_sample_op_FFT_verify(gl_axon_instance) < kAxonResultSuccess);
    failure_cnt += (axon_sample_op_FIR_verify(gl_axon_instance) < kAxonResultSuccess);
    failure_cnt += (axon_sample_op_SQRT_verify(gl_axon_instance) < kAxonResultSuccess);
    failure_cnt += (axon_sample_op_SQRT_verify(gl_axon_instance) < kAxonResultSuccess);
    failure_cnt += (axon_sample_op_LOGN_verify(gl_axon_instance) < kAxonResultSuccess);
    failure_cnt += (axon_sample_op_ACC_MEMCPY(gl_axon_instance) < kAxonResultSuccess);
    failure_cnt += (axon_sample_op_MATRIX_MULT_16_in_16_out_verify(gl_axon_instance, kAxonAfSigmoid) < kAxonResultSuccess);
    failure_cnt += (axon_sample_op_AXPB_verify(gl_axon_instance) < kAxonResultSuccess);
    failure_cnt += (axon_sample_op_AXPBYPTR_verify(gl_axon_instance) < kAxonResultSuccess);
  } // outer loop

  /*
   * All done (almost), free up these op handles.
   */
  if (kAxonResultSuccess > (result = AxonApiFreeOpHandles(gl_axon_instance, 9, op_handles))) {
    failure_cnt++;
    axon_printf(gl_axon_instance, "\r\n FreeOpHandles FAILED (%d)\r\n", result);
  }
  axon_printf(gl_axon_instance, "\r\nAxon Single Batch COMPLETE - %d failures\r\n\r\n", failure_cnt);
  return failure_cnt;
}
/*
 * Top-level entry function.
 * This dispatches the various demos.
 */
int AxonAppRun() {

  int failure_cnt = 0;

  failure_cnt += AxonDiscreteOpDemo();
  failure_cnt += AxonSingleBatchDemo();
  failure_cnt += AxonDemoQueuedBatches();

  axon_printf(gl_axon_instance, "\r\nAll Demos Complete - %d failures\r\n", failure_cnt);

  return failure_cnt == 0 ? kAxonResultSuccess : kAxonResultFailure;
}
