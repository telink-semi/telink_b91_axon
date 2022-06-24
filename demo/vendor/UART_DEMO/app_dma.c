/********************************************************************************************************
 * @file	app_dma.c
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

#if (UART_MODE==UART_DMA)

#if( FLOW_CTR==USE_CTS)
	#define STOP_VOLT   	1			//0 :Low level stops TX.  1 :High level stops TX.
#endif

#if (FLOW_CTR==USE_RTS)
	#define RTS_MODE		UART0_RTS_MODE_AUTO 		    //It can be UART_RTS_MODE_AUTO/UART_RTS_MODE_MANUAL.
	#define RTS_THRESH		5			//UART_RTS_MODE_AUTO need.It indicates RTS trigger threshold.
	#define RTS_INVERT		1			//UART_RTS_MODE_AUTO need.1 indicates RTS_pin will change from low to hign.
	#define RTS_POLARITY	0			//UART_RTS_MODE_MANUAL need. It indicates RTS_POLARITY .
#endif

#define UART_MASTER_DEVICE   1
#define UART_SLAVE_DEVICE    2

#define UART_DEVICE        UART_MASTER_DEVICE

#define UART_DMA_CHANNEL_RX  DMA2
#define UART_DMA_CHANNEL_TX  DMA3

#define BUFF_DATA_LEN     256

#define DMA_REV_LEN_LESS_MAX     1       //b91 and b92 are optional
#define DMA_REV_LEN_MAX          2       //only b92 can choose
#define DMA_REV_LEN_TYPE         1

#if(DMA_REV_LEN_TYPE == DMA_REV_LEN_LESS_MAX)
#define DMA_REV_LEN           BUFF_DATA_LEN
#elif(MCU_CORE_B92 && (DMA_REV_LEN_TYPE == DMA_REV_LEN_MAX))
#define DMA_REV_LEN           0xFFFFFC
#endif


volatile unsigned char tx_byte_buff[16] __attribute__((aligned(4))) ={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
volatile unsigned char rec_buff[BUFF_DATA_LEN] __attribute__((aligned(4))) = {0};
volatile unsigned char uart_dma_send_flag = 1;
volatile unsigned char rev_data_len=0;

void user_init()
{
	unsigned short div;
	unsigned char bwpc;
	gpio_function_en(LED1|LED2|LED3|LED4);
	gpio_output_en(LED1|LED2|LED3|LED4); 	//enable output
	gpio_input_dis(LED1|LED2|LED3|LED4);   //disable input
	uart_reset(UART0);
#if( UART_WIRE_MODE == UART_1WIRE_MODE)
#if(MCU_CORE_B91)
	uart_set_rtx_pin(UART0_RTX_PIN);// uart0 rtx pin set
#elif(MCU_CORE_B92)
	uart_set_rtx_pin(UART0,UART0_RTX_PIN);// uart0 rtx pin set
#endif
	uart_rtx_en(UART0);
#elif(UART_WIRE_MODE == UART_2WIRE_MODE)
#if(MCU_CORE_B91)
	uart_set_pin(UART0_TX_PIN,UART0_RX_PIN);
#elif(MCU_CORE_B92)
	uart_set_pin(UART0,UART0_TX_PIN,UART0_RX_PIN);// second parameter is tx_pin,third parameter is rx_pin.
	uart_rxdone_sel(UART0,0);// no_dma:can optionally turn it on,if you use rx_done,it must be on; dma: whether rx_done is used or not, it must be closed.
#endif
#endif
	uart_cal_div_and_bwpc(115200, sys_clk.pclk*1000*1000, &div, &bwpc);
	uart_set_rx_timeout(UART0, bwpc, 12, UART_BW_MUL2);//uart use rx_timeout to decide the end of a packet,UART_BW_MUL2 means rx_timeout is 2-byte.
	uart_init(UART0, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);
#if( FLOW_CTR ==  USE_CTS )
	uart_cts_config(UART0,UART0_CTS_PIN,STOP_VOLT);
	uart_set_cts_en(UART0);
#elif( FLOW_CTR ==   USE_RTS )
	uart_set_rts_en(UART0);
	uart_rts_config(UART0,UART0_RTS_PIN,RTS_INVERT,UART_RTS_MODE_AUTO);
	uart_rts_trig_level_auto_mode(UART0, RTS_THRESH);
#endif
	uart_set_tx_dma_config(UART0, UART_DMA_CHANNEL_TX);
	uart_set_rx_dma_config(UART0, UART_DMA_CHANNEL_RX);
#if(MCU_CORE_B91)
	uart_clr_tx_done(UART0);
#endif
	uart_set_irq_mask(UART0, UART_RXDONE_MASK);
	uart_set_irq_mask(UART0, UART_TXDONE_MASK);
	plic_interrupt_enable(IRQ19_UART0);
	core_interrupt_enable();


#if(DMA_REV_LEN  ==  BUFF_DATA_LEN)
/*
	Application scenario(B91/B92) : the dma receive length is set to the rec_buff length to avoid receiving data that is too long and out of the rec_buff address range,
						   but the data receive length needs to be calculated by calling uart_get_dMA_rev_data_len.
	Difference :          B91: the receive length cannot be greater than the DMA set length;
						  B92: the received length can be greater than the DMA set length, but excess data is discarded;
	Possible problems:    B91: In an application, there may be a high priority interrupt source that interrupts the uart rx_done interrupt processing sequence,
						  in interruption period,if the next data is received to the fifo, when the re-entry of the UART RX_DONE interrupt,resulting in the error in computing the received length of the current data.
 */
	uart_receive_dma(UART0, (unsigned char*)rec_buff,DMA_REV_LEN);
