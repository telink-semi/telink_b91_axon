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

int AxonAppRun(void *unused1, uint8_t unused2);
void axon_app_gpio_irq_handler(void);
void axon_app_gpio_risc1_irq_handler(void);
void axon_app_timer0_irq_handler(void);

/*
 * Call this upon exiting deepsleep.
 * Library uses this to inform itself that all non-retention variables
 * have been lost and need to be re-initialized.
 */
void AudioFrameworkDeepsleepWakeInit();

/*
 * Called one time, after power-on/reset.
 * Needs to prepare any axon operations (indirectly, through axon_ml_lib)
 * and do everything that DeepsleepWakeInit() does.
 */
int AudioFrameworkOneTimeInit();
