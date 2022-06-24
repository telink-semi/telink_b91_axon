/********************************************************************************************************
 * @file  app_config.h
 *
 * @brief This is the header file for B91
 *
 * @author  Driver Group
 * @date  2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
 *******************************************************************************************************/
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
#include "app_config.h"
#include "axon_dep.h"
#include "axon_api.h"
#include <nds_intrinsic.h>
#include "printf.h"


extern void axon_app_gpio_irq_handler(void);
extern void axon_app_gpio_risc1_irq_handler(void);
extern void axon_app_timer0_irq_handler(void);

/*
 * These memory resources are given to the driver through axon_instance
 */
#define MAX_USER_OP_HANDLES 110

static char axon_log_buffer[256];

static AxonInternalBuffer _Alignas(16) axon_internal_buffers[1+MAX_USER_OP_HANDLES];

#define AXON_MATRIX_MULT_BUFFER_COUNT 16
static AxonMatrixMultBuffer axon_mm_buffers[AXON_MATRIX_MULT_BUFFER_COUNT];
static AxonAcorrBuffer axon_acorr_buffer;
/*
 * mm line buffer needs to be sized to hold at least 2 rows of the largest MM array, padded out to 16byte multiple.
 *
 * In our case, the fully connected matrix is 256 rows by 490 columns of int8. Round up to 16 and that is 496bytes X 2 = 992bytes
 * at a bare minimum.
 */
#define MAX_MM_ROW_LENGTH FC_INPUT_LENGTH // FC_INPUT_LENGTH is defined through the build system; is specific to a build configuration.
#define MM_LINEBUFFER_MIN_SIZE (2*((MAX_MM_ROW_LENGTH+15) & ~0xf))
#define MM_LINE_BUFFER_COUNT   (4)
#define MM_LINE_BUFFER_SIZE_IN_WORDS (MM_LINEBUFFER_MIN_SIZE*MM_LINE_BUFFER_COUNT/sizeof(int32_t))
static uint32_t _Alignas(16) axon_mm_line_buffer[MM_LINE_BUFFER_SIZE_IN_WORDS];

/*
 * Describe the one and only Axon instance.
 */
static AxonInstanceStruct axon_instance = {
  .host_provided = {
    .log_buffer = axon_log_buffer,
    .log_buffer_size = sizeof(axon_log_buffer),
    .internal_buffer_size = sizeof(axon_internal_buffers)/sizeof(AxonInternalBuffer),
    .matrix_mult_buffer_size = sizeof(axon_mm_buffers)/sizeof(AxonMatrixMultBuffer),
    .mm_line_buffer_size = sizeof(axon_mm_line_buffer)/sizeof(uint32_t),
    .base_address = NULL,
    .internal_buffers = axon_internal_buffers,
    .acorr_buffer = &axon_acorr_buffer,
    .matrix_mult_buffer = axon_mm_buffers,
    .mm_line_buffer = axon_mm_line_buffer,
  }
};

extern AxonInstanceStruct *gl_axon_instance;
AxonInstanceStruct *gl_axon_instance = &axon_instance;

/*
 * For basic, bare-metal, asynchronous calls, our notification routine (which executes in the interrupt context)
 * will increment this counter.
 * Prior to invoking an async driver API, the user code will examine this value,
 * then  spin/wait until it changes, indicating the call has completed.
 */
static volatile struct {
  uint32_t async_notification_count;
  uint32_t axon_power_ballot;
  uint8_t chain_axon_ops_in_isr;
  uint8_t highest_power_ballot_no;
} axon_app_state;

/**
 * Called one time by the host application at startup.
 * @param axon_base_addr
 *   Address of the axon IP block in the host's address space.
 *
 * @returns
 *   0 for success, or a negative error code.
 */
int AxonAppPrepare(void *);

/*
 * Performs the main functionality of the demo. Invoke this after calling AxonAppPrepare()
 * The parameters are ignored by the function.
 */
int AxonAppRun(void *, uint8_t);

/*
 * Note: axon driver is guaranteed to not nest disable interrupt calls;
 * there will be only 1 disable before enable is called, so a single
 * state variable is safe.
 */