#elif(DMA_REV_LEN  ==  0xFFFFFC)
/*
	  Application scenario(B92):  write_num is available, the DMA receive length must be set to 0xfffffc byte,
	                         the dma automatically writes back the data receive length to the first four bytes of the rec_buff without manual calculation.
	  Possible problems:     Because the dma receive length is set to 0xfffffc byte, it is possible that the dma receive length is larger than the rec_buff set length,
	                         which is out of the rec_buff's address range.
*/
	uart_receive_dma(UART0, (unsigned char*)(rec_buff+4),DMA_REV_LEN);
#endif
}
void main_loop (void)
{

#if( (FLOW_CTR == NORMAL)||(FLOW_CTR ==  USE_CTS))
#if(UART_DEVICE==UART_MASTER_DEVICE)
	gpio_toggle(LED1);
	delay_ms(500);
	if(uart_dma_send_flag==1)
	{
		uart_send_dma(UART0, (unsigned char*)tx_byte_buff, 16);
		uart_dma_send_flag=0;
#if( UART_WIRE_MODE == UART_1WIRE_MODE)
		uart_rtx_pin_tx_trig(UART0);
#endif
	}
#elif(UART_DEVICE==UART_SLAVE_DEVICE)
	gpio_toggle(LED1);
	delay_ms(500);
#endif
#elif(FLOW_CTR ==  USE_RTS)
    while(1);
#endif
}
#if( (FLOW_CTR == NORMAL)||(FLOW_CTR ==  USE_CTS))
_attribute_ram_code_sec_ void uart0_irq_handler(void)
{
    if(uart_get_irq_status(UART0,UART_TXDONE))
    {
    	gpio_toggle(LED2);
    	uart_dma_send_flag=1;
    	uart_clr_tx_done(UART0);
    }
    if(uart_get_irq_status(UART0,UART_RXDONE))
    {
        gpio_toggle(LED3);
        if((uart_get_irq_status(UART0,UART_RX_ERR)))
        {
        	uart_clr_irq_status(UART0,UART_CLR_RX);
        }
#if(DMA_REV_LEN  ==  BUFF_DATA_LEN)
        /************************get the length of receive data****************************/
        rev_data_len = uart_get_dma_rev_data_len(UART0,UART_DMA_CHANNEL_RX);
        /************************clr rx_irq****************************/
        //In b91m,clearing the interrupt status flag needs to be placed after calculating the length of the DMA received data,
        //otherwise the DMA received data length is calculated incorrectly.
#if(MCU_CORE_B92)
        uart_clr_irq_status(UART0,UART_CLR_RXDONE);
#endif
        //in b91,clear rx_buff also clear rx_done,in b92,clear rx_buff can't clear rx_done.
        uart_clr_irq_status(UART0,UART_CLR_RX);
        //DMA access memory needs to be aligned according to word.
    	uart_receive_dma(UART0, (unsigned char*)rec_buff,DMA_REV_LEN);
    	uart_send_dma(UART0, (unsigned char*)rec_buff, rev_data_len);

#elif(DMA_REV_LEN  ==  0xFFFFFC)
        /************************get the length of receive data****************************/
        rev_data_len = *(unsigned int*)rec_buff;
        /************************clr rx_irq****************************/
        uart_clr_irq_status(UART0,UART_CLR_RXDONE);
        uart_clr_irq_status(UART0,UART_CLR_RX);
        uart_receive_dma(UART0, (unsigned char*)(rec_buff+4),DMA_REV_LEN);
        uart_send_dma(UART0, (unsigned char*)(rec_buff+4), rev_data_len);

#endif

    }
}
#endif

#endif











