/********************************************************************************************************
 * @file	app_emi.c
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
#if (TEST_DEMO == EMI_DEMO)

/*
 * @brief 	this macro definition serve to open the setting to deal with problem of zigbee mode 2480Mhz
 * 			band edge can't pass the spec.only use it at the time of certification.
 * */
#define	FIX_ZIGBEE_BANDEAGE_EN	0
/*
 * @brief 	this macro definition serve to close internal cap.
 * */
#define	CLOSE_INTERNAL_CAP_EN	0

#define CAP_SET_FLASH_ADDR_2M 		0x1fe000
#define CAP_SET_FLASH_ADDR_1M 		0xfe000
#define CAP_SET_FLASH_ADDR_512K 	0x7e000//B85m:0x77000 B91m:0x7e000
#define CAP_SET_FLASH_ADDR_128K 	0x1e000
#define CAP_SET_FLASH_ADDR_64K		0xe000


#define TX_PACKET_MODE_ADDR 		     0x00000005
#define RUN_STATUE_ADDR 			     0x00000006
#define TEST_COMMAND_ADDR			     0x00000007
#define POWER_ADDR 					     0x00000008
#define CHANNEL_ADDR				     0x00000009
#define RF_MODE_ADDR				     0x0000000a
#define CD_MODE_HOPPING_CHN			     0x0000000b
#define RSSI_ADDR                        0x00000004
#define RX_PACKET_NUM_ADDR               0x0000000c
#define PA_TX_RX_SETTING				 0x00000014 //2bytes

#define MAX_RF_CHANNEL                   40

/**
 * @brief   The emi test command and function
 */
typedef struct {
	unsigned char  cmd_id; 										/**< emi command id */
	void	 (*func)(rf_mode_e, unsigned char , signed char);   /**< emi function */
}test_list_t;

/**
 * @brief   The emi test mode.
 */
typedef enum{
	ble2m = 0,
	ble1m = 1,
	zigbee250k = 2,
	ble125K = 3,
	ble500K = 4,
	pri2m = 5,
	pri1m = 6,
}emi_rf_mode_e;

/**
 * @brief   Init the emi frequency hopping channel
 */
const unsigned char rf_chn_hipping[MAX_RF_CHANNEL] = {
	48,  4, 66, 62, 44,
	12, 38, 16 ,26, 20,
	22, 68, 18, 28,	42,
	32, 34, 36, 14,	40,
	30, 54, 46, 2,	50,
	52, 80, 56, 78,	74,
	8,  64, 6,  24,	70,
	72, 60, 76, 58, 10,
};

/**
 * @brief   Init the emi reference parameter
 */
unsigned char  g_power_level = 0;  // 0
unsigned char  g_chn = 2;          // 2
unsigned char  g_cmd_now = 1;      // 1
unsigned char  g_run = 1;          // 1
unsigned char  g_hop = 0;          // 0
unsigned char  g_tx_cnt = 0;       // 0
unsigned short g_pa_setting = 0;   // 0
emi_rf_mode_e  g_mode = ble1m;     // 1

