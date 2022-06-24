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

#include "axon_audio_framework.h"
#include "axon_audio_ml_api.h"
#include "axon_dep.h"
#include "axon_api.h"
#include <assert.h>
#include <string.h>

/*
 * Audio record ti
 */
#if (TRIGGER_MODE_ALWAYS_ON)
#  define TRIGGER_MODE_BUTTON_PRESS 0
#else
#  define TRIGGER_MODE_BUTTON_PRESS 1
#endif


#if TRIGGER_MODE_ALWAYS_ON
#  define AUDIO_SNIFF_FRAME_CNT  (3)  // check for sound energy on up to the 3rd audio frame in the sniff period
#  define AUDIO_OFF_TIME_MS    (400-16*AUDIO_SNIFF_FRAME_CNT)  // disable audio for 400ms between sniffs.
#  define AUDIO_SKIP_FRAME_CNT (1)

/*
 * ALWAYS_ON_ONESHOT means that always on audio sniffing stops after detecting noise (and recording the window).
 * It restarts after SW2 is pressed and released.
 * This allows for playing-back the audio that was recorded in one-shot mode w/o the audio sniffing corrupting/interfering with it.
 */
# define ALWAYS_ON_ONESHOT 0
#else
#  define AUDIO_SKIP_FRAME_CNT (0)
#endif

extern void bsp_set_profiling_gpio(uint8_t high_or_low);
extern void   bsp_power_up_mic();
extern void   bsp_power_down_mic();


#ifdef BLE_SDK
/*
 * using the ml_ble_platform
 */
#  include "power_mgr_api.h"
RETAINED_MEMORY_SECTION_ATTRIBUTE
static struct {
  PowerMgrVoterIdEnum power_voter_id;
}audio_framework_retained_state;
#else
/*
 * linking in to core_drivers
 */
#  define PowerMgrVoteForLowPowerState(A,B, C)
#endif

void audio_framework_print_usage() {
  AxonHostLog(NULL, "\r\n\r\n******* Axon Machine Learning Demo - Key Word Spotting*******\r\n\r\n");
  AxonHostLog(NULL, "Press and release SW2 to begin audio recording.\r\n");
  AxonHostLog(NULL, "The red LED will light when SW2 is down, and the blue LED will light when it is released, indicating that recording is in progress.\r\n");
  AxonHostLog(NULL, "Recording will occur for 2 seconds or until a word is detected.\r\n");
  AxonHostLog(NULL, "Speak one of the following key words: UP, DOWN, LEFT, RIGHT, STOP, GO, YES, NO, ON, OFF.\r\n");
  AxonHostLog(NULL, "If a word is detected within the 2 seconds, the green LED will light indicating classification is occurring.\r\n");
  AxonHostLog(NULL, "The classification is then printed.\r\n");
#if CAPTURE_AUDIO_PLAYBACK
  AxonHostLog(NULL, "To both playback the just-recorded audio, and to dump the audio sample values to the console, press SW5.\r\n");
  AxonHostLog(NULL, "The audio will playback in a loop until a new recording is started.\r\n");
  AxonHostLog(NULL, "*********************************************************\r\n\r\n");
#endif
}


#define KWS_ENABLED 1

#define AMIC 0
#define DMIC 1
#define AUDIO_MIC AMIC
/*
 * (super) simple state machine states.
 */
typedef enum {
  kIdle,
  kWaitingForTrigger, // in button mode, waiting for key to return up, in always-on mode, polling audio for energy threshold trigger.
  kTriggered,         // actively recording for the purpose of inference
  kRecordingStopped,   // Clean up and return to kWaitingForTrigger state
} LiveKwsDemoStateEnum;


/*
 * state variables. keep them all in one place.
 * Note that this state variable is not in retained memory.
 * Every audio cycle (push/release button, record audio, perform inference) will
 * be done w/ deep sleep disabled.
 */
#if TRIGGER_MODE==TRIGGER_ALWAYS_ON
  // need to retain this in always-on mode
#endif
static volatile struct {
  LiveKwsDemoStateEnum current_state;
  bool next_frame_is_1st_frame;
  uint32_t state_entry_time; // time-stamp of start key press
  uint32_t audio_start_time;    // time-stamp of audio start (start_key_down_time+debounce_delay)
  uint32_t audio_frame_number;  // current audio frame number.
  uint32_t end_key_up_time;     // time-stamp of audio end (ie, button release)
  volatile uint32_t sw5_down_event_cnt;    // isr increments this every time it sees a rising edge
  uint32_t last_processed_sw5_down_cnt; // delta between this and  sw2_up_event_cnt is the 3 of events that have occurred since last processed
  int16_t *last_frame;
  uint32_t process_state_count;
  int32_t kws_classification;  // classification index of last kws inference. negative number is an invalid index.
  char log_buffer[200];
#if (TRIGGER_MODE_ALWAYS_ON)
  uint8_t total_foregrounds_in_window;
  uint8_t consecutive_backgrounds;
#endif
#ifdef BLE_SDK
  power_mgr_alarm_struct alarm_info;
#endif
} live_kws_state_info_struct;