uint32_t AxonHostDisableInterrupts() {

  uint32_t state = NDS_MSTATUS & 1<<3;
  clear_csr(NDS_MSTATUS,1<<3);
  // clear_csr(NDS_MIE,1<<11 | 1 << 7 | 1 << 3);
  return state;
}
void AxonHostRestoreInterrupts(uint32_t restore_value) {
  if (restore_value) {
    // set_csr(NDS_MIE,1<<11 | 1 << 7 | 1 << 3);
    set_csr(NDS_MSTATUS,1<<3);
  }
}
static void enable_interrutps() {
  set_csr(NDS_MSTATUS,1<<3);
  set_csr(NDS_MIE,1<<11 | 1 << 7 | 1 << 3);
}

void AxonHostWfi() {

  __asm__("wfi");
}


/**
 * Console logging function implemented by host.
 */
void AxonHostLog(AxonInstanceStruct *axon, char *msg) {
  (void)axon;
  printf(msg);
}

/*
 * Time-stamp function implemented by host.
 */
uint32_t AxonHostGetTime() {
  /*
  uint32_t ticks_as_32 = 0;
  ticks_as_32 = __nds__csrr(NDS_MCYCLE);
  return ticks_as_32;
  */
  return stimer_get_tick();
}

/**
 * Host returns 1 if axon can access this address, 0 if it cannot (so will need to be copied to
 * ram before being used by Axon).
 */
uint8_t AxonHostAddressAvailableToAxon(uint32_t addr) {
  return addr < (FLASH_R_BASE_ADDR);
}

/*
 * Host function to transform ILM/DLM to Axon accessible addresses.
 */
uint32_t AxonHostTransformAddress(uint32_t from_addr) {
  return from_addr >= FLASH_R_BASE_ADDR ? from_addr :
      from_addr < CPU_DLM_BASE ? from_addr-CPU_ILM_BASE+ILM_BASE :
          from_addr-CPU_DLM_BASE+DLM_BASE;
}

/*
 * Function to poll async notification count for non-queued batch mode.
 */
uint32_t AxonAppGetAsyncNotificationCount() {
  return axon_app_state.async_notification_count;
}

/*
 * Called from the axon interrupt handler when the user needs to intervene.
 */
void AxonHostInterruptNotification(AxonInstanceStruct *axon) {
  axon_app_state.async_notification_count++;

  /*
   * In queued batch mode, we will check axon progress which will invoke
   * batch callbacks when a batch completes.
   */
  if (axon_app_state.chain_axon_ops_in_isr) {
    // in queued batch mode, this will invoke the batch completed callbacks.
    AxonApiGetAsyncResult(axon);
  }

}

/*
 *
 */
void AxonAppSetChainAxonOpsInIsrEnabled(bool value) {
  axon_app_state.chain_axon_ops_in_isr = value;
}


void npe_comb_irq_handler(void) {
  core_save_nested_context();
  /*
   * Give axon driver opportunity to clear interrupt at the source
   */
  AxonHandleInterrupt(gl_axon_instance, 1);

  core_restore_nested_context();
  __nds__fence(FENCE_IORW,FENCE_IORW); //NDS_FENCE_IORW;
}

void gpio_irq_handler(void)
{
  core_save_nested_context();
  axon_app_gpio_irq_handler();
  core_restore_nested_context();
  __nds__fence(FENCE_IORW,FENCE_IORW); //NDS_FENCE_IORW;
}

void gpio_risc1_irq_handler(void)
{
  core_save_nested_context();
  axon_app_gpio_risc1_irq_handler();
  core_restore_nested_context();
  __nds__fence(FENCE_IORW,FENCE_IORW); //NDS_FENCE_IORW;
}

void timer0_irq_handler(void)
{
  core_save_nested_context();
  axon_app_timer0_irq_handler();
  core_restore_nested_context();
  __nds__fence(FENCE_IORW,FENCE_IORW); //NDS_FENCE_IORW;
}

/**
 * @brief npe comb interrupt handler.
 * @return none
 */