void emi_init(void);
void emicarrieronly(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emi_con_prbs9(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emirx(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emitxprbs9(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emitx55(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emitx0f(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emi_con_tx55(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emi_con_tx0f(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emitxaa(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);
void emitxf0(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn);

/**
 * @brief   Init the structure of the emi test command and function
 */
test_list_t  ate_list[] = {
		{0x01,emicarrieronly},
		{0x02,emi_con_prbs9},
		{0x03,emirx},
		{0x04,emitxprbs9},
		{0x05,emitx55},
		{0x06,emitx0f},
		{0x07,emi_con_tx55},
		{0x08,emi_con_tx0f},
		{0x0d,emitxaa},
		{0x0f,emitxf0}
};

//Add by pengcheng 20210401, for PA
#define get_pin(value) (((unsigned short)((value) >> 3) << 8) | BIT((value) & 0x07))
#define check_pa_tx_hw(pin)	((pin == RF_RFFE_TX_PB0) || (pin == RF_RFFE_TX_PB6) || (pin == RF_RFFE_TX_PD7) || (pin == RF_RFFE_TX_PE5))
#define check_pa_rx_hw(pin)	((pin == RF_RFFE_RX_PB1) || (pin == RF_RFFE_RX_PD6) || (pin == RF_RFFE_RX_PE4))

unsigned char pa_hw_flag;
void pa_init(unsigned short v)
{
	unsigned short tx_pin = get_pin(v&0xff);
	unsigned short rx_pin = get_pin((v&0xff00) >> 8);
	if(tx_pin == rx_pin)
	{
		pa_hw_flag = 2;
		return;
	}

	if(check_pa_tx_hw(tx_pin) && check_pa_rx_hw(rx_pin))
	{
		pa_hw_flag = 1;
		rf_set_rffe_pin(tx_pin, rx_pin);
	}
	else
	{
		pa_hw_flag = 0;
		gpio_function_en(tx_pin);
		gpio_input_dis(tx_pin);
		gpio_output_en(tx_pin);
		gpio_set_low_level(tx_pin);
		gpio_function_en(rx_pin);
		gpio_input_dis(rx_pin);
		gpio_output_en(rx_pin);
		gpio_set_low_level(rx_pin);
	}
}

void pa_operation(unsigned short v, unsigned char s)
{
	if(pa_hw_flag == 0)
	{
		unsigned short tx_pin = get_pin(v&0xff);
		unsigned short rx_pin = get_pin((v&0xff00) >> 8);
		if(s == 0) //close
		{
			gpio_set_low_level(tx_pin);
			gpio_set_low_level(rx_pin);
		}
		else if(s == 1) //tx
		{
			gpio_set_high_level(tx_pin);
			gpio_set_low_level(rx_pin);
		}
		else //rx
		{
			gpio_set_low_level(tx_pin);
			gpio_set_high_level(rx_pin);
		}
	}
}

void read_calibration_flash()
{
	unsigned char flash_mid = (flash_read_mid() >> 16) & 0x7f;
	unsigned char chnidx = 0xff;
	if(flash_mid == FLASH_SIZE_2M)
	{
		flash_read_page(CAP_SET_FLASH_ADDR_2M, 1, &chnidx);
	}
	else if(flash_mid == FLASH_SIZE_1M)
	{
		flash_read_page(CAP_SET_FLASH_ADDR_1M, 1, &chnidx);
	}
	else if(flash_mid == FLASH_SIZE_512K)
	{
		flash_read_page(CAP_SET_FLASH_ADDR_512K, 1, &chnidx);
	}
	else if(flash_mid == FLASH_SIZE_128K || flash_mid == FLASH_SIZE_256K)
	{
		flash_read_page(CAP_SET_FLASH_ADDR_128K, 1, &chnidx);
	}
	else
	{
		flash_read_page(CAP_SET_FLASH_ADDR_64K, 1, &chnidx);
	}

	if(chnidx != 0xff)
	{
		rf_update_internal_cap(chnidx);
	}
}

/**
 * @brief		This function serves to EMI Init
 * @return 		none
 */

void emi_init(void)
{
	read_calibration_flash();
#if CLOSE_INTERNAL_CAP_EN
	rf_turn_off_internal_cap();
#endif

	rf_access_code_comm(EMI_ACCESS_CODE);             // access code

    write_sram8(TX_PACKET_MODE_ADDR,g_tx_cnt);        // tx_cnt
    write_sram8(RUN_STATUE_ADDR,g_run);               // run
    write_sram8(TEST_COMMAND_ADDR,g_cmd_now);         // cmd
    write_sram8(POWER_ADDR,g_power_level);            // power
    write_sram8(CHANNEL_ADDR,g_chn);                  // chn
    write_sram8(RF_MODE_ADDR,g_mode);                 // mode
    write_sram8(CD_MODE_HOPPING_CHN,g_hop);           // hop
    write_sram16(PA_TX_RX_SETTING, g_pa_setting);
    write_sram8(RSSI_ADDR,0);                         // rssi
    write_sram32(RX_PACKET_NUM_ADDR,0);               // rx_packet_num
}

/**
 * @brief		This function serves to EMI CarryOnly
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return 		none
 */

void emicarrieronly(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
	pa_operation(g_pa_setting, 1);
	(void)(rf_mode);
	rf_mode_e  power = rf_power_Level_list[pwr];
	rf_emi_tx_single_tone(power,rf_chn);
	while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
			&& ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
			&& ((read_sram8(RF_MODE_ADDR)) == g_mode) && ((read_sram16(PA_TX_RX_SETTING)) == g_pa_setting));
	rf_emi_stop();
}

/**
 * @brief		This function serves to EMI ConPrbs9
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr 	- power level of RF.
 * @param[in]   rf_chn 	- channel of RF.
 * @return 		none
 */
void emi_con_prbs9(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
	pa_operation(g_pa_setting, 1);
	unsigned int t0 = reg_system_tick,chnidx = 1;
	rf_power_level_e power = rf_power_Level_list[pwr];
	g_hop = read_sram8(CD_MODE_HOPPING_CHN);
	rf_emi_tx_continue_update_data(rf_mode,power,rf_chn,0);
	while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
			&& ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
			&& ((read_sram8(RF_MODE_ADDR)) == g_mode) && ((read_sram16(PA_TX_RX_SETTING)) == g_pa_setting))
	{
#if FIX_ZIGBEE_BANDEAGE_EN
		if(rf_mode == RF_MODE_ZIGBEE_250K)
		{
			if (rf_chn == 80)
			{
				write_reg8(0x140c23,0x00);
			}
			else
			{
				write_reg8(0x140c23,0x80);
			}
		}
#endif
		rf_continue_mode_run();
		if(g_hop)
		{
			while(!clock_time_exceed(t0,10000))    // run 10ms
				rf_continue_mode_run();

			while(!clock_time_exceed(t0,20000));   // stop 20ms
			t0 = reg_system_tick;
			rf_emi_tx_continue_update_data(rf_mode,power,rf_chn_hipping[chnidx - 1],0);
			(chnidx >= MAX_RF_CHANNEL)?(chnidx = 1):(chnidx++);
		}
	}
	rf_emi_stop();

	rf_set_tx_rx_off();
	dma_reset();
	rf_emi_reset_baseband();
}

/**
 * @brief		This function serves to EMI Rx
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr	    - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return 		none
 */
void emirx(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
	pa_operation(g_pa_setting, 2);
	(void)(pwr);
	rf_emi_rx_setup(rf_mode,rf_chn);
	write_sram8(RSSI_ADDR,0);
	write_sram32(RX_PACKET_NUM_ADDR,0);
	while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
			&& ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
			&& ((read_sram8(RF_MODE_ADDR)) == g_mode)  && ((read_sram16(PA_TX_RX_SETTING)) == g_pa_setting))
	{
		rf_emi_rx_loop();
		if(rf_emi_get_rxpkt_cnt() != read_sram32(RX_PACKET_NUM_ADDR))
		{
		    write_sram8(RSSI_ADDR,rf_emi_get_rssi_avg());
		    write_sram32(RX_PACKET_NUM_ADDR,rf_emi_get_rxpkt_cnt());
		}
	}
	rf_emi_stop();
}

/**
 * @brief		This function serves to EMI TxPrbs
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return 		none
 */

void emitxprbs9(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
	pa_operation(g_pa_setting, 1);
	unsigned int tx_num = 0;
	rf_power_level_e  power = rf_power_Level_list[pwr];
	rf_emi_tx_burst_setup(rf_mode,power,rf_chn,1);
	while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
			&& ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
			&& ((read_sram8(RF_MODE_ADDR)) == g_mode) && ((read_sram16(PA_TX_RX_SETTING)) == g_pa_setting))
	{
		rf_emi_tx_burst_loop(rf_mode,0);
		if(g_tx_cnt)
		{
			tx_num++;
			if(tx_num >= 1000)
				break;
		}
	}
	rf_emi_stop();
}

/**
 * @brief		This function serves to EMI Tx55
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return 		none
 */

void emitx55(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
	pa_operation(g_pa_setting, 1);
	unsigned int tx_num = 0;
	rf_power_level_e  power = rf_power_Level_list[pwr];
	rf_emi_tx_burst_setup(rf_mode,power,rf_chn,2);
	while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
			&& ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
			&& ((read_sram8(RF_MODE_ADDR)) == g_mode) && ((read_sram16(PA_TX_RX_SETTING)) == g_pa_setting))
	{
		rf_emi_tx_burst_loop(rf_mode,2);
		if(g_tx_cnt)
		{
			tx_num++;
			if(tx_num >= 1000)
				break;
		}
	}
	rf_emi_stop();
}

/**
 * @brief		This function serves to EMI Tx0f
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return 		none
 */
void emitx0f(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
	pa_operation(g_pa_setting, 1);
	unsigned int tx_num = 0;
	rf_power_level_e  power = rf_power_Level_list[pwr];
	rf_emi_tx_burst_setup(rf_mode,power,rf_chn,1);
	while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
			&& ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
			&& ((read_sram8(RF_MODE_ADDR)) == g_mode) && ((read_sram16(PA_TX_RX_SETTING)) == g_pa_setting))
	{
		rf_emi_tx_burst_loop(rf_mode,1);
		if(g_tx_cnt)
		{
			tx_num++;
			if(tx_num >= 1000)
				break;
		}
	}
	rf_emi_stop();
}
/**
 * @brief		This function serves to EMI Txaa
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return 		none
 */
void emitxaa(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
	pa_operation(g_pa_setting, 1);
	unsigned int tx_num = 0;
	rf_power_level_e  power = rf_power_Level_list[pwr];
	rf_emi_tx_burst_setup(rf_mode,power,rf_chn,3);
	while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
			&& ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
			&& ((read_sram8(RF_MODE_ADDR)) == g_mode) && ((read_sram16(PA_TX_RX_SETTING)) == g_pa_setting))
	{
		rf_emi_tx_burst_loop(rf_mode,3);
		if(g_tx_cnt)
		{
			tx_num++;
			if(tx_num >= 1000)
				break;
		}
	}
	rf_emi_stop();
}

/**
 * @brief		This function serves to EMI Txf0
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return 		none
 */
void emitxf0(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
	pa_operation(g_pa_setting, 1);
	unsigned int tx_num = 0;
	rf_power_level_e  power = rf_power_Level_list[pwr];
	rf_emi_tx_burst_setup(rf_mode,power,rf_chn,4);
	while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
			&& ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
			&& ((read_sram8(RF_MODE_ADDR)) == g_mode) && ((read_sram16(PA_TX_RX_SETTING)) == g_pa_setting))
	{
		rf_emi_tx_burst_loop(rf_mode,4);
		if(g_tx_cnt)
		{
			tx_num++;
			if(tx_num >= 1000)
				break;
		}
	}
	rf_emi_stop();
}