//fwd declarations
//static void timer_stop(timer_type_e type);
static void transition_state(LiveKwsDemoStateEnum new_state, const char *msg);
static void button_init();
static void audio_record_init(void);
static void AxonHostAudioEn();
static void AxonHostAudioDis();
static void audio_timer_start();


/***************************************************
 * Axon Host integration functions.
 ***************************************************/

void AxonHostEnableInterrupts(){
  AxonHostRestoreInterrupts(1);
}

/*
 * Function to disable Audio clocks when needed
 */
static void AxonHostAudioDis(){

  bsp_power_down_mic();
  //disable the audio clock
  reg_clk_en2 &= (~FLD_CLK2_AUD_EN);
  //disable the dma clock
  /*
   * DON'T DISABLE DMA CLOCK; OTHERS MIGHT BE USING IT!!!
   */
#if 0
  reg_clk_en1 &= (~FLD_CLK1_DMA_EN);
#endif
  //disable the audio
  reg_rst2 &= (~FLD_RST2_AUD);
  //disable the dma
  /*
   * DON'T DISABLE DMA CLOCK; OTHERS MIGHT BE USING IT!!!
   */
#if 0
  reg_rst1 &= (~FLD_RST1_DMA);
#endif
  //disable the codec
  reg_rst3 &= (~BIT(5));//codec is at bit 5

#if AUDIO_MIC==DMIC
  gpio_set_gpio_en(GPIO_PB2 | GPIO_PB3 | GPIO_PB4);
  gpio_set_output_en(GPIO_PB2 | GPIO_PB3 | GPIO_PB4);
  gpio_set_low_level(GPIO_PB2 | GPIO_PB3 | GPIO_PB4);
  gpio_set_input_dis(GPIO_PB2 | GPIO_PB3 | GPIO_PB4);
#endif

  // ok to deepsleep.
  PowerMgrVoteForLowPowerState(audio_framework_retained_state.power_voter_id,kLowPowerDeepsleepRetention, 1);
}

/*
 * Function to enable Audio clocks when needed
 */
static void AxonHostAudioEn(){
  bsp_power_up_mic();
  //enable the codec
  reg_rst3 |= (BIT(5));//codec is at bit 5
  //enable the dma
  reg_rst1 |= (FLD_RST1_DMA);
  //enable the audio
  reg_rst2 |= (FLD_RST2_AUD);

  //enable the dma clock
  reg_clk_en1 |= (FLD_CLK1_DMA_EN);
  //enable the audio clock
  reg_clk_en2 |= (FLD_CLK2_AUD_EN);

  /*
   * vote against sleep to keep audio going.
   */
  PowerMgrVoteForLowPowerState(audio_framework_retained_state.power_voter_id,kLowPowerNone, 1);

}

/*
 * Called by axon_audio_ml_main to optimized power by disabling axon whenever it
 * is not in use.
 * However, we are relying on the axon driver vote to prevent deepsleep (enable votes against
 * deepsleep, disable votes for it) so this function won't do anything. Instead,
 * we'll enable/disable axon at the start/end of an inference session. Axon power
 * is pretty small when compared to the audio subsystem, and this is a low duty cycle
 * use-case anyway.
 */
void AxonMlDemoHostAxonSetEnabled(AxonBoolEnum enabled){
}

static void timer_really_stop() {
  // stop the polling timer.
#ifdef BLE_SDK
  // de-register this callback from power mgr
  PowerMgrDeleteAlarm(&live_kws_state_info_struct.alarm_info);
#else
  // for core_drivers platform, we control timer0 directly
  timer_stop(TIMER0);
  // make sure the interrupt doesn't fire in case it is pending
  reg_tmr_sta = FLD_TMR_STA_TMR0;
  plic_interrupt_complete(IRQ4_TIMER0);
#endif
}
#if TRIGGER_MODE_ALWAYS_ON

/*
 * configures the timer to go off for the next audio sniff
 *
 * dont_start_the_sniff is for always-on one-shot mode, where the sniff is not re-enabled
 * after a window has been recorded (and classified). This enables playback of the captured window.
 */
static void audio_sniff_timer_init(uint8_t dont_start_the_sniff){
  timer_really_stop();
  if (!dont_start_the_sniff) {
    timer_set_init_tick(TIMER0, 0);
    timer_set_cap_tick(TIMER0, AUDIO_OFF_TIME_MS*sys_clk.pclk*1000);
    timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);
    plic_interrupt_enable(IRQ4_TIMER0);
    timer_start(TIMER0);
    live_kws_state_info_struct.total_foregrounds_in_window = 0;
    live_kws_state_info_struct.consecutive_backgrounds = 0;
  }
  live_kws_state_info_struct.current_state = kIdle;
}
#endif

/*
 * enables axon and audio hardware as well as some GPIOs.
 */
