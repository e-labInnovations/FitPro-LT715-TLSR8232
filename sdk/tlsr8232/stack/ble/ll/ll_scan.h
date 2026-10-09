/********************************************************************************************************
 * @file     ll_scan.h
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
#ifndef LL_SCAN_H_
#define LL_SCAN_H_

#include "config.h"
#include "stack/ble/ble_common.h"

#if((__TL_LIB_5316__ || MCU_CORE_TYPE == MCU_CORE_5316) && BLE_STATE_MACHINE_EXTENSION_EN)



/* Macro define */
#define BLC_SCAN_DISABLE				0
#define BLC_SCAN_ENABLE					1

//Scan filter policy
#define	FILTER_DUP_DISABLE				0
#define FILTER_DUP_ENABLE				1

//Extend BLE state machine
#define	BLS_FLAG_SCAN_IN_ADV_MODE		BIT(5)
#define	BLS_FLAG_SCAN_IN_SLAVE_MODE		BIT(6)

/* Type define */
typedef struct {
	u8	scan_en;
	u8	scan_type;
	u8	scan_filterPolicy;
	u8	filter_dup;

	u8	scanDevice_num;
	u8	scanRspDevice_num;

	u8	scan_extension_mask;
	s8	T_SCAN_REQ_INTVL;

	//u32   scan_interval;
}st_ll_scan_t;

typedef int (*ll_procScanPkt_callback_t)(u8 *, u8 *, u32);
typedef int (*ll_procScanDat_callback_t)(u8 *);
typedef void (*ll_switchScanChannel_t)(int, int);

/* External variable statement */
st_ll_scan_t  blts;
u32 blts_scan_interval;
extern rf_packet_scan_req_t	pkt_scan_req;


//extern ll_switchScanChannel_t     blc_ll_switchScanChannelCb;
extern ll_procScanPkt_callback_t  blc_ll_procScanPktCb;
extern ll_procScanDat_callback_t  blc_ll_procScanDatCb;

/* Function statement */
/*------------------ User Interface ------------------------------------------*/
void blc_ll_initScanning_module(u8 *public_adr);

/**
 * @Param: scan_type -> 0 for passive scan; 1 for active scan
 */
ble_sts_t blc_ll_setScanParameter(u8 scan_type, u16 scan_interval, u16 scan_window,
		                          u8 ownAddrType, u8 filter_policy);
//Scan in ADV state
ble_sts_t blc_ll_addScanningInAdvState(void);
ble_sts_t blc_ll_removeScanningFromAdvState(void);

//Scan in Connection state
ble_sts_t blc_ll_addScanningInConnSlaveRole(void);
ble_sts_t blc_ll_removeScanningFromConnSLaveRole(void);

/*---------- Stack Interface, user can not use!!! ----------------------------*/
int	 blc_ll_filterAdvDevice(u8 type, u8 * mac);
int  blc_ll_addScanRspDevice(u8 type, u8 *mac);
void blc_ll_clearScanRspDevice(void);

bool blc_ll_isScanRspReceived(u8 type, u8 *mac);

int blc_ll_procScanPkt(u8 *raw_pkt, u8 *new_pkt, u32 tick_now);
int blc_ll_procScanData(u8 *raw_pkt);

void blc_ll_switchScanChannel (int, int);

#endif//(__TL_LIB_5316__ || MCU_CORE_TYPE == MCU_CORE_5316)

#endif /* LL_SCAN_H_ */
