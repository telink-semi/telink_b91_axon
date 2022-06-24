/********************************************************************************************************
 * @file	spk_app.c
 *
 * @brief	This is the source file for B91m
 *
 * @author	Driver Group
 * @date	2019
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
#include "app_config.h"
#if(USB_DEMO_TYPE==USB_SPEAKER)
#include "usb_default.h"
#include "application/usbstd/usb.h"
#include <string.h>
#if(MCU_CORE_B91)
#define SPK_SAMPLING_RATE   (SPEAKER_SAMPLE_RATE== 8000) ?  AUDIO_8K :((SPEAKER_SAMPLE_RATE== 16000) ?  AUDIO_16K :(  (SPEAKER_SAMPLE_RATE== 32000) ?  AUDIO_32K :( (SPEAKER_SAMPLE_RATE== 48000) ? AUDIO_48K : AUDIO_16K) ) )
#define SPK_MONO_STEREO       ((SPK_CHANNLE_COUNT==1) ?  MONO_BIT_16 :STEREO_BIT_16 )
#define SPK_DMA_CHN    DMA3
#define	SPK_BUFFER_SIZE			2048
unsigned short		iso_out_buff[SPK_BUFFER_SIZE];

volatile unsigned int		iso_out_w = 0;
volatile unsigned int  	     iso_out_r = 0;


volatile unsigned int		            num_iso_out = 0;

unsigned char more_rate;
unsigned char less_rate;
unsigned char equal_rate;
void user_init(void)
{
#if(CHIP_VER_A0==CHIP_VER)
	audio_set_codec_supply(CODEC_2P8V);
#endif
	gpio_function_en(LED1|LED2|LED3|LED4);
	gpio_output_en(LED1|LED2|LED3|LED4);

	reg_usb_ep6_buf_addr = 0x40;		// 192 max
	reg_usb_ep7_buf_addr = 0x20;		// 32
	reg_usb_ep8_buf_addr = 0x00;
	reg_usb_ep_max_size = (192 >> 3);
	usbhw_data_ep_ack(USB_EDP_SPEAKER);
	//1.enable USB DP pull up 1.5k
	usb_set_pin_en();
	//2.enable USB manual interrupt(in auto interrupt mode,USB device would be USB printer device)
	usb_init_interrupt();
	//3.enable global interrupt
	core_interrupt_enable();
	plic_interrupt_enable(IRQ11_USB_ENDPOINT);		// enable usb endpoint interrupt
	usbhw_set_eps_irq_mask(FLD_USB_EDP6_IRQ);
#if(MCU_CORE_B91)
	usbhw_set_irq_mask(USB_IRQ_RESET_MASK|USB_IRQ_SUSPEND_MASK);
#endif
	audio_init(BUF_TO_LINE_OUT ,SPK_SAMPLING_RATE,SPK_MONO_STEREO);
	audio_tx_dma_chain_init(SPK_DMA_CHN,(unsigned short*)&iso_out_buff,SPK_BUFFER_SIZE*2);
}

/**
 * @brief     This function servers to set USB Input.
 * @param[in] none
 * @return    none.
 */
void  audio_rx_data_from_usb ()
{
	unsigned char len = reg_usb_ep6_ptr;
	usbhw_reset_ep_ptr(USB_EDP_SPEAKER);
	for (unsigned int i=0; i<len; i+=4)
	{
		signed short d0 = reg_usb_ep6_dat;
		d0 |= reg_usb_ep6_dat << 8;
	    signed short d1 = reg_usb_ep6_dat;
		d1 |= reg_usb_ep6_dat << 8;

		iso_out_buff[iso_out_w++ & (SPK_BUFFER_SIZE - 1)] = d0;
		iso_out_buff[iso_out_w++ & (SPK_BUFFER_SIZE - 1)] = d1;



	}
	usbhw_data_ep_ack(USB_EDP_SPEAKER);
}


_attribute_ram_code_sec_ void  usb_endpoint_irq_handler (void)
{
	/////////////////////////////////////
             	// ISO OUT
	/////////////////////////////////////
	if (usbhw_get_eps_irq()&FLD_USB_EDP6_IRQ)
	{
		usbhw_clr_eps_irq(FLD_USB_EDP6_IRQ);
		///////////// output to audio fifo out ////////////////
		audio_rx_data_from_usb();
		num_iso_out++;
		if ((num_iso_out & 0x7f) == 0)		gpio_toggle(LED2);
	}

}
#if 0
/**
 * @brief     This function servers to adjust sampling rate .
 * @param[in] none
 * @return    none.
 */