static void enable_audio_and_axon(){
  AxonHostAudioEn();

  //enable Axon clocks and peripherals. This will vote against deepsleep.
  /*
   * @FIXME! NEED TO GET A REAL VOTE IN ORDER TO BE COMPATIBLE W/ OTHER AXON USERS!
   */
  AxonHostAxonEnableVote(0,0);
#ifndef BLE_SDK
  gpio_function_en( LED2 | LED3 | LED4);   // enable GPIO LEDs
  gpio_output_en( LED2 | LED3 | LED4); // enable output
  gpio_input_dis( LED2 | LED3 | LED4); // disable input
  gpio_set_low_level( LED2 | LED3 | LED4); // turn LEDs OFF
#endif

#if AUDIO_MIC==DMIC
  gpio_set_input_en(GPIO_PB2 | GPIO_PB3 | GPIO_PB4);
#endif
}
/************************************************
 * Audio functions
 ************************************************/
/*
 * everything is predicated on 16bit audio...
 */
static_assert(AUDIO_SAMPLE_BIT_WIDTH==16,"ONLY 16BIT AUDIO IS SUPPORTED IN THIS DEMO\r\n");

#if CAPTURE_AUDIO_PLAYBACK
/*
 * @FIXME!!! SETTING PLAYBACK_BUFFER_SIZE TO 2s * 16kfps * 2bytes/sample was causing a hard fault in the button press ISR
 * of all places (???!!!)
 * This macro is only used to configure DMA, so perhaps DMA has a size limit that is under 64000?
 */
#define PLAYBACK_BUFFER_SIZE (PLAYBACK_BUFFER_LEN * AUDIO_SAMPLE_BIT_WIDTH/8)

# if AUDIO_LOGGING == 0
#   define AUDIO_LOGGING 1
# endif
/*
   * playback buffer populated by audio ping pongs. Needs 4byte alignment but that is achieved
   * by following a 4byte field.
   */
__attribute__((section(".ram_code")))
int16_t audio_playback_buffer[PLAYBACK_BUFFER_LEN];
#endif

static struct {
  /*
   * ping_count/pong_count track the # of times each buffer has been filled.
   * When ping_count>0 and ping_count==pong_count,
   * buffers are ready to be used in ping/pong order. Need to consume ping before it gets refilled.
   * When ping_count > 0 and ping_count > pong_count the buffers are ready to be used in pong/ping order.
   */
  uint32_t ping_count; // number of times ping has been filled.
  uint32_t pong_count; // number of times pong has been filled.
  /*
   * circular buffer populated by audio DMA. Needs 4byte alignment but that is achieved
   * by following a 4byte field.
   */
  int16_t audio_circle_buffer[RECORD_FRAME_LEN];
  /*
   * "ping" buffer, lower half of the audio buffer
   */
  int16_t ping_buffer[RECORD_HALF_FRAME_LEN];
  /*
   * "pong" buffer, upper half of the audio buffer
   */
  int16_t pong_buffer[RECORD_HALF_FRAME_LEN];

  /*
   * not sure if this needs to be retained but we'll declare non-local just in case.
   */
  dma_chain_config_t rx_dma_list_config;
  dma_chain_config_t tx_dma_list_config;
#if AUDIO_LOGGING > 0
  char log_buffer[80];
#endif
#if CAPTURE_AUDIO_PLAYBACK
  uint32_t playback_wrap; // # of times playback buffer has wrapped
  uint32_t playback_offset; // playback is a circular buffer. This is the location of the next write.
#endif
} audio_state_info;

/*
 * Call this once at start up to configure audio and/or dma
 */
static void audio_record_init(void) {
  audio_rx_dma_config(AUDIO_RX_DMA_CH,(uint16_t*)&audio_state_info.audio_circle_buffer[0], RECORD_FRAME_SIZE, &audio_state_info.rx_dma_list_config);
  audio_rx_dma_add_list_element(&audio_state_info.rx_dma_list_config, &audio_state_info.rx_dma_list_config,(uint16_t*)&audio_state_info.audio_circle_buffer[0], RECORD_FRAME_SIZE);
  dma_chn_dis(AUDIO_RX_DMA_CH);

  /*
   * microphone needs some help. don't think D_GAIN is used by AMIC but it doesn't seem
   * to harm anything.
   */
  audio_set_codec_in_path_a_d_gain(CODEC_IN_D_GAIN_8_DB, CODEC_IN_A_GAIN_16_DB);
#if AUDIO_MIC==AMIC
//  audio_init(AMIC_IN_OUT,AUDIO_16K,MONO_BIT_16); // initialize audio AMIC with OUT
  // audio_init(AMIC_INPUT,AUDIO_16K,MONO_BIT_16); // initialize audio AMIC

 audio_init(AMIC_IN_TO_BUF,AUDIO_16K,MONO_BIT_16); // initialize audio AMIC
#elif AUDIO_MIC==DMIC
  audio_set_dmic_pin(DMIC_GROUPB_B2_DAT_B3_B4_CLK);
  audio_init(DMIC_IN,AUDIO_16K,MONO_BIT_16); // initialize audio using DMIC
#endif
}

