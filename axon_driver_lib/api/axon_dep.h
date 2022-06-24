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

#include <stdint.h>
#include <stdbool.h>
#include "axon_api.h"

#pragma once

/**
 * Internal buffer required by axon driver, 1 for each defined operation.
 * Minimum size to support discrete operations is 1. Each additional buffer supports a single predefined
 * operation (for batch mode).
 * - Must be 16 byte aligned.
 * - Must be allocated from retained memory.
 */
typedef uint32_t AxonInternalBuffer[24];

/**
 * Internal buffer required by the axon driver to support matrix multiplication.
 * A minimum of 1 is required to support matrix multiplication, discrete or batch.
 * - Must be 4 byte aligned.
 * - Does NOT need to be in retained memory.
 */
typedef uint32_t AxonMatrixMultBuffer[32];

/**
 * Buffer required to support ACOR operations. Not needed if ACOR is not used.
 * - Must be 16 byte aligned if 8 bit ACOR operations occur.
 * - Must be 8 byte aligned if 16 bit ACOR operations occur.
 */
#define ACOR_BUFFER_LEN (1024)
typedef union {
  int32_t as32[ACOR_BUFFER_LEN];
  int16_t as16[ACOR_BUFFER_LEN*2];
  int8_t as8[ACOR_BUFFER_LEN*4];
} AxonAcorrBuffer;

/*
 * Structure populated by the host before initializing the driver.
 */
typedef struct {
  char * log_buffer;           /**< buffer for storing formatted log messages. If NULL, some log messages will not be formatted */
  uint16_t log_buffer_size;    /**< size (in chars) of log_buffer. Should be 0 if log_buffer is NULL */
  uint16_t internal_buffer_size; /**< size in AxonInternalBuffer's of internal_buffers. Must be at least 1 */
  uint16_t matrix_mult_buffer_size; /**< size of matrix_mult_buffers. Must be at least 1 to support matrix multiply. No more than 16 are needed. */
  uint16_t mm_line_buffer_size;          /**< size of mm_line_buffer measured in words (32-bit values)  */
  uint32_t *base_address;      /**< base address of the axon instance's registers space */
  AxonInternalBuffer *internal_buffers; /**< 1 or more internal buffers for axon to use. This memory must be available while axon is executing */
  AxonAcorrBuffer *acorr_buffer; /**< needed to support acorr operation */
  AxonMatrixMultBuffer *matrix_mult_buffer; /**< needed to support matrix multiply operation */
  uint32_t *mm_line_buffer;       /**< buffer for holding filter rows during within matrix multiply op. Must be sized to hold at least 2 rows of the largest matrix, with each row padded out to the next aligned index */
}AxonInstanceHostProvidedStruct;

/**
 * Other memory used internally by the driver.
 */
typedef uint32_t AxonDriverUseBuffer[16];


/**
 * Axon driver instance structure. This structure is provided to the driver and maintained by the driver.
 * - Must be "permanent" (not a stack variable).
 * - Must be allocated from retained memory.
 */
typedef struct {
  AxonInstanceHostProvidedStruct host_provided; /**< host populates this before calling AxonInitInstance() */
  AxonDriverUseBuffer driver_use;         /**< for internal use by the driver */
} AxonInstanceStruct;



/**
 * Console logging function implemented by host.
 */
void AxonHostLog(AxonInstanceStruct *axon, char *msg);

/**
 * Function is called once per axon hardware instance on the device.
 * axon_instance is then passed as an opaque "handle" to axon operation
 * APIs.
 * @param axon_instance -> populated with all the instance details.
 */
AxonResultEnum AxonInitInstance(AxonInstanceStruct *axon_instance);


/**
 * Called after power has been removed from Axon
 */
AxonResultEnum AxonReInitInstance(AxonInstanceStruct *axon_instance);


