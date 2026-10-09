/********************************************************************************************************
 * @file     rf_2p4g.h
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
#ifndef RF_2P4G_H_
#define RF_2P4G_H_

#include "vendor/common/rf_frame.h"

#define		PM_REG_SYS_MODE					DEEP_ANA_REG3
#define		PM_REG_MODE_LINK				DEEP_ANA_REG9
#define		PM_REG_DONGLE_PIPECODE_LOW		DEEP_ANA_REG5
#define		PM_REG_DONGLE_PIPECODE_HIGH		DEEP_ANA_REG8

#define     FRAME_TYPE_KB_SEND_ID	  		0x0a
#define 	FRAME_AUTO_ACK_KB_ASK_ID  		0x8a

typedef struct {
	u8  link_ok;  				//0: not connect   1: connected
	u8  kb_pipe_rssi;
	u8  tx_power;
	u8  rsvd;

	u8  kb_mode;
	u8  tx_retry;
    u16 no_ack;

	u32 dongle_pipeCode;
} kb_status_t;

extern kb_status_t  kb_status;

typedef enum{
	STATE_PAIRING = 0,
	STATE_CONNECT,
}KB_MODE;

void user_2p4g_init();
int  rf_rx_process(u8 * p);
void rf_2p4g_proc( u32 det_key );
void pm_2p4g_proc(void);
void main_loop_2p4g(void);


#endif /* RF_2P4G_H_ */
