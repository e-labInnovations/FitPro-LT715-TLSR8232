/********************************************************************************************************
 * @file     app.h
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
 * @date     May. 12, 2018
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
#ifndef _APP_H
#define _APP_H

#include "tl_common.h"
#include "drivers.h"

extern void user_init();
extern void main_loop (void);

extern void my_att_init(void);

void feature_adv_power_test_init(void);
void feature_adv_power_test_mainloop(void);

void feature_linklayer_state_test_init(void);
void feature_linklayer_state_test_main_loop(void);

void feature_phytest_init(void);

void feature_soft_timer_test_init(void);
void feature_soft_timer_test_mainloop(void);

//2m PHY test
extern void feature_2m_phy_conn_init(void);
extern void feature_2m_phy_conn_mainloop(void);

//DLE test
void feature_sdle_test_init(void);
void feature_sdle_test_mainloop(void);

void feature_whitelist_test_mainloop(void);

#endif /* APP_H_ */