static void audio_playback_init(void) {
#if CAPTURE_AUDIO_PLAYBACK
  AxonHostAudioDis();
  AxonHostAudioEn();
  audio_tx_dma_config(AUDIO_TX_DMA_CH,(uint16_t*)&audio_playback_buffer[0], PLAYBACK_BUFFER_SIZE, &audio_state_info.tx_dma_list_config);
  audio_tx_dma_add_list_element(&audio_state_info.tx_dma_list_config, &audio_state_info.tx_dma_list_config,(uint16_t*)&audio_playback_buffer[0], PLAYBACK_BUFFER_SIZE);
  dma_chn_dis(AUDIO_TX_DMA_CH);
  audio_init(BUF_TO_LINE_OUT,AUDIO_16K,MONO_BIT_16); // initialize audio AMIC
#endif

}
/*
 * call this to start audio recording
 */
static void audio_record_start() {
  /*
   * zero out our ping/pong counts to start over
   */
  audio_state_info.ping_count = 0;
  audio_state_info.pong_count = 0;
  /*
   * ...and this starts microphone recording
   */
  audio_record_init();   // initialize audio circle buffer
  audio_rx_dma_en();

#if CAPTURE_AUDIO_PLAYBACK
  // 0 out write buffer and offset
  audio_state_info.playback_offset = 0; // playback is a circular buffer. This is the location of the next write.
  audio_state_info.playback_wrap = 0;
  memset(audio_playback_buffer, 0, sizeof(audio_playback_buffer));
#endif
#ifndef BLE_SDK
  gpio_set_high_level(LED1); // blue on
#endif
}

/*
 * call this to stop audio recording
 */
static void audio_record_stop() {
  timer_really_stop();
  dma_chn_dis(AUDIO_RX_DMA_CH);
#ifndef BLE_SDK
  gpio_set_low_level(LED1); // blue off
#endif
}

/*
 * call this to start audio recording
 */
static void audio_playback_start() {
#if CAPTURE_AUDIO_PLAYBACK
  audio_playback_init();
  /*
   * this starts playback
   */
  audio_tx_dma_en();
#endif
}

/*
 * call this to stop audio recording
 */
static void audio_playback_stop() {
#if CAPTURE_AUDIO_PLAYBACK
  dma_chn_dis(AUDIO_TX_DMA_CH);
#endif
}

extern void print_int16_circ_buffer(void *axon_handle, char *name, int16_t *vector_ptr, uint32_t count, uint8_t stride, uint32_t start_index);

static void log_audio_playback(void){
#if CAPTURE_AUDIO_PLAYBACK
  snprintf(audio_state_info.log_buffer, sizeof(audio_state_info.log_buffer), "playback offset %d, len %d, wrap count %d\r\n",
      audio_state_info.playback_offset, PLAYBACK_BUFFER_LEN, audio_state_info.playback_wrap );
  printf(audio_state_info.log_buffer);
  print_int16_circ_buffer(NULL, "PLAYBACK_BUFFER", audio_playback_buffer,
      audio_state_info.playback_wrap == 0 ? audio_state_info.playback_offset : PLAYBACK_BUFFER_LEN,
      1,
      audio_state_info.playback_wrap == 0 ? 0 : audio_state_info.playback_offset);
#endif
}

#if CAPTURE_AUDIO_PLAYBACK
static void copy_to_playback_buffer(int16_t *from_buf, uint32_t sample_count) {
  while(sample_count--) {
    audio_playback_buffer[audio_state_info.playback_offset++] = *from_buf;
    from_buf++;
    if (audio_state_info.playback_offset >= PLAYBACK_BUFFER_LEN) {
      audio_state_info.playback_wrap++;
      audio_state_info.playback_offset = 0;
    }

  }
}
#endif

/*
 * monitors when 1st or 2nd half of circular buffer has
 * filled, and transfers contents to ping or pong buffer, respectively when
 * they have.
 *
 * Returns the just filled buffer or NULL if no buffer has filled.
 */
static int16_t *audio_buffer_monitoring(void)
{
  /*
   * be careful of logging in real-time systems. This logging could
   * cause us to lose audio!
   */
  uint32_t next_audio_sample_ptr = audio_get_rx_dma_wptr(AUDIO_RX_DMA_CH);
  uint32_t next_audio_sample_ndx = (next_audio_sample_ptr - (uint32_t)audio_state_info.audio_circle_buffer)>>1;
#if AUDIO_LOGGING > 1
  snprintf(audio_state_info.log_buffer, sizeof(audio_state_info.log_buffer), "next audio:0x%x, offset %d\r\n",next_audio_sample_ptr,next_audio_sample_ndx);
  printf(audio_state_info.log_buffer);
#endif
  /*
   * We know "ping" is ready if the audio cursor is in the "pong" sectiont of the buffer,
   * and ping_count == pong_count.
   * Similarly, "pong" is ready if the audio cursor in in the "ping" section of the buffer,
   * and ping_count > pong_count
   */
  if ((RECORD_HALF_FRAME_LEN) <= next_audio_sample_ndx) {
    // cursor is in the 2nd half (pong section) of the buffer.
    if (audio_state_info.ping_count == audio_state_info.pong_count) {
      // ping is ready. copy it over and return
      audio_state_info.ping_count++;
      memcpy(audio_state_info.ping_buffer, audio_state_info.audio_circle_buffer, RECORD_HALF_FRAME_SIZE);
#if CAPTURE_AUDIO_PLAYBACK
      copy_to_playback_buffer(audio_state_info.ping_buffer, RECORD_HALF_FRAME_LEN);
#endif
      return audio_state_info.ping_buffer;
    }
  } else {
    // cursor is in the 1st half (ping section) of the buffer.
    if (audio_state_info.ping_count > audio_state_info.pong_count) {
      // pong is ready. copy it over and return
      audio_state_info.pong_count++;
      memcpy(audio_state_info.pong_buffer, &audio_state_info.audio_circle_buffer[RECORD_HALF_FRAME_LEN], RECORD_HALF_FRAME_SIZE);
#if CAPTURE_AUDIO_PLAYBACK
      copy_to_playback_buffer(audio_state_info.pong_buffer, RECORD_HALF_FRAME_LEN);
#endif
      return audio_state_info.pong_buffer;
    }
  }
  return NULL;
}

