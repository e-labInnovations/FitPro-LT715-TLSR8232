/********************************************************************************************************
 * @file     ble.h 
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
 * @date     Sep. 18, 2015
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
/*
 * ble.h
 *
 *  Created on: 2018-5-25
 *      Author: Administrator
 */

#ifndef BLE_H_
#define BLE_H_


#include "blt_config.h"
#include "ble_common.h"
#include "l2cap.h"
#include "att.h"
#include "gap.h"
#include "ble_smp.h"
#include "uuid.h"
#include "ble_phy.h"

#include "crypt/aes_ccm.h"
#include "crypt/le_crypto.h"
#include "crypt/aes/aes_att.h"

#include "hci/hci.h"
#include "hci/hci_const.h"
#include "hci/hci_event.h"

#include "service/ble_ll_ota.h"
#include "service/device_information.h"
#include "service/hids.h"

#include "ll/ll.h"
#include "ll/ll_adv.h"
#include "ll/ll_encrypt.h"
#include "ll/ll_pm.h"
#include "ll/ll_slave.h"
#include "ll/ll_whitelist.h"
#include "ll/ll_scan.h"
#include "ll/ll_conn_phy.h"




#endif /* BLE_H_ */
