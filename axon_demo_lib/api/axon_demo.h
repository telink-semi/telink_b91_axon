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

#include <stdint.h>
/**
 * Called one time by the host application at startup.
 * @param axon_base_addr
 *   Address of the axon IP block in the host's address space.
 *
 * @returns
 *   0 for success, or a negative error code.
 */
int AxonDemoPrepare(uint32_t* axon_base_addr, void *);

/*
 * Performs the main functionality of the demo. Invoke this after calling AxonDemoPrepare()
 * The parameters are ignored by the function.
 */
int AxonDemoRun(void *, uint8_t);

