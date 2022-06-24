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
#include <string.h>
#include <stdio.h>

#include "axon_api.h"
#include "axon_dep.h"
#include "axon_logging_api.h"

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
   AxonHostLog(axon_instance, axon_instance->host_provided.log_buffer);
}

void print_float_vector(void *axon_handle, char *name, float *vector_ptr, uint32_t count, uint8_t stride) {
  char printbuffer[20]; // sized big enough for longest integer.
  AxonHostLog(axon_handle, "float ");;

  AxonHostLog(axon_handle, name);
  sprintf(printbuffer, "[%d] = {\r\n", count);
  AxonHostLog(axon_handle, printbuffer);
  while (count--) {
    sprintf(printbuffer, "%f,", *vector_ptr);
    vector_ptr+=stride;
    AxonHostLog(axon_handle, printbuffer);
  }
  AxonHostLog(axon_handle, "\r\n}\r\n");
}

void print_int32_vector(void *axon_handle, char *name, int32_t *vector_ptr, uint32_t count, uint8_t stride) {
  char printbuffer[20]; // sized big enough for longest integer.

  AxonHostLog(axon_handle, "int32_t ");;

  AxonHostLog(axon_handle, name);
  sprintf(printbuffer, "[%d] = {\r\n", count);
  AxonHostLog(axon_handle, printbuffer);
  while (count--) {
    sprintf(printbuffer, "%d,", *vector_ptr);
    vector_ptr+=stride;
    AxonHostLog(axon_handle, printbuffer);
  }
  AxonHostLog(axon_handle, "\r\n}\r\n");
}

void print_int16_vector(void *axon_handle, char *name, int16_t *vector_ptr, uint32_t count, uint8_t stride) {
  print_int16_circ_buffer(axon_handle, name, vector_ptr, count, stride, 0);
}
void print_int16_circ_buffer(void *axon_handle, char *name, int16_t *vector_ptr, uint32_t count, uint8_t stride, uint32_t start_index) {
  char printbuffer[20]; // sized big enough for longest integer.
  uint32_t sample_ndx;

  AxonHostLog(axon_handle, "int16_t ");
  AxonHostLog(axon_handle, name);
  sprintf(printbuffer, "[%d] = {\r\n", count);
  AxonHostLog(axon_handle, printbuffer);
  start_index *= stride;
  for (sample_ndx=0; sample_ndx<count; sample_ndx++) {
    sprintf(printbuffer, "%d,", vector_ptr[start_index]);
    AxonHostLog(axon_handle, printbuffer);
    start_index+=stride;
    if (start_index >= count * stride) {
      start_index=0;
    }
  }
  AxonHostLog(axon_handle, "\r\n}\r\n");

}

void print_int8_vector(void *axon_handle, char *name, int8_t *vector_ptr, uint32_t count) {
  char printbuffer[20]; // sized big enough for longest integer.
  AxonHostLog(axon_handle, "int8_t ");

  AxonHostLog(axon_handle, name);
  sprintf(printbuffer, "[%d] = {\r\n", count);
  AxonHostLog(axon_handle, printbuffer);
  while (count--) {
    sprintf(printbuffer, "%d,", *vector_ptr++);
    AxonHostLog(axon_handle, printbuffer);
  }
  AxonHostLog(axon_handle, "\r\n}\r\n");
}


/*
 * utility for printing a vector to the debug console.
 */
void PrintVector(void *axon_handle, char *name, uint8_t *vector_ptr, uint32_t count, uint8_t element_size) {
  char printbuffer[20]; // sized big enough for longest integer.
  int32_t element_value = 0;
  switch (element_size) {
  case 1: AxonHostLog(axon_handle, "int8_t "); break;
  case 2: AxonHostLog(axon_handle, "int16_t "); break;
  case 4: AxonHostLog(axon_handle, "int32_t "); break;
  default: return;
  }

  AxonHostLog(axon_handle, name);
  AxonHostLog(axon_handle, " = {\r\n");
  while (count--) {
    memcpy(&element_value, vector_ptr, element_size);
    sprintf(printbuffer, "%d,", element_value);
    AxonHostLog(axon_handle, printbuffer);
    vector_ptr+=element_size;
  }
  AxonHostLog(axon_handle, "\r\n}\r\n");
}