#ifndef BLE_SDK
static uint8_t get_sw2_state() {
  return gpio_get_level(GPIO_PC2);
}

static uint8_t get_sw5_state() {
  return gpio_get_level(GPIO_PC0);
}

static void button_init()
{

  // TL Key1 is low input
  gpio_function_en(GPIO_PC2);
  gpio_input_en(GPIO_PC2);
  gpio_output_dis(GPIO_PC2);
  gpio_set_up_down_res(GPIO_PC2, GPIO_PIN_PULLDOWN_100K);
  gpio_set_low_level(GPIO_PC2);

  // TL Key3 is high output
  gpio_function_en(GPIO_PC3);
  gpio_output_en(GPIO_PC3);
  gpio_input_dis(GPIO_PC3);
  gpio_set_up_down_res(GPIO_PC3, GPIO_PIN_PULLUP_10K);
  gpio_set_high_level(GPIO_PC3);

  //core_enable_interrupt(); //enabled in user_init of main
  gpio_irq_en(GPIO_PC2);
  gpio_set_irq(GPIO_PC2, INTR_RISING_EDGE);
  // clear any spurious int..
  reg_gpio_irq_clr =FLD_GPIO_IRQ_CLR ;
  plic_interrupt_complete(IRQ25_GPIO);
  plic_interrupt_enable(IRQ25_GPIO); // GPIO_PC2

  /****GPIO_IRQ POL_RISING   SW5 link PC0**/
  gpio_function_en(GPIO_PC0);
  gpio_input_en(GPIO_PC0);
  gpio_output_dis(GPIO_PC0);
  gpio_set_up_down_res(GPIO_PC0, GPIO_PIN_PULLDOWN_100K);
  gpio_set_low_level(GPIO_PC0);

  gpio_function_en(GPIO_PC1);
  gpio_output_en(GPIO_PC1);
  gpio_input_dis(GPIO_PC1);
  gpio_set_up_down_res(GPIO_PC1, GPIO_PIN_PULLUP_10K);
  gpio_set_high_level(GPIO_PC1);

  //core_enable_interrupt();
  gpio_gpio2risc1_irq_en(GPIO_PC0);
  gpio_set_gpio2risc1_irq(GPIO_PC0,INTR_FALLING_EDGE);
  plic_interrupt_enable(IRQ27_GPIO2RISC1); // GPIO_PC0

  pm_set_gpio_wakeup(GPIO_PC2, WAKEUP_LEVEL_HIGH, 1);
  pm_set_gpio_wakeup(GPIO_PC0, WAKEUP_LEVEL_HIGH, 1);

}
#endif


/******************************************************************
 * application state machine functions
 ******************************************************************/

/*
 * changes state value, records time,
 * and (optionally) logs
 */
static void transition_state(LiveKwsDemoStateEnum new_state, const char *msg) {
  uint32_t current_time = AxonHostGetTime();

#if (AXON_LOGGING > 1)
  if (NULL != msg) {
    AxonHostLog(NULL, msg);
  }
#endif

  live_kws_state_info_struct.state_entry_time = current_time;
  live_kws_state_info_struct.current_state = new_state;
}

/*
 * The start of the window is ready. we need to harvest the audio features
 * here/now before they get consumed or lost.
 * Then we can set a signal and kick off the speaker ID algo with these features.
 */
void AxonMlDemoHostStartWindowReady(uint32_t start_frame_no, uint32_t frame_cnt) {
  int8_t mfcc_buffer[32];
  int8_t result;
  int8_t frame_ndx = 0;
#if AXON_LOGGING > 1
  snprintf(live_kws_state_info_struct.log_buffer, sizeof(live_kws_state_info_struct.log_buffer),
      "Start Frame ready @%d, len=%d\r\n\r\n",
      start_frame_no, frame_cnt );
  AxonHostLog(NULL, live_kws_state_info_struct.log_buffer);
#endif
  /*
   * ??? shouldn't happen
   */
  if (frame_cnt==0) {
    return;
  }

}

/*
 * status functions for host to indicate that classification is occurring
 */
void AxonMlDemoHostClassifyingStart(uint32_t start_frame_no, uint32_t frame_cnt) {
#ifndef BLE_SDK
  gpio_set_high_level(LED2); // green on
#endif
  audio_record_stop(); // done w/ audio!
  //disable audio
  AxonHostAudioDis();
  transition_state(kRecordingStopped, "Classifying...\r\n");
  // AxonPrintf("Classifying...\r\n");

}