/**
 * @brief		This function serves to EMI_CON_TX55
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return 		none
 */
void emi_con_tx55(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
	pa_operation(g_pa_setting, 1);
	rf_power_level_e power = rf_power_Level_list[pwr];
	rf_emi_tx_continue_update_data(rf_mode,power,rf_chn,2);
	while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
			&& ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
			&& ((read_sram8(RF_MODE_ADDR)) == g_mode) && ((read_sram16(PA_TX_RX_SETTING)) == g_pa_setting))
	{
		rf_continue_mode_run();
	}
	rf_emi_stop();
}

/**
 * @brief		This function serves to EMI_CON_TX0f
 * @param[in]   rf_mode - mode of RF.
 * @param[in]   pwr     - power level of RF.
 * @param[in]   rf_chn  - channel of RF.
 * @return 		none
 */
void emi_con_tx0f(rf_mode_e rf_mode,unsigned char pwr,signed char rf_chn)
{
	pa_operation(g_pa_setting, 1);
	rf_power_level_e power = rf_power_Level_list[pwr];
	rf_emi_tx_continue_update_data(rf_mode,power,rf_chn,1);
	while( ((read_sram8(RUN_STATUE_ADDR)) == g_run ) &&  ((read_sram8(TEST_COMMAND_ADDR)) == g_cmd_now )\
			&& ((read_sram8(POWER_ADDR)) == g_power_level ) &&  ((read_sram8(CHANNEL_ADDR)) == g_chn )\
			&& ((read_sram8(RF_MODE_ADDR)) == g_mode)  && ((read_sram16(PA_TX_RX_SETTING)) == g_pa_setting))
	{
		rf_continue_mode_run();
	}
	rf_emi_stop();
}