/**
 * This function is called by the host's interrupt handler to check for and handle
 * an interrupt on one or more axon instances.
 *
 * This function will clear the interrupt source in Axon so it is no longer
 * asserted to the host, and calls AxonHostInterruptNotification() if there truly is
 * a pending interrupt on Axon.
 *
 * @param axon_instances -> array of axon instances to check.
 * @param axon_instance_count -> number of elements in axon_instances[]
 */
AxonResultEnum AxonHandleInterrupt(AxonInstanceStruct axon_instances[], uint8_t axon_instance_count);

/**
 * This function is invoked by the axon driver's IRQ handler.
 * The host may use this function to signal another execution context to
 * call
 * The host uses this function to post a message/signal to the axon driver thread
 * for the interrupt to be handled in the kernel context.
 */
void AxonHostInterruptNotification(AxonInstanceStruct *axon);


/**
 * Host provides this function to map addresses from "cpu" space to "axon" space. In other words,
 * a buffer address that the host provides to axon might need to be transformed so that axon
 * can access the same memory location.
 */
uint32_t AxonHostTransformAddress(uint32_t from_addr);

/**
 * Host implements this function to support performance profiling.
 */
uint32_t AxonHostGetTime();


/**
 * Host implements these functions to support low-power synchronous mode.
 * Axon driver will call this function when waiting for Axon hardware to
 * complete an operation when performing operations in mode kAxonAsyncModeSyncWithWfi.
 *
 * Axon driver will invoke AxonHostDisableInterrupts() prior to AxonHostWfi(). An axon interrupt
 * will need to wake the system from WFI(). The driver will invoke AxonHostEnableInterrupts()
 * upon return from AxonHostWfi().  Interrupt should not be processed until AxonHostEnableInterrupts()
 * is called.
 *
 * AxonHostWfi() should cause the host to enter a low power state. Note that if another event causes the
 * host to wake from WFI, the axon driver will simply invoke it again.
 *
 * It is possible for AxonHostWfi() to do nothing and simply return, but this behavior is essentially the
 * same as Synchronous mode.
 *
 * Axon driver will occasionally invoke AxonHostDisableInterrupts() in order to inspect a state variable
 * that is set in the ISR, but the duration will be very short before calling AxonHostEnableInterrupts().
 *
 * Axon driver will not nest calls to AxonHostDisableInterrutps (ie, call it multiple times before calling
 * AxonHostEnableInterrupts), so the host need not worry if the state upon entry was with interrupts already
 * disabled.
 */
void AxonHostWfi();
uint32_t AxonHostDisableInterrupts();
void AxonHostRestoreInterrupts(uint32_t restore_value);

/**
 * enables the axon insterrupt specifically
 */
void AxonHostEnableAxonInterrupt();
/**
 * disables the axon insterrupt specifically
 */
void AxonHostDisableAxonInterrupt();

// void AxonHostEnableInterrupts();
/**
 * host function for enabling axon by powering it up and providing a clock.
 */
void AxonHostAxonEnable(uint8_t power_on_reset);

/**
 * host function for disabling axon by powering it down and removing the clock.
 */
void AxonHostAxonDisable();

/*
 * Voting functions to enable/disable Axon, implemented by the host.
 */
/*
 * Returns a unique VoteId that can be used in subsequent calls to
 * AxonHostAxonEnableVote/AxonHostAxonDisableVote
 */
uint16_t AxonHostGetVoteId();

/**
 * host function for enabling axon by powering it up and providing a clock in a
 * multi-axon user system.
 * vote_id is a bit mask returned by AxonHostGetVoteId().
 * Voting for Axon enable ensures that Axon will remain powered on until no earlier
 * than voting against Axon enable.
 */
void AxonHostAxonEnableVote(uint8_t power_on_reset, uint16_t vote_id);

/**
 * host function for disabling axon by powering it down and removing the clock.
 */
void AxonHostAxonDisableVote(uint16_t vote_id);


/**
 * Host returns 1 if axon can access this address, 0 if it cannot (so will need to be copied to
 * ram before being used by Axon).
 */
uint8_t AxonHostAddressAvailableToAxon(uint32_t addr);
