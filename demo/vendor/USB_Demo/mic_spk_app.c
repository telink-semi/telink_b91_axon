/********************************************************************************************************
 * @file	mic_spk_app.c
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
#if(USB_DEMO_TYPE==USB_MIC_SPEAKER)
#include "usb_default.h"
#include "application/usbstd/usb.h"
#include <string.h>

#if (MCU_CORE_B91)
#define MIC_DMA_CHN             DMA2
#define SPK_DMA_CHN             DMA3
#define	MIC_BUFFER_SIZE			4096
#define	SPK_BUFFER_SIZE			4096

unsigned short		iso_in_buff[MIC_BUFFER_SIZE];
unsigned short		iso_out_buff[SPK_BUFFER_SIZE];

int		iso_in_w = 0;
int		iso_in_r = 0;

int		iso_out_w = 0;
int		iso_out_r = 0;

/**
 * @brief     This function serves to send data to USB. only adaptive mono 16bit
 * @param[in] audio_rate - audio rate. This value is matched with usb_default.h :MIC_SAMPLE_RATE.
 * @return    none.
*/

void audio_tx_data_to_usb(audio_sample_rate_e audio_rate)
{
	unsigned char length = 0;
	iso_in_w = ((audio_get_rx_dma_wptr(MIC_DMA_CHN) - (unsigned int)iso_in_buff) >> 1);
	 usbhw_reset_ep_ptr(USB_EDP_MIC);//reset pointer of Endpoint7's buf

	switch(audio_rate)
	{
		case 	AUDIO_8K:	length = 8* MIC_CHANNLE_COUNT;break;
		case	AUDIO_16K:	length = 16*MIC_CHANNLE_COUNT;break;
		case	AUDIO_32K:	length = 32*MIC_CHANNLE_COUNT;break;
		case	AUDIO_48K:	length = 48*MIC_CHANNLE_COUNT;break;
		default:			length = 16*MIC_CHANNLE_COUNT;break;
	}

		for (unsigned char i=0; i<length&& iso_in_r != iso_in_w ; i++)
		{
			short md = iso_in_buff[iso_in_r++ &(MIC_BUFFER_SIZE-1)];
			reg_usb_ep7_dat = md;
			reg_usb_ep7_dat = md >>8;
		}
   usbhw_data_ep_ack(USB_EDP_MIC);

}




/**
 * @brief     This function servers to set USB Input.
 * @param[in] none
 * @return    none.
 */
void  audio_rx_data_from_usb ()
{
	unsigned char len = reg_usb_ep_ptr(USB_EDP_SPEAKER);
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



int		num_iso_in = 0;
volatile int 	num_iso_out = 0;

_attribute_ram_code_sec_ void  usb_endpoint_irq_handler (void)
{
	/////////////////////////////////////
	// ISO IN
	/////////////////////////////////////
	if (usbhw_get_eps_irq()& FLD_USB_EDP7_IRQ)
	{
		usbhw_clr_eps_irq(FLD_USB_EDP7_IRQ);	//clear interrupt flag of endpoint 7
		/////// get MIC input data ///////////////////////////////
		audio_tx_data_to_usb(AUDIO_16K);
		num_iso_in++;
		if ((num_iso_in & 0x7f) == 0) gpio_toggle(LED2);
	}

	/////////////////////////////////////
	// ISO OUT
	/////////////////////////////////////
	if (usbhw_get_eps_irq()&FLD_USB_EDP6_IRQ)
	{
		usbhw_clr_eps_irq(FLD_USB_EDP6_IRQ);
		///////////// output to audio fifo out ////////////////
		audio_rx_data_from_usb();
		num_iso_out++;
		if ((num_iso_out & 0x7f) == 0) gpio_toggle(LED3);
	}

}


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
	plic_interrupt_enable(IRQ11_USB_ENDPOINT);		// enable usb endpoint interrupt

	//1.enable USB DP pull up 1.5k
	usb_set_pin_en();
	//2.enable USB manual interrupt(in auto interrupt mode,USB device would be USB printer device)
	usb_init_interrupt();
	//3.enable global interrupt
	core_interrupt_enable();
	usbhw_set_irq_mask(USB_IRQ_RESET_MASK|USB_IRQ_SUSPEND_MASK);
	/*1  "AUDIO_ADC_16K_DAC_48K" - mic sampling=16K,spk sampling=48K;"AUDIO_16K"-mic sampling=16K,spk sampling=16K,should match the setting  in usb_default.h
	 *2  mic and spk  channel count should be the same (1 or 2 ) in usb_default.h
	 *3  channel count =1,MONO_BIT_16;channel count =2  STEREO_BIT_16*/
	audio_init(LINE_IN_TO_BUF_TO_LINE_OUT,AUDIO_16K,STEREO_BIT_16);
	audio_rx_dma_chain_init(MIC_DMA_CHN,iso_in_buff,MIC_BUFFER_SIZE * 2);
	audio_tx_dma_chain_init (SPK_DMA_CHN,iso_out_buff, SPK_BUFFER_SIZE * 2);


}
unsigned int t=0;
int temp=0;
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

#define MIC_DMA_CHN             DMA2
#define SPK_DMA_CHN             DMA3
#define	MIC_BUFFER_SIZE			4096
#define	SPK_BUFFER_SIZE			4096

unsigned short		iso_in_buff[MIC_BUFFER_SIZE];
unsigned short		iso_out_buff[SPK_BUFFER_SIZE];

void user_init(void)
{
	gpio_function_en(LED1|LED2|LED3|LED4);
	gpio_output_en(LED1|LED2|LED3|LED4);

	reg_usb_ep6_buf_addr = 0x40;		// 192 max
	reg_usb_ep7_buf_addr = 0x20;		// 32
	reg_usb_ep8_buf_addr = 0x00;
	reg_usb_ep_max_size = (192 >> 3);
	plic_interrupt_enable(IRQ11_USB_ENDPOINT);		// enable usb endpoint interrupt

	//1.enable USB DP pull up 1.5k
	usb_set_pin_en();
	//2.enable USB manual interrupt(in auto interrupt mode,USB device would be USB printer device)
	usb_init_interrupt();
	//3.enable global interrupt
	core_interrupt_enable();
#if(USB_MODE==AISO)
	audio_data_fifo0_path_sel(USB_AISO_IN_FIFO,USB_AISO_OUT_FIFO);
	audio_rx_dma_chain_init(DMA0,(unsigned short*)iso_in_buff,MIC_BUFFER_SIZE*2,FIFO0);
	while (audio_get_rx_wptr(FIFO0)<64);
	audio_tx_dma_chain_init(DMA1,(unsigned short*)iso_in_buff,MIC_BUFFER_SIZE*2,FIFO0);
#endif
}
void main_loop (void)
{
	usb_handle_irq();
}
#endif

#endif