/**
 * @brief	  This function serves to EMI ServiceLoop
 * @return 	  none
 */
void emi_serviceloop(void)
{
   unsigned char i = 0;

   while(1)
   {
	   g_run = read_sram8(RUN_STATUE_ADDR);  // get the run state!
	   if(g_run != 0)
	   {
		   g_power_level = read_sram8(POWER_ADDR);
		   g_chn = read_sram8(CHANNEL_ADDR);
		   g_mode = read_sram8(RF_MODE_ADDR);
		   g_cmd_now = read_sram8(TEST_COMMAND_ADDR);  // get the command!
		   g_tx_cnt = read_sram8(TX_PACKET_MODE_ADDR);
		   g_hop = read_sram8(CD_MODE_HOPPING_CHN);
		   g_pa_setting = read_sram16(PA_TX_RX_SETTING);
		   pa_init(g_pa_setting);

		   for (i = 0;i < sizeof(ate_list)/sizeof(test_list_t);i++)
		   {
				if(g_cmd_now == ate_list[i].cmd_id)
				{
					switch(g_mode)
					{
					  case ble2m:
						  ate_list[i].func(RF_MODE_BLE_2M,g_power_level,g_chn);
						  break;

					  case ble1m:
						  ate_list[i].func(RF_MODE_BLE_1M_NO_PN,g_power_level,g_chn);
						  break;

					  case zigbee250k:
						  ate_list[i].func(RF_MODE_ZIGBEE_250K,g_power_level,g_chn);
						  break;

					  case ble125K:
						  ate_list[i].func(RF_MODE_LR_S8_125K,g_power_level,g_chn);
						  break;

					  case ble500K:
						  ate_list[i].func(RF_MODE_LR_S2_500K,g_power_level,g_chn);
						  break;

					  case pri2m:
						  ate_list[i].func(RF_MODE_PRIVATE_2M,g_power_level,g_chn);
						  break;

					  case pri1m:
						  ate_list[i].func(RF_MODE_PRIVATE_1M,g_power_level,g_chn);
						  break;

					  default:break;
					}
				}
		   }
		   pa_operation(g_pa_setting, 0);
		   g_run = 0;
		   write_sram8(RUN_STATUE_ADDR,g_run);
	   }
   }
}

/**
 * @brief		This function serves to User Init
 * @return 		none
 */

void user_init(void)
{
	emi_init();
}

/**
 * @brief		This function serves to main
 * @return 		none
 */
void main_loop(void)
{
	emi_serviceloop();
}

#endif