/*
 * status functions for host to indicate that classification has completed
 */
void AxonMlDemoHostClassifyingEnd(uint32_t classification_number) {

#ifndef BLE_SDK
  gpio_set_low_level(LED2); // green off
#endif

  const char *label;
  // print and clear the last result
  live_kws_state_info_struct.kws_classification = AxonKwsClearLastResult(&label);
  live_kws_state_info_struct.next_frame_is_1st_frame = 1;

  AxonPrintf("Classification index: %d, %s\r\n", live_kws_state_info_struct.kws_classification, label);



  //disable Axon since classification is done
  /*
   * @FIXME! NEED TO GET A REAL VOTE IN ORDER TO BE COMPATIBLE W/ OTHER AXON USERS!
   */
  AxonHostAxonDisableVote(0);
#if TRIGGER_MODE_ALWAYS_ON
  audio_sniff_timer_init(ALWAYS_ON_ONESHOT);
#else
  live_kws_state_info_struct.current_state = kIdle;
#endif
}

/*
 * callback for when ml does not perform classification on the last frame.
 */
void AxonMlDemoHostNoClassification() {
  //disable Axon since classification is done
  /*
   * @FIXME! NEED TO GET A REAL VOTE IN ORDER TO BE COMPATIBLE W/ OTHER AXON USERS!
   */
  AxonHostAxonDisableVote(0);
#if TRIGGER_MODE_ALWAYS_ON
  audio_sniff_timer_init(ALWAYS_ON_ONESHOT);
#else
  live_kws_state_info_struct.current_state = kIdle;
#endif
  // clear out the previous result
  AxonKwsClearLastResult(NULL);
  AxonPrintf("No Classification occurred\r\n");
}

static void start_recording() {
  // AxonPrintf("Recording...\r\n");

  enable_audio_and_axon();
  audio_playback_stop();

  audio_record_start();

  /*
   * start the frames over at 0.
   */
  live_kws_state_info_struct.audio_frame_number = 0;
  /*
   * clear out our ping/pong frames
   */
  live_kws_state_info_struct.last_frame = NULL;
  /*
   * start the polling timer
   */
  audio_timer_start();
}
/*
 * This function processes the state machine.
 * It gets invoked continuously.
 */
static void process_state(){

  live_kws_state_info_struct.process_state_count++;


  switch(live_kws_state_info_struct.current_state) {

  case kIdle:
    if (live_kws_state_info_struct.sw5_down_event_cnt != live_kws_state_info_struct.last_processed_sw5_down_cnt){
      audio_playback_start();
      log_audio_playback();
      // the audio logging is plenty of time to debounce the key.
      live_kws_state_info_struct.last_processed_sw5_down_cnt = live_kws_state_info_struct.sw5_down_event_cnt;
    }
    break;

#if !TRIGGER_MODE_ALWAYS_ON
    case kWaitingForTrigger:
    //switch to PLL clock to run Axon and Audio (and vote against deep sleep)
    enable_audio_and_axon();

    audio_playback_stop();

#ifndef BLE_SDK
    gpio_set_high_level(LED4); // red while waiting for button to be released
    delay_ms(10); // debounce timer...
    while (0!=get_sw2_state()); // wait for it to rise
    gpio_set_low_level(LED4); // turn red off (not waiting for release)
#endif
    start_recording();
    transition_state(kTriggered, "Recording started\r\n");
    break;
#endif



  case kRecordingStopped:
    // clear out any button events
    live_kws_state_info_struct.last_processed_sw5_down_cnt = live_kws_state_info_struct.sw5_down_event_cnt;
    transition_state(kIdle, "Press and release SW2 to record another key word.\r\n");
    break;

  }

}

#if CAPTURE_AUDIO_PLAYBACK
extern int32_t wave_data_length;
extern const int16_t *wave_data_playback;
void CopyAudio() {
  // copies audio from an audio sample file to the audio tx buffer
  copy_to_playback_buffer(wave_data_playback, wave_data_length);
}
#endif