_attribute_ram_code_sec_  void change_rate(void)
	{
		iso_out_r = ((audio_get_tx_dma_rptr (SPK_DMA_CHN) - (unsigned int)iso_out_buff) >> 1);


		if((((iso_out_w&(SPK_BUFFER_SIZE - 1))- iso_out_r)&(SPK_BUFFER_SIZE - 1))<1000)

		{
			audio_set_i2s_clock(AUDIO_48K,AUDIO_RATE_LT_L1,1);
			less_rate++;
			gpio_toggle(LED3);
		}

		else  if((((iso_out_w&(SPK_BUFFER_SIZE - 1))- iso_out_r)&(SPK_BUFFER_SIZE - 1))>2000)

		{

			 audio_set_i2s_clock(SPK_SAMPLING_RATE,AUDIO_RATE_GT_L1,1);
			 gpio_toggle(LED4);
			 more_rate++;

		}

		else
		{

			audio_set_i2s_clock(SPK_SAMPLING_RATE,AUDIO_RATE_LT_L0,1);
			equal_rate++;

	    }



	}
#endif
unsigned int t=0;
unsigned int temp=0;
void main_loop (void)
{
	usb_handle_irq();
	if(clock_time_exceed(t,2000)&&(num_iso_out))//2ms
	{
		if(num_iso_out==temp)
		{
			memset(iso_out_buff,0,SPK_BUFFER_SIZE * 2);//if host pause playback clear buff.
			num_iso_out=0;
		}

		gpio_toggle(LED4);
		temp=num_iso_out;
		t = stimer_get_tick()|1;

	}
}
#elif(MCU_CORE_B92)
#define SPK_SAMPLING_RATE   (SPEAKER_SAMPLE_RATE== 8000) ?  AUDIO_8K :((SPEAKER_SAMPLE_RATE== 16000) ?  AUDIO_16K :(  (SPEAKER_SAMPLE_RATE== 32000) ?  AUDIO_32K :( (SPEAKER_SAMPLE_RATE== 48000) ? AUDIO_48K : AUDIO_16K) ) )
#define SPK_DMA_CHN    DMA3
#define	SPK_BUFFER_SIZE			2048
unsigned short		iso_out_buff[SPK_BUFFER_SIZE];

volatile unsigned int		iso_out_w = 0;
volatile unsigned int  	     iso_out_r = 0;
volatile unsigned int  	     iso_out_w_r = 0;

unsigned int		            num_iso_out = 0;



void user_init(void)
{
	gpio_function_en(LED1|LED2|LED3|LED4);
	gpio_output_en(LED1|LED2|LED3|LED4);

	reg_usb_ep6_buf_addr = 0x40;		// 192 max
	reg_usb_ep7_buf_addr = 0x20;		// 32
	reg_usb_ep8_buf_addr = 0x00;
	reg_usb_ep_max_size = (192 >> 3);

	//1.enable USB DP pull up 1.5k
	usb_set_pin_en();
	//2.enable USB manual interrupt(in auto interrupt mode,USB device would be USB printer device)
	usb_init_interrupt();
	//3.enable global interrupt
	core_interrupt_enable();
	plic_interrupt_enable(IRQ11_USB_ENDPOINT);		// enable usb endpoint interrupt
	usbhw_set_eps_irq_mask(FLD_USB_EDP6_IRQ);

	//audio_init(BUF_TO_LINE_OUT ,SPK_SAMPLING_RATE,MONO_BIT_16);
	audio_tx_dma_chain_init(SPK_DMA_CHN,(unsigned short*)&iso_out_buff,SPK_BUFFER_SIZE*2,FIFO0);

}


/**
 * @brief     This function servers to set USB Input.
 * @param[in] none
 * @return    none.
 */
void  audio_rx_data_from_usb ()
{
	unsigned char len = reg_usb_ep6_ptr;
	usbhw_reset_ep_ptr(USB_EDP_SPEAKER);
	for (unsigned int i=0; i<len; i+=4)
	{
		signed short d0 = reg_usb_ep6_dat;
		d0 |= reg_usb_ep6_dat << 8;
	    signed short d1 = reg_usb_ep6_dat;
		d1 |= reg_usb_ep6_dat << 8;

		iso_out_buff[iso_out_w++ & (SPK_BUFFER_SIZE - 1)] = d0;
		iso_out_buff[iso_out_w++ & (SPK_BUFFER_SIZE - 1)] = d1;

		iso_out_w_r=((iso_out_w&(SPK_BUFFER_SIZE - 1))- iso_out_r)&(SPK_BUFFER_SIZE - 1);

	}
	usbhw_data_ep_ack(USB_EDP_SPEAKER);
}


_attribute_ram_code_sec_ void  usb_endpoint_irq_handler (void)
{
	/////////////////////////////////////
             	// ISO OUT
	/////////////////////////////////////
	if (usbhw_get_eps_irq()&FLD_USB_EDP6_IRQ)
	{
		usbhw_clr_eps_irq(FLD_USB_EDP6_IRQ);
		///////////// output to audio fifo out ////////////////
		audio_rx_data_from_usb();
		num_iso_out++;
		if ((num_iso_out & 0x7f) == 0)		gpio_toggle(LED2);
	}

}


void main_loop (void)
{
	usb_handle_irq();
}

#endif
#endif