#if BLE_SDK
_attribute_ram_code_sec_noinline_ void  my_entry_irq61(void) __attribute__ ((interrupt ("machine") , aligned(4)));
void  my_entry_irq61(void)
{
  core_save_nested_context();
  npe_comb_irq_handler();
  core_restore_nested_context();
  plic_interrupt_complete(IRQ61_NPE_COMB);
  __nds__fence(FENCE_IORW,FENCE_IORW); //NDS_FENCE_IORW;
}
#endif

/**
 * enables the axon insterrupt specifically
 */
void AxonHostEnableAxonInterrupt() {
  plic_interrupt_enable(IRQ61_NPE_COMB);
}
/**
 * disables the axon insterrupt specifically
 */
void AxonHostDisableAxonInterrupt() {
  plic_interrupt_disable(IRQ61_NPE_COMB);
}

/*
 * Function to enable NPE clock and power
 */
void AxonHostAxonEnable(uint8_t power_on_reset){
  //power up the NPE
  analog_write_reg8(0x7d,(analog_read_reg8(0x7d) & (~BIT(2))));

  //enable the NPE reset register
  reg_rst2 |= (FLD_RST2_NPE);
  //enable the NPE clock
  reg_clk_en2 |= (FLD_CLK2_NPE_EN);

  // vote against low power when axon enabled
  //PowerMgrVoteForLowPowerState(POWER_MGR_AXON_DRIVER_VOTER_ID, kLowPowerNone, 0);
  plic_interrupt_enable(IRQ61_NPE_COMB);

  delay_us(5);

  if (power_on_reset) {
    // 1st time through, init the driver.
    axon_instance.host_provided.base_address = (uint32_t*)(REG_RW_BASE_ADDR+NPE_BASE_ADDR);
    AxonInitInstance(&axon_instance);
  } else {
    AxonReInitInstance(gl_axon_instance);
  }
}

/*
 * Function to disable NPE clock and power
 */
void AxonHostAxonDisable(){
  //disable the NPE clock
  reg_clk_en2 &= (~FLD_CLK2_NPE_EN);
  //disable the NPE reset register
  reg_rst2 &= (~FLD_RST2_NPE);

  //power gate the NPE
  analog_write_reg8(0x7d,(analog_read_reg8(0x7d) | BIT(2)));

  plic_interrupt_disable(IRQ61_NPE_COMB);
}
/*
 * NOT THREAD-SAFE! SHOULD ONLY BE CALLED DURING START-UP SEQUENCE!
 */
uint16_t AxonHostGetVoteId() {
  return (1<<axon_app_state.highest_power_ballot_no++);
}

/*
 * Function to enable NPE clock and power
 *
 * Note that this implementation is not thread-safe. However, this application is inherently safe
 * because 1)Axon is only accessed in the interrupt context and 2) interrupts are not pre-emptible except by
 * the BLE stack which does not access Axon.
 */
void AxonHostAxonEnableVote(uint8_t power_on_reset, uint16_t voter_id){

  if (axon_app_state.axon_power_ballot) {
    // already enabled, nothing to do except record this vote..
    axon_app_state.axon_power_ballot |= voter_id;
    return;
  }
  axon_app_state.axon_power_ballot |= voter_id;
  AxonHostAxonEnable(power_on_reset);
}

void AxonHostAxonDisableVote(uint16_t voter_id){

  if (0==(axon_app_state.axon_power_ballot &= ~voter_id)) {
    // last vote is cleared, time to disable it.
    AxonHostAxonDisable();
  }
}

/*
 * function to shutdown the GPIOs
 */