static void audio_framework_handle_timer() {
  if (live_kws_state_info_struct.current_state==kIdle) {
#if TRIGGER_MODE_ALWAYS_ON
    // start a trigger polling cycle.
    start_recording();
    live_kws_state_info_struct.current_state = kWaitingForTrigger;
#endif
    return;
  }

  bsp_set_profiling_gpio(1);

  // check to see if a valid audio frame has been captured.
  int16_t *current_frame = audio_buffer_monitoring();
  if(current_frame == NULL) {
    bsp_set_profiling_gpio(0);
    return; // no new audio to process.
  }

  live_kws_state_info_struct.audio_frame_number++;
  // AxonPrintf("frame no %d...", live_kws_state_info_struct.audio_frame_number);

  if (live_kws_state_info_struct.audio_frame_number<(AUDIO_SKIP_FRAME_CNT+2)) {
    // need 2 half-frames to do anything.
    live_kws_state_info_struct.last_frame = current_frame;
    bsp_set_profiling_gpio(0);

    return;
  }

  // have a new frame of audio to process
  uint8_t is_last_frame = 0;

#if TRIGGER_MODE_ALWAYS_ON
  // have to wait for the start of the 3rd frame before BG/FG detect has occurred
  if (live_kws_state_info_struct.audio_frame_number>(2+AUDIO_SKIP_FRAME_CNT)) {
    if (AxonKwsLastFrameWasForeground()) {

      // handle a foreground frame.
      live_kws_state_info_struct.total_foregrounds_in_window++; // count it.
      live_kws_state_info_struct.consecutive_backgrounds = 0;   // 0 out consecutive backgrounds
      if (live_kws_state_info_struct.current_state==kWaitingForTrigger) {
        // if we weren't trigged before, we are now.
        AxonPrintf("triggered %d\r\n", live_kws_state_info_struct.audio_frame_number);
        live_kws_state_info_struct.current_state = kTriggered;
      }
    } else {
      //  handle a background frame

      if (live_kws_state_info_struct.audio_frame_number==(1+AUDIO_SKIP_FRAME_CNT+AUDIO_SNIFF_FRAME_CNT)) {
        // if still not triggered and this is the last frame to check, shut it down.
        audio_record_stop(); // done w/ audio!
        AxonHostAudioDis();
        AxonHostAxonDisable();
        audio_sniff_timer_init(0); // keep sniffing
        AxonKwsClearLastResult(NULL); // reset the model state.
        return;
      }
      live_kws_state_info_struct.consecutive_backgrounds++;   // increment consecutive backgrounds
    }

    if ((MAX_HALF_FRAME_COUNT+AUDIO_SKIP_FRAME_CNT) < live_kws_state_info_struct.audio_frame_number) {
      // already triggered, and reached the last frame to record.
      audio_record_stop(); // done w/ audio!
      AxonHostAudioDis();
      // don't process this frame if not ending on a long silence or not enough foregrounds detected.
      if ((live_kws_state_info_struct.consecutive_backgrounds<12) ||
        (live_kws_state_info_struct.total_foregrounds_in_window<8)) {
        audio_sniff_timer_init(ALWAYS_ON_ONESHOT);
        AxonPrintf("expired %d\r\n", live_kws_state_info_struct.audio_frame_number);
        return;
      }
      transition_state(kRecordingStopped, "Recording STOPPED\r\n");
      is_last_frame = 1;
    }
  }

  // process this audio frame
  AxonKwsProcessFrame(
    live_kws_state_info_struct.last_frame,
    RECORD_HALF_FRAME_LEN,
    current_frame,
    1, // stride
    live_kws_state_info_struct.audio_frame_number==(2+AUDIO_SKIP_FRAME_CNT) ? kFirstFrame : is_last_frame ? kLastFrame : kMiddleFrame, // first full frame is after 2nd slice
        is_last_frame ? kDoClassify + MAX_HALF_FRAME_COUNT-1: kDoNotClassify ); // by-pass BG/FG algo; if this is last frame then classify, otherwise don't/

  live_kws_state_info_struct.last_frame = current_frame;

#elif TRIGGER_MODE_BUTTON_PRESS
  if ((MAX_HALF_FRAME_COUNT) < live_kws_state_info_struct.audio_frame_number) {
    // already triggered, and reached the last frame to record.
    audio_record_stop(); // done w/ audio!
    AxonHostAudioDis();
    transition_state(kRecordingStopped, "Recording STOPPED\r\n");
    is_last_frame = 1;
  } else {
    // debug - let's see the audio energy
    // AxonKwsLastFrameWasForeground();
  }

  // process this audio frame
  AxonKwsProcessFrame(
    live_kws_state_info_struct.last_frame,
    RECORD_HALF_FRAME_LEN,
    current_frame,
    1, // stride
    live_kws_state_info_struct.audio_frame_number==2 ? kFirstFrame : is_last_frame ? kLastFrame : kMiddleFrame, // first full frame is after 2nd slice
    live_kws_state_info_struct.current_state!=kTriggered ? kDoNotClassify : kClassifyOnValidWindow);

  live_kws_state_info_struct.last_frame = current_frame;
#endif
  bsp_set_profiling_gpio(0);

}

#ifdef BLE_SDK
void audio_poll_timer_callback(void *context, uint64_t timestamp) {
  audio_framework_handle_timer();
}

extern uint64_t SystemTimeGetTicks64();
static void audio_timer_start(void){
  // want a recurring alarm every 16ms, wh
  live_kws_state_info_struct.alarm_info.recurrence = 16000000*(0.016);
  live_kws_state_info_struct.alarm_info.alarm_time = live_kws_state_info_struct.alarm_info.recurrence +SystemTimeGetTicks64();
  live_kws_state_info_struct.alarm_info.alarm_callback = audio_poll_timer_callback;
  PowerMgrAddAlarm(&live_kws_state_info_struct.alarm_info);

}

/*
 * button up handler invoked by the bsp.
 */
void bsp_user_btn_up_handler() {
  if (live_kws_state_info_struct.current_state != kIdle ) {
    // already busy, ignore it.
    return;
  }
  transition_state(kWaitingForTrigger, "");
  process_state();
}
#else

// can't use timer0 directly w/ ml_ble_platform

