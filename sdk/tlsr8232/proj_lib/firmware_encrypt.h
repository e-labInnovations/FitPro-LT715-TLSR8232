/********************************************************************************************************
 * @file     firmware_encrypt.h
 *
 * @brief    This is the source file for TLSR8232
 *
 * @author	 BLE Group
 * @date     May 8, 2019
 *
 * @par      Copyright (c), Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *
 *           The information contained herein is confidential property of Telink
 *           Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *           of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *           Co., Ltd. and the licensee or the terms described here-in. This heading
 *           MUST NOT be removed from this file.
 *
 *           Licensees are granted free, non-transferable use of the information in this
 *           file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided.
 *
 *******************************************************************************************************/

#ifndef ENCRYPT_H_
#define ENCRYPT_H_

void firmware_encrypt_based_on_uid(unsigned char* uid,unsigned char* ciphertext);

#endif /* ENCRYPT_H_ */
