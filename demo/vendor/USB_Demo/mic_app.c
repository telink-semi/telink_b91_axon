/********************************************************************************************************
 * @file	mic_app.c
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
#if(USB_DEMO_TYPE==USB_MICROPHONE)
#include "usb_default.h"
#include "application/usbstd/usb.h"
#if(MCU_CORE_B91)
#define AUDIO_LINE_IN            0
#define AUDIO_AMIC_IN            1
#define AUDIO_DMIC_IN            2

#define  AUDIO_IN_MODE          AUDIO_AMIC_IN

#define MIC_SAMPLING_RATE   (MIC_SAMPLE_RATE== 8000) ?  AUDIO_8K :((MIC_SAMPLE_RATE== 16000) ?  AUDIO_16K :(  (MIC_SAMPLE_RATE== 32000) ?  AUDIO_32K :( (MIC_SAMPLE_RATE== 48000) ? AUDIO_48K : AUDIO_16K) ) )
#define MIC_MONO_STEREO       ((MIC_CHANNLE_COUNT==1) ?  MONO_BIT_16 :STEREO_BIT_16 )

#define	MIC_BUFFER_SIZE			2048
#define MIC_DMA_CHN             DMA2


unsigned short		iso_in_buff[MIC_BUFFER_SIZE];

volatile unsigned int		iso_in_w = 0;
volatile unsigned int  	     iso_in_r = 0;
unsigned int		num_iso_in = 0;
/**
 * @brief     This function serves to send data to USB. only adaptive mono 16bit
 * @param[in] audio_rate - audio rate. This value is matched with usb_default.h :MIC_SAMPLE_RATE.
 * @return    none.
 */

void audio_tx_data_to_usb(audio_sample_rate_e audio_rate)
{
	unsigned char length = 0;
	iso_in_w = ((audio_get_rx_dma_wptr (MIC_DMA_CHN) - (unsigned int)iso_in_buff) >> 1);
	 usbhw_reset_ep_ptr(USB_EDP_MIC);//reset pointer of Endpoint7's buf
	switch(audio_rate)
	{
		case 	AUDIO_8K:	length = 8* MIC_CHANNLE_COUNT;break;
		case	AUDIO_16K:	length = 16*MIC_CHANNLE_COUNT;break;
		case	AUDIO_32K:	length = 32*MIC_CHANNLE_COUNT;break;
		case	AUDIO_48K:	length = 48*MIC_CHANNLE_COUNT;break;
		default:			length = 16*MIC_CHANNLE_COUNT;break;
	}

	if (0 &&(iso_in_w - (iso_in_r&(MIC_BUFFER_SIZE-1)))< length)
	{
		for (unsigned char i=0; i<length*2; i++)
		{
			reg_usb_ep7_dat = 0;
		}
	}

	else
	{
		for (unsigned char i=0; i<length&& iso_in_r != iso_in_w ; i++)
		{
			short md = iso_in_buff[iso_in_r++ &(MIC_BUFFER_SIZE-1)];
			reg_usb_ep7_dat = md;
			reg_usb_ep7_dat = md >>8;
		}

	}
	 usbhw_data_ep_ack(USB_EDP_MIC);
}
/**
 * @brief		This function serves to handle  iso in usb endpoint interrupt ,interval is 1 ms
 * @param[in] 	none
 * @return 		none
 */