/*
 * configure timer0. This is used as the audio buffer polling
 * thread.  Note: sys_clk.pclk appears to the pclk frequency in Mhz.
 *
 */
static void audio_timer_start(){
  timer_set_init_tick(TIMER0, 0);


  timer_set_cap_tick(TIMER0, 16*sys_clk.pclk*1000);
  timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);
  plic_interrupt_enable(IRQ4_TIMER0);
  timer_start(TIMER0);
}

/******************************************************************
 * button functions
 *
 * SW2 is the record button, SW5 is the playback button
 ******************************************************************/
void axon_app_gpio_irq_handler(void)
{
  reg_gpio_irq_clr =FLD_GPIO_IRQ_CLR ;

  if (live_kws_state_info_struct.current_state != kIdle ) {
    // already busy, ignore it.
    return;
  }

#if TRIGGER_MODE_ALWAYS_ON

#  if ALWAYS_ON_ONESHOT
  /*
   * This starts "always on" mode.
   */
  gpio_set_high_level(LED4); // red while waiting for button to be released
  delay_ms(10); // debounce timer...
  while (0!=get_sw2_state()); // wait for it to rise
  gpio_set_low_level(LED4); // turn red off (not waiting for release)
  audio_sniff_timer_init(0);
#  endif

#else
  /*
   * In button mode, record will be triggered by the button going back up.
   */
  transition_state(kWaitingForTrigger, "");
  process_state();
#endif
}

// ml_ble_platform manages gpio IRQs

/*
 * Timer 0 isr
 * - In "waiting for trigger" state, this is the trigger.
 * - In "recording audio" state, need to check-up on the audio buffer.
 */
void axon_app_timer0_irq_handler(void) {
  if(0==(reg_tmr_sta & FLD_TMR_STA_TMR0)) {
    return; //???
  }
  reg_tmr_sta = FLD_TMR_STA_TMR0;
  audio_framework_handle_timer();
}

void axon_app_gpio_risc1_irq_handler(void)
{
  reg_gpio_irq_clr = FLD_GPIO_IRQ_GPIO2RISC1_CLR ;
  live_kws_state_info_struct.sw5_down_event_cnt++;
}

#endif

/*
 * Called upon wake from deepsleep.
 * 2 things to do:
 * 1) Initialize current state to waiting for button. None of our state variables
 *    are in retained memory so this indidcates a fresh start.
 * 2) Check the state of the button gpios. If a button press was the source of the wake-up,
 *    an interrupt will not be generated. So we need to spoof the interrupt.
 */
void AudioFrameworkDeepsleepWakeInit() {
  live_kws_state_info_struct.current_state = kIdle;
#ifndef BLE_SDK
  button_init();
  // check the buttons for pending interrupts.
  if (gpio_get_level(GPIO_PC0)) {
    plic_set_pending(IRQ27_GPIO2RISC1); // GPIO_PC0
  }
  if (gpio_get_level(GPIO_PC2)) {
    plic_set_pending(IRQ25_GPIO); // GPIO_PC2
  }
#endif
}
/*
 * Called one time, after power-on/reset.
 * Needs to prepare any axon operations (indirectly, through axon_ml_lib)
 * and do everything that DeepsleepWakeInit() does.
 */
int AudioFrameworkOneTimeInit() {

  int result = AxonDemoPrepare(NULL);
  AudioFrameworkDeepsleepWakeInit();

#ifdef BLE_SDK
  audio_framework_retained_state.power_voter_id = PowerMgrRequestVoterId();

  // vote for deepsleep w/ gpio wakeup
  PowerMgrVoteForLowPowerState(audio_framework_retained_state.power_voter_id,kLowPowerDeepsleepRetention, 1);
#endif
  return result;
}

int AxonAppPrepare() {
  return AudioFrameworkOneTimeInit();
}
/*
 * called by main, over and over, but this function
 * never returns. we'll control looping from here.
 */
int AxonAppRun(void *unused1, uint8_t deepRetWakeUp) {
#ifdef BLE_SDK
  if(!deepRetWakeUp){
    enable_audio_and_axon();
    AxonDemoRun(NULL, 0);
    audio_framework_print_usage();
    return 0;
  }
  

#else
  AxonDemoRun(NULL, 0);
#if CAPTURE_AUDIO_PLAYBACK
  CopyAudio();
  audio_playback_start();
#endif
  audio_framework_print_usage();

#if TRIGGER_MODE_ALWAYS_ON
  audio_sniff_timer_init(ALWAYS_ON_ONESHOT);
#endif

  /*
   * Starting off in "SidOnly" mode means speaker ID classification 0 unlocks the device and KWS works until
   * user says "STOP", then device is locked again.
   * Starting off in SidAndKws mode means SID and KWS always infer concurrently.
   */
  //uint32_t loop_cnt = 0;
  while(1) {
    // printf("loop cnt A %d\r\n", loop_cnt++);
    process_state();
    // printf("loop cnt B %d\r\n", loop_cnt++);
    uint32_t interrupt_state = AxonHostDisableInterrupts();
    AxonHostWfi();
    AxonHostRestoreInterrupts(interrupt_state);
  }
#endif

}
