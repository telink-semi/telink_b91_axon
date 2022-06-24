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
#pragma once
#include "axon_api.h"
#include "stdint.h"
#include "stdarg.h"
extern uint8_t ML_LOGGING_DISABLE_PRINT;
/*
 * This functions will print a vector of the associated type and specified length out the
 * debug port. The printed format is:
 * <type of vector_ptr> <name>[<count>]=\r\n
 * {<vector_ptr[0]>,<vector_ptr[1]>, <...>,<vector_ptr[count-1]>}\r\n
 * For example
 * int32_t my_int32_vector[10]=
 * {1,-1,3,0,7,-1000,-10,8,9,10}
 */
void print_float_vector(void *axon_handle, char *name, float *vector_ptr, uint32_t count, uint8_t stride);
void print_int32_vector(void *axon_handle, char *name, int32_t *vector_ptr, uint32_t count, uint8_t stride);
void print_int16_vector(void *axon_handle, char *name, int16_t *vector_ptr, uint32_t count, uint8_t stride);
void print_int16_circ_buffer(void *axon_handle, char *name, int16_t *vector_ptr, uint32_t count, uint8_t stride, uint32_t start_offset);
void print_int8_vector(void *axon_handle, char *name, int8_t *vector_ptr, uint32_t count);
void PrintVector(void *axon_handle, char *name, uint8_t *vector_ptr, uint32_t count, uint8_t element_size);
void axon_printf(void *axon_handle, char *fmt_string, ...);