void shutdown_gpio(){

#if 0 // fixme! what is the purpose of this?
  //shutdown all the GPIO PORTA and PORTD
  gpio_shutdown(GPIO_ALL);
  //disabling the SWS pin //on PORTA[7]
  reg_gpio_pa_ie = 0x00;
  gpio_shutdown(GPIO_PE0);
  gpio_shutdown(GPIO_PE1);
  gpio_shutdown(GPIO_PE2);
  //init the UART pin
  gpio_function_en(GPIO_PA0);
  gpio_set_up_down_res(GPIO_PA0, GPIO_PIN_PULLUP_1M);
  gpio_set_output_en(GPIO_PA0);
  //init the LEDs
  gpio_function_en(LED1 | LED2 | LED3 | LED4);
  gpio_set_output_en(LED1 | LED2 | LED3 | LED4);    //enable output
  gpio_set_input_dis(LED1 | LED2 | LED3 | LED4);      //disable input
  // turn LEDs ON
  gpio_set_high_level(LED3 | LED4);
#endif
}

void AxonHostSetProfilingGpio(uint8_t level) {
  gpio_set_level(GPIO_LED_BLUE, level);
}
void bsp_set_profiling_gpio(uint8_t high_or_low) {
  AxonHostSetProfilingGpio(high_or_low);
}

void bsp_power_up_mic() {
  // nothing to do on EVB
}
void bsp_power_down_mic() {
  // nothing to do on EVB
}

void user_init()
{

  gpio_function_en(GPIO_LED_WHITE | GPIO_LED_GREEN | GPIO_LED_RED | GPIO_LED_BLUE);
  gpio_set_output(GPIO_LED_WHITE | GPIO_LED_GREEN | GPIO_LED_RED | GPIO_LED_BLUE, 1);    //enable output
  gpio_set_input(GPIO_LED_WHITE | GPIO_LED_GREEN | GPIO_LED_RED | GPIO_LED_BLUE, 0);      //disable input
  // turn LEDs ON
  gpio_set_high_level(GPIO_LED_WHITE | GPIO_LED_GREEN | GPIO_LED_RED | GPIO_LED_BLUE);

  memset((void *)&axon_app_state, 0, sizeof(axon_app_state));
  axon_app_state.chain_axon_ops_in_isr = 1; // default to direct support for queued batches

#if(DEBUG_BUS==USB_PRINT_DEBUG_ENABLE)
  usb_set_pin_en();
  reg_usb_ep8_send_thre = 1;
  reg_usb_ep8_fifo_mode = 1;
  delay_ms(1000);
  delay_ms(3000);
#elif(DEBUG_BUS==UART_PRINT_DEBUG_ENABLE)
#endif
  AxonHostAxonEnable(1);
  int axon_result;
  if (kAxonResultSuccess > (axon_result=AxonAppPrepare(NULL))) {
    printf("AxonAppPrepare failed! %d\r\n", axon_result);

    while(1);
  }

  enable_interrutps();
  plic_interrupt_enable(IRQ61_NPE_COMB);
}
void main_loop (void)
{
  // turn off blue & green
  gpio_set_low_level(GPIO_LED_GREEN | GPIO_LED_BLUE);


#if AXO_APP_WAIT_FOR_KEY_PRESS
  // TL Key1 is low input
  gpio_function_en(GPIO_PC2);
  gpio_set_input(GPIO_PC2, 1);
  gpio_set_output(GPIO_PC2, 0);
  gpio_set_up_down_res(GPIO_PC2, GPIO_PIN_PULLDOWN_100K);
  gpio_set_low_level(GPIO_PC2);

  // TL Key3 is high output
  gpio_function_en(GPIO_PC3);
  gpio_set_output(GPIO_PC3, 1);
  gpio_set_input(GPIO_PC3, 0);
  gpio_set_up_down_res(GPIO_PC3, GPIO_PIN_PULLUP_10K);
  gpio_set_high_level(GPIO_PC3);

  // poll until Key1 is high, indicating button press
  while(0==gpio_get_level(GPIO_PC2));
#endif

  gpio_set_high_level(GPIO_LED_RED);

  printf("AxonAppRun\r\n");
  AxonAppRun(NULL, 0);

  while(1) {
    delay_ms(300);
    gpio_toggle(GPIO_LED_RED | GPIO_LED_WHITE );
  }

}

void bsp_get_spo2_abc(float *a, float *b, float *c) {
  // returns a, b, &c constants for spo2 calculation
  *a = 2.698;
  *b = -36.583;
  *c = 118.346;
}
