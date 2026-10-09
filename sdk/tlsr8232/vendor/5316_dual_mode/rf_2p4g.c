/********************************************************************************************************
 * @file     rf_2p4g.c
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
 * @date     August. 30, 2019
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *			 The information contained herein is confidential and proprietary property of Telink
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in.
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "rf_2p4g.h"
#include "app.h"
#include "app_config.h"

#include "vendor/link_layer/rf_ll.h"
#include "vendor/common/keyboard.h"

rf_packet_pairing_t	pkt_pairing = {
		sizeof (rf_packet_pairing_t) - 4,	// dma_len
		sizeof (rf_packet_pairing_t) - 5,	// rf_len
		RF_PROTO_BYTE,						// proto
		PKT_FLOW_TOKEN,	                    // flow
		FRAME_TYPE_KEYBOARD,				// type
		0,									// rssi
		0,									// per
		0,									// seq_no
		0,									// reserved
};

rf_packet_keyboard_t	pkt_km = {
		sizeof (rf_packet_keyboard_t) - 4,	// dma_len
		sizeof (rf_packet_keyboard_t) - 5,	// rf_len,	28-5 = 0x17
		RF_PROTO_BYTE,						// proto,	0x51
		PKT_FLOW_DIR,						// flow,	0x80
		FRAME_TYPE_KEYBOARD,				// type,	0x01
		0,									// rssi
		0,									// per
		0,									// seq_no
		1,									// pno
};

kb_status_t  kb_status;

u8 	km_dat_sending 			= 0;
u8 	pipe1_send_id_flg 		= 0;
u8 	key_2p4g_not_released 	= 0;

u8* kb_rf_pkt 				= (u8*)&pkt_pairing;

u32 tick_key_pressed;

extern kb_data_t		kb_event;
extern systemStatus_t 	sys_status;

extern unsigned int 	cpu_wakup_last_tick;


void user_2p4g_init(){

	kb_status.no_ack = 1;

	//get id from flash 76000(ble MAC address low 4 byte)
	if ( *(u32 *) CFG_ADR_MAC != U32_MAX) {
		pkt_pairing.did = *(u32 *) CFG_ADR_MAC;
	}

	kb_status.link_ok = analog_read(PM_REG_MODE_LINK);
	if(kb_status.link_ok){
		u32 did;
		u8 * pd = (u8 *) &did;
		int i;
		for (i=PM_REG_DONGLE_PIPECODE_LOW; i<=PM_REG_DONGLE_PIPECODE_HIGH; i++) {
			*pd ++ = analog_read (i);
		}
		kb_status.dongle_pipeCode = did;
		rf_set_access_code_data (did);
		kb_status.kb_mode =  STATE_CONNECT;
	}
	else{
		kb_status.link_ok 			= 0;
		kb_status.dongle_pipeCode 	= 0;
		rf_set_access_code_pairing (0x39517695);
		kb_status.kb_mode =  STATE_PAIRING;
	}

	//rf init
	ll_device_init ();
	rf_receiving_pipe_enble(0x3f);	//open all RX receive pipe(pipe0 - pipe5)
	kb_status.tx_retry = 5;
	kb_status.tx_power = RF_POWER_7dBm;

	if(kb_status.kb_mode ==  STATE_CONNECT){
		kb_rf_pkt = (u8*)&pkt_km;
		rf_set_power_level_index(kb_status.tx_power);
	}
	else{
		kb_rf_pkt = (u8*)&pkt_pairing;
		rf_set_power_level_index (RF_POWER_0dBm);
	}
	/***********************************************************************************
	 * Keyboard matrix initialization. These section must be before battery_power_check.
	 * Because when low battery,chip will entry deep.if placed after battery_power_check,
	 * it is possible that can not wake up chip.
	 *  *******************************************************************************/
	u32 pin[] = KB_DRIVE_PINS;
	for(int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
	{
		gpio_set_wakeup(pin[i],1,1);  	   //drive pin core(gpio) high wakeup suspend
		cpu_set_gpio_wakeup (pin[i],1,1);  //drive pin pad high wakeup deepsleep
	}

	km_dat_sending = 1;
	tick_key_pressed = clock_time();
}

_attribute_ram_code_ int  rf_rx_process(u8 * p)
{
	rf_packet_ack_pairing_t *p_pkt = (rf_packet_ack_pairing_t *) (p + 8);
	if (p_pkt->proto == RF_PROTO_BYTE) {
		pkt_pairing.rssi = p[4];
		///////////////  Paring/Link ACK //////////////////////////
		if ( p_pkt->type == FRAME_TYPE_ACK && (p_pkt->did == pkt_pairing.did) ) {	//paring/link request
			rf_set_access_code_data(p_pkt->gid1);//need change to pipe2 that is for kb's data
			kb_status.dongle_pipeCode = p_pkt->gid1;
			kb_status.link_ok = 1;
			return 1;
		}
		////////// end of PIPE0 /////////////////////////////////////
		///////////// PIPE1: ACK /////////////////////////////
		else if (p_pkt->type == FRAME_TYPE_ACK_KEYBOARD) {
			kb_status.kb_pipe_rssi = p[4];
			pipe1_send_id_flg = 0;
			return 1;
		}
		else if (p_pkt->type == FRAME_AUTO_ACK_KB_ASK_ID){
			pipe1_send_id_flg = 1;//fix auto bug
			return 1;
		}
		////////// end of PIPE1 /////////////////////////////////////
	}
	return 0;
}

