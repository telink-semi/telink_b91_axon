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
 * This file implements utilities used by the demo code.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "axon_api.h"
#include "axon_dep.h"
#include "axon_demo_private.h"


/*
 * Formats a string and prints it via AxonHostLog()
 */
void axon_printf(void *axon_handle, char *fmt_string, ...) {
  AxonInstanceStruct *axon_instance = (AxonInstanceStruct *)axon_handle;
  va_list arg_list;

   // get the variable arg list
   va_start(arg_list,fmt_string);
   // pass it to snprintf
   vsnprintf(axon_instance->host_provided.log_buffer, axon_instance->host_provided.log_buffer_size, fmt_string, arg_list);
   va_end(arg_list);

   // now print it
   AxonHostLog((AxonInstanceStruct*)axon_instance, axon_instance->host_provided.log_buffer);
}


#define my_abs(x) ((int32_t)x<0?-1*(int32_t)x:x)
/*
 * Verifies output[] == expected_output[] with each value being within margin of each other.
 */
int verify_vectors(char *msg, int32_t *output, int32_t* expected_output, uint32_t count, uint32_t margin) {
  int err_cnt = 0;
  int error_dif;

  axon_printf(gl_axon_handle, "Verify %s... ", msg);

  for (int i = 0; i < count; i++) {
    error_dif = output[i] - expected_output[i];
    if (my_abs(error_dif) > margin) {
      axon_printf(gl_axon_handle, " error @ %d: dif %d not less than %d: got 0x%.8lx, expected 0x%.8lx\r\n", i, error_dif, margin, output[i], expected_output[i]);
      err_cnt++;
    }
  }
  // return error in case of mismatch
  if (err_cnt > 0) {
    axon_printf(gl_axon_handle, " FAILED!: %d mismatches!\r\n", err_cnt);
    return (-1);
  } else {
    axon_printf(gl_axon_handle, " PASSED!\r\n");
    return (0);
  }

}

/*
 * verifies int16_t vectors
 */
int verify_vectors_16(char *msg, int16_t *output, int16_t* expected_output, uint32_t count, uint32_t margin) {
  int err_cnt = 0;
  int error_dif;

  axon_printf(gl_axon_handle, "Verify %s... ", msg);

  for (int i = 0; i < count; i++) {
    error_dif = output[i] - expected_output[i];
    if (my_abs(error_dif) > margin) {
      axon_printf(gl_axon_handle, "error @ %d: dif %d not less than %d: got 0x%.8lx, expected 0x%.8lx\r\n", i, error_dif, margin, output[i], expected_output[i]);
      err_cnt++;
    }
  }
  // return error in case of mismatch
  if (err_cnt > 0) {
    axon_printf(gl_axon_handle, " FAILED!: %d mismatches!\r\n", err_cnt);
    return (-1);
  } else {
    axon_printf(gl_axon_handle, " PASSED!\r\n");
    return (0);
  }

}

/*
 * verifies int8_t vectors
 */
int verify_vectors_8(char *msg, int8_t *output, int8_t* expected_output, uint32_t count, uint32_t margin) {
  int err_cnt = 0;
  int error_dif;

  axon_printf(gl_axon_handle, "Verify %s... ", msg);

  for (int i = 0; i < count; i++) {
    error_dif = output[i] - expected_output[i];
    if (my_abs(error_dif) > margin) {
      axon_printf(gl_axon_handle, "error @ %d: dif %d not less than %d: got 0x%.8lx, expected 0x%.8lx\r\n", i, error_dif, margin, output[i], expected_output[i]);
      err_cnt++;
    }
  }
  // return error in case of mismatch
  if (err_cnt > 0) {
    axon_printf(gl_axon_handle, "FAILED!: %d mismatches!\r\n", err_cnt);
    return (-1);
  } else {
    axon_printf(gl_axon_handle, "PASSED!\r\n");
    return (0);
  }

}

