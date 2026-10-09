/********************************************************************************************************
 * @file     blt_fw_sign.c
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
 * @date     June. 12, 2019
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

#include "tl_common.h"
#include "drivers.h"
#include "blt_fw_sign.h"
#include "stack/ble/blt_config.h"
#include "proj_lib/firmware_encrypt.h"

#if FIRMWARES_SIGNATURE_ENABLE

void blt_firmware_signature_check(void)
{
		unsigned int flash_mid;
		unsigned char flash_uid[16];
		unsigned char signature_enc_key[16];
		int flag = flash_read_mid_uid_with_check(&flash_mid, flash_uid);

		if(flag==0){  //reading flash UID error
			while(1);
		}

		firmware_encrypt_based_on_uid (flash_uid, signature_enc_key);

		if(memcmp(signature_enc_key, (u8*)CUST_FIRMWARE_SIGNKEY_ADDR, 16)){  //signature not match
			while(1);   //user can change the code here to stop firmware running
		}
}

#endif