_attribute_ram_code_ void rf_2p4g_proc( u32 det_key ){
	static u32 kb_tx_retry_thresh = 0;

	if (kb_status.link_ok ) {
        if ( det_key ){
			memcpy ((void *) &pkt_km.data[0], (void *) &kb_event, sizeof(kb_data_t));
			pkt_km.seq_no++;
			km_dat_sending = 1;
			kb_tx_retry_thresh = 0x400;
		}
#if 1	//KEYBOARD_PIPE1_DATA_WITHOUT_DID
    	//fix auto paring bug, if dongle ACK ask for  id,send it in on pipe1
        int allow_did_in_kb_data = 0;
		if(pipe1_send_id_flg){
			if(det_key){ //kb data with did in last 4 byte
				if(kb_event.cnt < 3){
	        		allow_did_in_kb_data = 1;
				}
			}
			else{  //no kb data, only did in last 4 byte; seq_no keep same, so dongle reject this invalid data
				allow_did_in_kb_data = 1;
			}

			if(allow_did_in_kb_data){
				*(u32 *) (&pkt_km.data[4]) = pkt_pairing.did;  //did in last 4 byte
				pkt_km.type = FRAME_TYPE_KB_SEND_ID;
				km_dat_sending = 1;
				kb_tx_retry_thresh = 0x400;
			}
			else{
				pkt_km.type = FRAME_TYPE_KEYBOARD;
			}
		}
		else{
			 pkt_km.type = FRAME_TYPE_KEYBOARD;
		}
#endif

		if (km_dat_sending) {
            if ( kb_tx_retry_thresh-- == 0 ){
				km_dat_sending = 0;
            }
		}
	}
	else{
		pkt_pairing.seq_no++;
	}

	if((km_dat_sending || !kb_status.link_ok)){
		if(device_send_packet ( kb_rf_pkt, 550, kb_status.tx_retry, 0) ){
			km_dat_sending = 0;
			kb_status.no_ack = 0;
		}
		else{
			kb_status.no_ack ++;
			pkt_km.per ++;
		}
	}
}

void pm_2p4g_proc(void){
	if (!key_2p4g_not_released && clock_time_exceed (tick_key_pressed, 10* 1000000)){
		if(kb_status.link_ok){
			u32 did;
			did = kb_status.dongle_pipeCode;
			u8 * pd = (u8 *) &did;
			for (int i = PM_REG_DONGLE_PIPECODE_LOW; i <= PM_REG_DONGLE_PIPECODE_HIGH; i++) {
				analog_write (i, *pd ++);
			}
			analog_write (PM_REG_MODE_LINK, kb_status.link_ok);
		}
		cpu_sleep_wakeup (DEEPSLEEP_MODE, PM_WAKEUP_PAD , 0) ;  //deep
	}
	else
	{
		cpu_sleep_wakeup (SUSPEND_MODE, PM_WAKEUP_TIMER, cpu_wakup_last_tick + 12 * CLOCK_SYS_CLOCK_1MS) ;
	}
}

void main_loop_2p4g(void){
	cpu_wakup_last_tick = clock_time();
	kb_event.keycode[0] = 0;
	u32 det_key = kb_scan_key (0, !km_dat_sending);
	if (det_key){
		key_2p4g_not_released = 1;
		tick_key_pressed = clock_time();
		if(kb_event.cnt == 2){
			det_key = 0;
			if((kb_event.keycode[0] == VK_ENTER && kb_event.keycode[1] == VK_UP)
			|| (kb_event.keycode[0] == VK_UP && kb_event.keycode[1] == VK_ENTER) ){
				combination_key.key_ble_1m_ui_press_flg = 1;
			}
		}
		else{
			key_2p4g_not_released = 0;
			if(sys_status.sys_mode == SYS_2P4G_MODE ){
				if(combination_key.key_ble_1m_ui_press_flg){
					combination_key.key_ble_1m_ui_press_flg = 0;
					if(clock_time_exceed(combination_key.key_vaild_tick , 1000000)){
						sys_status.sys_ble_1m_chg_flg = 1;
					}
				}

				if(sys_status.sys_ble_1m_chg_flg){
					if(kb_status.link_ok){
						u32 did;
						did = kb_status.dongle_pipeCode;
						u8 * pd = (u8 *) &did;
						for (int i = PM_REG_DONGLE_PIPECODE_LOW; i <= PM_REG_DONGLE_PIPECODE_HIGH; i++) {
							analog_write (i, *pd ++);
						}
						analog_write (PM_REG_MODE_LINK, kb_status.link_ok);

					}
					return;
				}
			}
		}
	}

	if(kb_status.kb_mode != STATE_CONNECT  && kb_status.link_ok){
		kb_rf_pkt = (u8*)&pkt_km;
		kb_status.kb_mode = STATE_CONNECT;
		rf_set_power_level_index(kb_status.tx_power);
		memset(&kb_event,0,sizeof(kb_event));
		det_key = 1;
	}

	rf_2p4g_proc(det_key);

	pm_2p4g_proc();
}