_attribute_ram_code_sec_ void  usb_endpoint_irq_handler (void)
{
	if (usbhw_get_eps_irq() & FLD_USB_EDP7_IRQ)
	{

		usbhw_clr_eps_irq(FLD_USB_EDP7_IRQ);;	//clear interrupt flag of endpoint 7
		/////// get MIC input data ///////////////////////////////
		audio_tx_data_to_usb(MIC_SAMPLING_RATE);
		num_iso_in++;
		if ((num_iso_in & 0x7f) == 0)		gpio_toggle(LED1);
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
	//1.enable USB DP pull up 1.5k
	usb_set_pin_en();
	//2.enable USB manual interrupt(in auto interrupt mode,USB device would be USB printer device)
	usb_init_interrupt();
	//3.enable global interrupt
	core_interrupt_enable();
	plic_interrupt_enable(IRQ11_USB_ENDPOINT);		// enable usb endpoint interrupt
	usbhw_set_eps_irq_mask(FLD_USB_EDP7_IRQ);
#if(MCU_CORE_B91)
	usbhw_set_irq_mask(USB_IRQ_RESET_MASK|USB_IRQ_SUSPEND_MASK);
#endif
#if(AUDIO_IN_MODE==AUDIO_LINE_IN)
   // set in path digital and analog gain, must be set before audio_init();
	audio_set_codec_in_path_a_d_gain(CODEC_IN_D_GAIN_12_DB,CODEC_IN_A_GAIN_20_DB);
	audio_init(LINE_IN_TO_BUF ,MIC_SAMPLING_RATE,MIC_MONO_STEREO);
	audio_rx_dma_chain_init(DMA2,(unsigned short*)&iso_in_buff,MIC_BUFFER_SIZE*2);
#elif(AUDIO_IN_MODE==AUDIO_DMIC_IN)
	audio_set_dmic_pin(DMIC_GROUPD_D4_DAT_D5_D6_CLK);
	// set in path digital gain,analog gain does not work, must be set before audio_init(),;
	audio_set_codec_in_path_a_d_gain(CODEC_IN_D_GAIN_12_DB,CODEC_IN_A_GAIN_0_DB);
	audio_init(DMIC_IN_TO_BUF ,MIC_SAMPLING_RATE,MIC_MONO_STEREO);
	audio_rx_dma_chain_init(DMA2,(unsigned short*)&iso_in_buff,MIC_BUFFER_SIZE*2);
#elif(AUDIO_IN_MODE==AUDIO_AMIC_IN)
	// set in path digital and analog gain, must be set before audio_init();
	audio_set_codec_in_path_a_d_gain(CODEC_IN_D_GAIN_12_DB,CODEC_IN_A_GAIN_20_DB);
	audio_init(AMIC_IN_TO_BUF ,MIC_SAMPLING_RATE,MIC_MONO_STEREO);
	audio_rx_dma_chain_init(DMA2,(unsigned short*)&iso_in_buff,MIC_BUFFER_SIZE*2);
#endif

}

void main_loop (void)
{
	usb_handle_irq();
}

#elif(MCU_CORE_B92)
#define AUDIO_LINE_IN            0
#define AUDIO_AMIC_IN            1
#define AUDIO_DMIC_IN            2

#define  AUDIO_IN_MODE          AUDIO_DMIC_IN

#define MIC_SAMPLING_RATE   (MIC_SAMPLE_RATE== 8000) ?  AUDIO_8K :((MIC_SAMPLE_RATE== 16000) ?  AUDIO_16K :(  (MIC_SAMPLE_RATE== 32000) ?  AUDIO_32K :( (MIC_SAMPLE_RATE== 48000) ? AUDIO_48K : AUDIO_16K) ) )
#define MIC_MONO_STEREO       ((MIC_CHANNLE_COUNT==1) ?  MONO_BIT_16 :STEREO_BIT_16 )

#define	MIC_BUFFER_SIZE			2048
#define MIC_DMA_CHN             DMA0


#define DMIC_DEC0              0//only dec0 stored in the buff
#define DMIC_DEC1              1//only dec1 stored in the buff
#define DMIC_2_DEC_FOR_DEC0    2//The data of dec0 and dec1 are stored in one buff,only process dec0 data( the second 4bytes in the buff).
#define DMIC_2_DEC_FOR_DEC1    3//The data of dec0 and dec1 are stored in one buff,only process dec1 data( the  first 4bytes in the buff).

#define DMIC_INPUT_MODE   DMIC_DEC0




unsigned short		iso_in_buff[MIC_BUFFER_SIZE];
unsigned short		iso_in_buff1[MIC_BUFFER_SIZE];
volatile unsigned int		iso_in_w = 0;
volatile unsigned int  	     iso_in_r = 0;
unsigned int		num_iso_in = 0;
/**
 * @brief     This function serves to send data to USB. only adaptive mono 16bit
 * @param[in] audio_rate - audio rate. This value is matched with usb_default.h :MIC_SAMPLE_RATE.
 * @return    none.
 */
#if (USB_MODE==INT)
void audio_tx_data_to_usb(audio_sample_rate_e audio_rate)
{
	unsigned char length = 0;
	 usbhw_reset_ep_ptr(USB_EDP_MIC);//reset pointer of Endpoint7's buf
	switch(audio_rate)
	{
		case 	AUDIO_8K:	length = 8* MIC_CHANNLE_COUNT;break;
		case	AUDIO_16K:	length = 16*MIC_CHANNLE_COUNT;break;
		//case	AUDIO_24K:  length = 24*MIC_CHANNLE_COUNT;break;
		case	AUDIO_32K:	length = 32*MIC_CHANNLE_COUNT;break;
		case	AUDIO_48K:	length = 48*MIC_CHANNLE_COUNT;break;
		default:			length = 16*MIC_CHANNLE_COUNT;break;
	}

#if (DMIC_INPUT_MODE==DMIC_2_DEC_FOR_DEC1)//dec1  codec fifo0 4 dmic
	for (unsigned char i=0; i<length ; i++)
		{
			short md=0;
			if(((i&0x01)==0))
			{
				if(iso_in_r==0)
				{
					iso_in_r=0;
				}
				else
				{
					iso_in_r=iso_in_r+3;
				}

			}
			else
			{
				iso_in_r=iso_in_r+1;
			}
			md = iso_in_buff[(iso_in_r) &(MIC_BUFFER_SIZE-1)];
			reg_usb_ep7_dat = md;
			reg_usb_ep7_dat = md >>8;
		}

#elif(DMIC_INPUT_MODE==DMIC_2_DEC_FOR_DEC0)//dec 0

		for (unsigned char i=0; i<length ; i++)
		{
			short md=0;
			if(((i&0x01)==0))
			{
				if(iso_in_r==0)
				{
					iso_in_r=2;
				}
				else
				{
					iso_in_r=iso_in_r+3;
				}

			}
			else
			{
				iso_in_r=iso_in_r+1;
			}
			md = iso_in_buff[(iso_in_r) &(MIC_BUFFER_SIZE-1)];

			reg_usb_ep7_dat = md;
			reg_usb_ep7_dat = md >>8;
		}

#endif

#if (!((DMIC_INPUT_MODE==DMIC_2_DEC_FOR_DEC0)||((DMIC_INPUT_MODE==DMIC_2_DEC_FOR_DEC1))))
	for (unsigned char i=0; i<length ; i++)
		{
			short md = iso_in_buff[iso_in_r++ &(MIC_BUFFER_SIZE-1)];
			reg_usb_ep7_dat = md;
			reg_usb_ep7_dat = md >>8;
		}
#endif

	 usbhw_data_ep_ack(USB_EDP_MIC);
}

/**
 * @brief		This function serves to handle  iso in usb endpoint interrupt ,interval is 1 ms
 * @param[in] 	none
 * @return 		none
 */
_attribute_ram_code_sec_noinline_ void  usb_endpoint_irq_handler (void)
{
	if (usbhw_get_eps_irq() & FLD_USB_EDP7_IRQ)
	{

		usbhw_clr_eps_irq(FLD_USB_EDP7_IRQ);;	//clear interrupt flag of endpoint 7
		/////// get MIC input data ///////////////////////////////
		audio_tx_data_to_usb(MIC_SAMPLING_RATE);
		num_iso_in++;
		if ((num_iso_in & 0x7f) == 0)		gpio_toggle(LED1);
	}
}
#endif


#define    AUDIO_BUFF_SIZE    4096
volatile signed short AUDIO_BUFF[AUDIO_BUFF_SIZE>>1] __attribute__((aligned(4)));

void user_init(void)
{
	   reg_usb_ep7_buf_addr = 0x00;//must to trigger it
	    reg_usb_ep_max_size = (192>> 3);
	gpio_function_en(LED1|LED2|LED3|LED4);
    gpio_output_en(LED1|LED2|LED3|LED4);
   // usbhw_set_ep_addr(USB_EDP_MIC,0x3c0);
	//1.enable USB DP pull up 1.5k
	usb_set_pin_en();
	//2.enable USB manual interrupt(in auto interrupt mode,USB device would be USB printer device)
	usb_init_interrupt();
	//3.enable global interrupt
#if (USB_MODE==INT)
	core_interrupt_enable();
	plic_interrupt_enable(IRQ11_USB_ENDPOINT);		// enable usb endpoint interrupt
	usbhw_set_eps_irq_mask(FLD_USB_EDP7_IRQ);

#if(AUDIO_IN_MODE==AUDIO_LINE_IN)

#elif(AUDIO_IN_MODE==AUDIO_AMIC_IN)

#elif(AUDIO_IN_MODE==AUDIO_DMIC_IN)


#if (DMIC_INPUT_MODE==DMIC_DEC0)
	audio_set_codec_clk(1,16);
	reg_codec_adc_1m_clk_ctr|=FLD_AUDIO_CODEC_DMIC_CLK_MODE;
	audio_set_codec_dec0_sample_rate(MIC_SAMPLING_RATE);
    audio_set_codec_dec0_path(1,1,DMIC_INPUT);
	audio_data_fifo0_path_sel(CODEC_DEC0_IN_FIFO,OUT_NO_USE);
	audio_dec0_in_mux_config(CODEC_BIT_16_STEREO,CODEC_BIT_16_STEREO);
	audio_set_dec0_vol(CODEC_IN_D_GAIN_42_DB);//dec0  codec vol
	audio_set_dmic0_pin(GPIO_PA0,GPIO_PA1);
	audio_rx_dma_chain_init(DMA0,(unsigned short*)iso_in_buff,MIC_BUFFER_SIZE*2,FIFO0);
#elif (DMIC_INPUT_MODE==DMIC_DEC1)
	audio_set_codec_clk(1,16);
	audio_set_codec_dec1_sample_rate(AUDIO_32K);
    audio_set_codec_dec1_path(1,1);
	audio_data_fifo0_path_sel(CODEC_DEC1_IN_FIFO,OUT_NO_USE);
	audio_dec1_in_mux_config(CODEC_BIT_16_STEREO,CODEC_BIT_16_STEREO);
	audio_set_dec1_vol(CODEC_IN_D_GAIN_42_DB);//dec1 codec vol
	audio_set_dmic1_pin(GPIO_PA0,GPIO_PA1);
	audio_rx_dma_chain_init(DMA0,(unsigned short*)iso_in_buff,MIC_BUFFER_SIZE*2,FIFO0);
#elif ((DMIC_INPUT_MODE==DMIC_2_DEC_FOR_DEC0)||(DMIC_INPUT_MODE==DMIC_2_DEC_FOR_DEC1))
	audio_set_codec_clk(1,16);
	reg_codec_adc_1m_clk_ctr|=FLD_AUDIO_CODEC_DMIC_CLK_MODE;
	audio_set_codec_dec0_sample_rate(MIC_SAMPLING_RATE);
    audio_set_codec_dec0_path(1,1,DMIC_INPUT);
	audio_set_codec_dec1_sample_rate(MIC_SAMPLING_RATE);
    audio_set_codec_dec1_path(1,1);
	audio_data_fifo0_path_sel(CODEC_DEC1_IN_FIFO,OUT_NO_USE);
	audio_dec0_in_mux_config(CODEC_BIT_16_STEREO,CODEC_BIT_16_STEREO);
	audio_dec1_in_mux_config(CODEC_BIT_16_STEREO_DEC0_DEC1,CODEC_BIT_16_STEREO_DEC0_DEC1);
	audio_set_dec0_vol(CODEC_IN_D_GAIN_42_DB);//dec0  codec vol
	audio_set_dec1_vol(CODEC_IN_D_GAIN_42_DB);//dec1 codec vol
	audio_set_dmic0_pin(GPIO_PA0,GPIO_PA1);
	audio_set_dmic1_pin(GPIO_PA2,GPIO_PA3);
	audio_rx_dma_chain_init(DMA0,(unsigned short*)iso_in_buff,MIC_BUFFER_SIZE*2,FIFO0);

#endif
#endif
#else
#if (DMIC_INPUT_MODE==DMIC_DEC0)
	audio_set_codec_clk(1,16);
    audio_set_codec_dec0_sample_rate(AUDIO_16K);
    audio_set_codec_dec0_path(1,1,DMIC_INPUT);
	audio_data_fifo0_path_sel(CODEC_DEC0_IN_FIFO,USB_AISO_OUT_FIFO);
	audio_dec0_in_mux_config(CODEC_BIT_16_STEREO,CODEC_BIT_16_STEREO);
	audio_set_dec0_vol(CODEC_IN_D_GAIN_42_DB);//dec0  codec vol
	audio_set_dmic0_pin(GPIO_PA0,GPIO_PA1);
	audio_rx_dma_chain_init(DMA0,(unsigned short*)iso_in_buff,MIC_BUFFER_SIZE*2,FIFO0);
	while (audio_get_rx_wptr(FIFO0)<64);
	audio_tx_dma_chain_init(DMA1,(unsigned short*)iso_in_buff,MIC_BUFFER_SIZE*2,FIFO0);
#elif (DMIC_INPUT_MODE==DMIC_DEC1)
	audio_set_codec_clk(1,16);
	audio_set_codec_dec1_sample_rate(AUDIO_16K);
    audio_set_codec_dec1_path(1,1);
	audio_data_fifo0_path_sel(CODEC_DEC1_IN_FIFO,USB_AISO_OUT_FIFO);
	audio_dec1_in_mux_config(CODEC_BIT_16_STEREO,CODEC_BIT_16_STEREO);
	audio_set_dec1_vol(CODEC_IN_D_GAIN_42_DB);//dec1 codec vol
	audio_set_dmic1_pin(GPIO_PA0,GPIO_PA1);
	audio_rx_dma_chain_init(DMA0,(unsigned short*)iso_in_buff,MIC_BUFFER_SIZE*2,FIFO0);
	while (audio_get_rx_wptr(FIFO0)<64);
	audio_tx_dma_chain_init(DMA1,(unsigned short*)iso_in_buff,MIC_BUFFER_SIZE*2,FIFO0);
#endif

#endif
}
unsigned int t=0;
void main_loop (void)
{
	usb_handle_irq();

	if(clock_time_exceed(t,500000))
		{
			  t = stimer_get_tick()|1;
			 gpio_toggle(LED4);

		  }
}
#endif
#endif


