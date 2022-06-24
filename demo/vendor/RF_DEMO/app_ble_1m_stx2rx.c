/********************************************************************************************************
 * @file	app_ble_1m_stx2rx.c
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
#if RF_MODE == RF_BLE_STX2RX


unsigned char  rx_packet[128*4]  __attribute__ ((aligned (4)));
unsigned char  ble_tx_packet[48] __attribute__ ((aligned (4))) ={3,0,0,0,0,10,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xc,0xf};



#define TX_FIRST				1
#define RX_FIRST				2
#define RF_STRX_MODE			RX_FIRST

#define STX_WAITTIME_US         4000

#define TX_DELAY_US             10



#define TX_PKT_PAYLOAD		15

#define RX_FIFO_NUM			4
#define RX_FIFO_DEP			128


#define RF_FREQ				35
#define RF_POWER			RF_POWER_P4p35dBm
#define ACCESS_CODE        0xd6be898e//0x29417671
volatile unsigned int rx_cnt=0;
volatile unsigned int tx_cnt=0;
volatile unsigned int timeout_cnt=0;
volatile unsigned int tx_state=0;
volatile unsigned int rx_state=0;
volatile unsigned int timeout_state=0;


_attribute_ram_code_sec_noinline_ void rf_irq_handler(void)
{
	if(rf_get_irq_status(FLD_RF_IRQ_TX))
		{
			tx_state = 1;
			gpio_toggle(LED4);
			rf_clr_irq_status(FLD_RF_IRQ_TX);
		}

	if(rf_get_irq_status(FLD_RF_IRQ_RX))
		{
			rx_state = 1;
			gpio_toggle(LED5);
			rf_clr_irq_status(FLD_RF_IRQ_RX);
		}

	if(rf_get_irq_status(FLD_RF_IRQ_RX_TIMEOUT))
		{
			timeout_state = 1;
			gpio_toggle(LED6);
			rf_clr_irq_status(FLD_RF_IRQ_RX_TIMEOUT);
		}
	 if(rf_get_irq_status(FLD_RF_IRQ_FIRST_TIMEOUT))
		{
			timeout_state = 1;
			gpio_toggle(LED6);
			rf_clr_irq_status(FLD_RF_IRQ_FIRST_TIMEOUT);
		}


}



void user_init()
{
	gpio_function_en(LED1|LED2|LED3|LED4|LED5|LED6);
	gpio_output_en(LED1|LED2|LED3|LED4|LED5|LED6);
	rf_set_power_level (RF_POWER);
	rf_access_code_comm(ACCESS_CODE);
	rf_set_ble_chn(RF_FREQ);
	rf_clr_irq_mask(0xffff);
	unsigned char rf_data_len = TX_PKT_PAYLOAD+2;
	ble_tx_packet[4]=0;
	ble_tx_packet[5]=TX_PKT_PAYLOAD;
	unsigned int rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
	ble_tx_packet[3] = (rf_tx_dma_len >> 24)&0xff;
	ble_tx_packet[2] = (rf_tx_dma_len >> 16)&0xff;
	ble_tx_packet[1] = (rf_tx_dma_len >> 8)&0xff;
	ble_tx_packet[0] = rf_tx_dma_len&0xff;
	rf_set_rx_dma(rx_packet,RX_FIFO_NUM-1,RX_FIFO_DEP);
	rf_set_tx_dma(2,128);

#if(RF_STRX_MODE==TX_FIRST)
	core_interrupt_enable();
	plic_interrupt_enable(IRQ15_ZB_RT);
	rf_set_irq_mask(FLD_RF_IRQ_TX|FLD_RF_IRQ_RX|FLD_RF_IRQ_RX_TIMEOUT);
#elif(RF_STRX_MODE==RX_FIRST)
	core_interrupt_enable();
	plic_interrupt_enable(IRQ15_ZB_RT);
	rf_set_irq_mask(FLD_RF_IRQ_TX|FLD_RF_IRQ_RX|FLD_RF_IRQ_FIRST_TIMEOUT);
#endif

}
void main_loop (void)
{
#if(RF_STRX_MODE==TX_FIRST)
//	reg_rf_rx_timeout = SRX_WAITTIME_US;
	rf_set_rx_timeout(0xffff);
	while(1)
	{
		tx_state=0;
		rx_state=0;
		timeout_state=0;
		delay_ms(100);
		rf_start_stx2rx (ble_tx_packet, stimer_get_tick()+16*TX_DELAY_US);
		while(1)
		{
			if(tx_state==1)//tx
			{
				tx_state = 0;
				tx_cnt++;
			}
			else if(rx_state==1)//rx
			{
				if(rf_ble_packet_crc_ok(rx_packet))
				rx_cnt++;
				break;

			}
			else if(timeout_state==1)//time out
			{
				timeout_cnt++;
				break;
			}
		}
	}


#elif(RF_STRX_MODE==RX_FIRST)

	while(1)
	{
		tx_state=0;
		rx_state=0;
		timeout_state=0;
		rf_start_srx2tx (ble_tx_packet, stimer_get_tick()+16*TX_DELAY_US );
		while(1)
		{
			if(tx_state==1)//tx
			{
				tx_state = 0;
				tx_cnt++;
				break;
			}
			else if(rx_state==1)//rx (check crc & length)
			{
				rx_state=0;
				if(rf_ble_packet_crc_ok(rx_packet))
				rx_cnt++;
			}
			else if(timeout_state==1)//time out
			{
				timeout_state = 0;
				timeout_cnt++;
				break;
			}
		}
	}
#endif
}

#endif
