/********************************************************************************************************
 * @file     rf_ll.h
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
#ifndef RF_LL_H_
#define RF_LL_H_

u8	 get_next_channel_with_mask(u32 mask, u8 chn);
void rf_set_access_code_pairing(unsigned int code);
void rf_set_access_code_data(unsigned int code);
void rf_receiving_pipe_enble(u8 channel_mask);
void rf_send_packet (void* addr, unsigned short rx_waittime, unsigned char retry);

void irq_device_rx(void);
void irq_device_tx(void);
void ll_device_init (void);
void ll_add_clock_time (u32 ms);
int	device_send_packet (u8 * p, u32 timeout, int retry, int pairing_link);

extern int device_sync;
extern unsigned int cpu_wakup_last_tick;

#endif /* RF_LL_H_ */
