/********************************************************************************************************
 * @file     ll.h 
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
#ifndef LL__H_
#define LL__H_


//#include <stack/ble/att.h>
#include <stack/ble/ble_common.h>
#include <stack/ble/blt_config.h>
#include <stack/ble/gap.h>
#include <stack/ble/hci/hci_const.h>
#include <stack/ble/hci/hci_event.h>
#include <stack/ble/l2cap.h>
#include <stack/ble/ll/ll_adv.h>
#include <stack/ble/ll/ll_encrypt.h>
#include <stack/ble/ll/ll_pm.h>
#include <stack/ble/ll/ll_slave.h>
#include <stack/ble/ll/ll_whitelist.h>
#include <stack/ble/uuid.h>
#include "tl_common.h"
#include "drivers.h"


//#include "../../../proj/drivers/rf_pa.h"

extern u8 blt_state;

/////////////////////////////////////////////////////////////////////////////
#define CLOCK_SYS_CLOCK_1250US 	(1250 * sys_tick_per_us)
#define CLOCK_SYS_CLOCK_10MS 	(10000 * sys_tick_per_us)
#define FLG_RF_CONN_DONE 		(FLD_RF_IRQ_CMD_DONE | FLD_RF_IRQ_FSM_TIMEOUT | FLD_RF_IRQ_FIRST_TIMEOUT | FLD_RF_IRQ_RX_TIMEOUT | FLD_RF_IRQ_RX_CRC_2)


/////////////////////////////////////////////////////////////////////////////

#define LL_ROLE_MASTER 		0
#define LL_ROLE_SLAVE 		1

#define BLM_CONN_HANDLE 	BIT(7)
#define BLS_CONN_HANDLE 	BIT(6)

#define HANDLE_STK_FLAG 	BIT(15)

//LL_DATA_PDU header LLID field
#define LLID_RESERVED				 0x00
#define LLID_DATA_CONTINUE			 0x01
#define LLID_DATA_START				 0x02
#define LLID_CONTROL				 0x03

/*-------------------Start of BLE Controller feature -------------------------*/
#define LL_CONNECTION_UPDATE_REQ 	 0x00
#define LL_CHANNEL_MAP_REQ     		 0x01
#define LL_TERMINATE_IND 			 0x02

#define LL_ENC_REQ 					 0x03
#define LL_ENC_RSP 					 0x04
#define LL_START_ENC_REQ 			 0x05
#define LL_START_ENC_RSP 			 0x06

#define LL_UNKNOWN_RSP 				 0x07
#define LL_FEATURE_REQ 				 0x08
#define LL_FEATURE_RSP 				 0x09

#define LL_PAUSE_ENC_REQ 			 0x0A
#define LL_PAUSE_ENC_RSP		     0x0B

#define LL_VERSION_IND 				 0x0C
#define LL_REJECT_IND 				 0x0D
#define LL_SLAVE_FEATURE_REQ         0x0E
#define LL_CONNECTION_PARAM_REQ      0x0F
#define LL_CONNECTION_PARAM_RSP      0x10
#define LL_REJECT_IND_EXT  			 0x11
#define LL_PING_REQ 				 0x12
#define LL_PING_RSP 				 0x13
#define LL_LENGTH_REQ 				 0x14 //Core 4.2
#define LL_LENGTH_RSP 				 0x15 //Core 4.2

#define LL_PHY_REQ 			  		 0x16 //Core 5.0
#define LL_PHY_RSP 					 0x17 //Core 5.0
#define LL_PHY_UPDATE_IND 			 0x18 //Core 5.0

#define LL_MIN_USED_CHANNELS_IND 	 0x19 //Core 5.0
/*-------------------End of BLE Controller feature ---------------------------*/

/*-------------------- Tx settle time ----------------------------------------*/
//#if(CLOCK_SYS_CLOCK_HZ == 48000000)
//	#define	LL_ADV_TX_SETTLE_1M		   (87)//tested
//#elif(CLOCK_SYS_CLOCK_HZ == 32000000)
//	#define	LL_ADV_TX_SETTLE_1M		   (85)
//#else
//	#define	LL_ADV_TX_SETTLE_1M		   (80)//tested
//#endif

//#define	LL_ADV_TX_SETTLE_1M		    (80)//tested
//#define LL_SCAN_TX_SETTLE_1M	   		(80)
#define LL_SLAVE_TX_SETTLE_1M	   		(87)//tested

//#define	LL_ADV_TX_SETTLE_2M		    (80)
//#define LL_SCAN_TX_SETTLE_2M	   		(80)
#define LL_SLAVE_TX_SETTLE_2M	   		(89)//tested. 0x402 = 0x26 -> tx settle = 113
/*----------------------------------------------------------------------------*/

#define SLAVE_LL_ENC_OFF 			0
#define SLAVE_LL_ENC_REQ 			1
#define SLAVE_LL_ENC_RSP_T 			2
#define SLAVE_LL_ENC_START_REQ_T 	3
#define SLAVE_LL_ENC_START_RSP 		4
#define SLAVE_LL_ENC_START_RSP_T 	5
#define SLAVE_LL_ENC_PAUSE_REQ 		6
#define SLAVE_LL_ENC_PAUSE_RSP_T 	7
#define SLAVE_LL_ENC_PAUSE_RSP 		8
#define SLAVE_LL_REJECT_IND_T 		9

#define MASTER_LL_ENC_OFF 			0
#define MASTER_LL_ENC_REQ 			1
#define MASTER_LL_ENC_RSP_T 		2
#define MASTER_LL_ENC_START_REQ_T 	3
#define MASTER_LL_ENC_START_RSP 	4
#define MASTER_LL_ENC_START_RSP_T 	5
#define MASTER_LL_ENC_PAUSE_REQ 	6
#define MASTER_LL_ENC_PAUSE_RSP_T 	7
#define MASTER_LL_ENC_PAUSE_RSP 	8
#define MASTER_LL_REJECT_IND_T 		9
#define MASTER_LL_ENC_SMP_INFO_S 	10
#define MASTER_LL_ENC_SMP_INFO_E 	11

//ble link layer state
#define BLS_LINK_STATE_IDLE 	0
#define BLS_LINK_STATE_ADV 		BIT(0)
#define BLS_LINK_STATE_SCAN 	BIT(1)
#define BLS_LINK_STATE_INIT 	BIT(2)
#define BLS_LINK_STATE_CONN 	BIT(3)

//#define BLS_LINK_STATE_CONN_SLAVE		BIT(3)
//#define BLS_LINK_STATE_CONN_MASTER	BIT(4)

#define BLE_STATE_BTX_S 	4
#define BLE_STATE_BTX_E 	5
#define BLE_STATE_BRX_S 	6
#define BLE_STATE_BRX_E 	7

#define MAX_OCTETS_DATA_LEN_27 			27
#define MAX_OCTETS_DATA_LEN_EXTENSION 	251

#define LL_PACKET_OCTET_TIME(n) 		((n)*8 + 112)

#define DATA_LENGTH_REQ_PENDING 		1
#define DATA_LENGTH_REQ_DONE 			2


my_fifo_t blt_rxfifo;
u8 blt_rxfifo_b[];

my_fifo_t blt_txfifo;
u8 blt_txfifo_b[];
//////////////////////////////////////

typedef struct{
    u8 macAddress_public[6];
    u8 macAddress_random[6]; //host may set this
} ll_mac_t;

ll_mac_t bltMac;

typedef struct{
    u16 connEffectiveMaxRxOctets;
    u16 connEffectiveMaxTxOctets;
    u16 connMaxRxOctets;
    u16 connMaxTxOctets;
    u16 connRemoteMaxRxOctets;
    u16 connRemoteMaxTxOctets;
    u16 supportedMaxRxOctets;
    u16 supportedMaxTxOctets;

    u8 connInitialMaxTxOctets; //u8 is enough
    u8 connMaxTxRxOctets_req;
    u8 connRxDiff100;
    u8 connTxDiff100;
} ll_data_extension_t;

ll_data_extension_t bltData;

typedef struct{
	u8 llid;
	u8 rf_len;
	u8 opcode;
	u8 ctrlData;
}ll_unknown_rsp_t;

typedef struct{
    u8 adv_en;
    u8 adv_extension_mask;
    u8 adv_scanReq_connReq;
    u8 phy_en;

    u8 ll_recentAvgRSSI;
    u8 tx_irq_proc_en;
    u8 conn_rx_num; //slave: rx number in a new interval
    u8 is_adv_after_scan_req;

    u32 custom_access_code;

    u16	bluetooth_subver;
    u8  bluetooth_ver;
    u8	sdk_mainLoop_run_flag;   //Prevent enter sleep in first main_loop

} st_ll_conn_t;


st_ll_conn_t bltParam;





////////////////// Telink defined Event Callback  ////////////////////////
typedef void (*blt_event_callback_t)(u8 e, u8 *p, int n);

#define BLT_EV_MAX_NUM 		20

#define BLT_EV_FLAG_ADV 					0
#define BLT_EV_FLAG_ADV_DURATION_TIMEOUT 	1
#define BLT_EV_FLAG_SCAN_RSP 				2
#define BLT_EV_FLAG_CONNECT 				3
#define BLT_EV_FLAG_TERMINATE 				4
#define BLT_EV_FLAG_PAIRING_BEGIN 			5
#define BLT_EV_FLAG_PAIRING_END 			6//success or fail(with fail reason)
#define BLT_EV_FLAG_ENCRYPTION_CONN_DONE 	7
#define BLT_EV_FLAG_DATA_LENGTH_EXCHANGE 	8
#define BLT_EV_FLAG_GPIO_EARLY_WAKEUP 		9
#define BLT_EV_FLAG_CHN_MAP_REQ 			10
#define BLT_EV_FLAG_CONN_PARA_REQ 			11
#define BLT_EV_FLAG_CHN_MAP_UPDATE 			12
#define BLT_EV_FLAG_CONN_PARA_UPDATE	 	13
#define BLT_EV_FLAG_SUSPEND_ENTER 			14
#define BLT_EV_FLAG_SUSPEND_EXIT 			15
#define BLT_EV_FLAG_RX_DATA_ABANDOM 		16
#define BLT_EV_FLAG_SMP_PINCODE_PROCESS 	17
#define BLT_EV_FLAG_SMP_KEY_MISSING 		18 //add for UTB2
#define BLT_EV_FLAG_PHY_UPDATE              19

#define EVENT_MASK_ADV_DURATION_TIMEOUT 	BIT(BLT_EV_FLAG_ADV_DURATION_TIMEOUT)
#define EVENT_MASK_SCAN_RSP 				BIT(BLT_EV_FLAG_SCAN_RSP)
#define EVENT_MASK_CONNECT 					BIT(BLT_EV_FLAG_CONNECT)
#define EVENT_MASK_TERMINATE 				BIT(BLT_EV_FLAG_TERMINATE)
#define EVENT_MASK_CHN_MAP_REQ 				BIT(BLT_EV_FLAG_CHN_MAP_REQ)
#define EVENT_MASK_CONN_PARA_REQ 			BIT(BLT_EV_FLAG_CONN_PARA_REQ)
#define EVENT_MASK_CHN_MAP_UPDATE 			BIT(BLT_EV_FLAG_CHN_MAP_UPDATE)
#define EVENT_MASK_CONN_PARA_UPDATE 		BIT(BLT_EV_FLAG_CONN_PARA_UPDATE)
#define EVENT_MASK_RX_DATA_ABANDOM 			BIT(BLT_EV_FLAG_RX_DATA_ABANDOM)
#define EVENT_MASK_PHY_UPDATE 			    BIT(BLT_EV_FLAG_PHY_UPDATE)

typedef void (*ll_irq_tx_callback_t)(void);

typedef int (*ll_irq_rx_data_callback_t)(u8 *, u32);
typedef int (*ll_irq_rx_post_callback_t)(void);

typedef void (*ll_irq_systemTick_conn_callback_t)(void);

typedef int (*blc_main_loop_data_callback_t)(u8 *);
typedef int (*blc_main_loop_post_callback_t)(void);

typedef int (*blc_main_loop_phyTest_callback_t)(void);

typedef int (*blt_LTK_req_callback_t)(u16 handle, u8 *rand, u16 ediv);

extern my_fifo_t hci_tx_fifo;


/******************************* User Interface  ************************************/
void irq_blt_sdk_handler();

int blt_sdk_main_loop(void);

void blc_ll_initBasicMCU(u8 *public_adr);

ble_sts_t blc_ll_setRandomAddr(u8 *randomAddr);

ble_sts_t blc_ll_readBDAddr(u8 *addr);

u8 blc_ll_getCurrentState(void);

u8 blc_ll_getLatestAvgRSSI(void);

u16 blc_ll_setInitTxDataLength(u16 maxTxOct); //core4.2 long data packet

// application
void bls_app_registerEventCallback(u8 e, blt_event_callback_t p);

inline void blc_ll_setCustomizedAdvScanAccessCode(u32 access_code)
{
    bltParam.custom_access_code = access_code;
}
inline u8 blc_ll_get_connEffectiveMaxTxOctets(void)
{
	#if(LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
		return bltData.connEffectiveMaxTxOctets;
	#else
		return 27;
	#endif
}

/************************* Stack Interface, user can not use!!! ***************************/
ble_sts_t blc_hci_ltkRequestNegativeReply(u16 connHandle);
ble_sts_t blc_hci_ltkRequestReply(u16 connHandle, u8 *ltk);

void blc_ll_setEncryptionBusy(u8 enc_busy);
bool blc_ll_isEncryptionBusy(void);
void blc_ll_registerLtkReqEvtCb(blt_LTK_req_callback_t evtCbFunc);

void blc_ll_setIdleState(void);
ble_sts_t blc_hci_le_getLocalSupportedFeatures(u8 *features);

ble_sts_t blc_hci_le_readBufferSize_cmd(u8 *pData);

//core4.2 data extension
void blc_ll_initDataLengthExtension(void);
ble_sts_t blc_ll_exchangeDataLength(u8 opcode, u16 maxTxOct);
ble_sts_t blc_hci_setTxDataLength(u16 connHandle, u16 tx, u16 txtime);
ble_sts_t blc_hci_readSuggestedDefaultTxDataLength(u8 *tx, u8 *txtime);
ble_sts_t blc_hci_writeSuggestedDefaultTxDataLength(u16 tx, u16 txtime);

int blm_send_acl_to_btusb(u16 conn, u8 *p);
//为了节约时间必须放在RAM中,这是Hawk系列芯片特有的
void blt_adjust_timer0_capt(u32 sys_timer_tick);

static inline u8 blc_ll_getTxFifoNumber(void)
{
	u8 r = irq_disable();

	u8 fifo_num = ((reg_dma_tx_wptr - reg_dma_tx_rptr) & 15 )  +  ( (blt_txfifo.wptr - blt_txfifo.rptr) & 31 ) ;

	irq_restore(r);

	return  fifo_num;
}

static inline u8 blc_ll_getTxHardWareFifoNumber(void)
{
    return ((reg_dma_tx_wptr - reg_dma_tx_rptr) & 15);
}

static inline void blc_ll_resetInfoRSSI(void)
{
    bltParam.ll_recentAvgRSSI = 0;
}

static inline void blc_ll_recordRSSI(u8 rssi)
{
    if (bltParam.ll_recentAvgRSSI == 0)
    {
        bltParam.ll_recentAvgRSSI = rssi;
    }
    else
    {
        bltParam.ll_recentAvgRSSI = (bltParam.ll_recentAvgRSSI + rssi) >> 1;
    }
}

static inline u8* 	blc_ll_get_macAddrRandom(void)
{
	return bltMac.macAddress_random;
}

static inline u8* 	blc_ll_get_macAddrPublic(void)
{
	return bltMac.macAddress_public;
}

ble_sts_t blt_ll_unknown_rsp(u16 connHandle, u8 opcode);


/************************************************************* RF DMA RX\TX data strcut ***************************************************************************************
----RF RX DMA buffer struct----:
byte0    byte3   byte4    byte5   byte6  byte7  byte8    byte11   byte12      byte13   byte14 byte(14+w-1) byte(14+w) byte(16+w) byte(17+w) byte(18+w) byte(19+w)    byte(20+w)
*-------------*----------*------*------*------*----------------*----------*------------*------------------*--------------------*---------------------*------------------------*
| DMA_len(4B) | Rssi(1B) |xx(1B)|yy(1B)|zz(1B)| Stamp_time(4B) | type(1B) | Rf_len(1B) |    payload(wB)   |      CRC(3B)       |   Fre_offset(2B)    |  jj(1B)     0x40(1B)   |
|             | rssi-110 |                    |                |         Header        |     Payload      |                    | (Fre_offset/8+25)/2 |          if CRC OK:0x40|
|             |                               |                |<--               PDU                  -->|                    |                     |                        |
*-------------*-------------------------------*----------------*------------------------------------------*--------------------*---------------------*------------------------*
              |<------------------------------------------------------   DMA len  -----|------------------|--------------------|--------------------------------------------->|
                                                                                       |                  |                    |                                              |
              |<---------------------------------- 10 byte --------------------------->|<---- Rf_len ---->|<------ 3 Byte ---->|<------------------ 4 Byte ------------------>|
note: byte12 ->  type(1B):llid(2bit) nesn(1bit) sn(1bit) md(1bit)
we can see: DMA_len = w(Rf_len) + 17.
		    CRC_OK  = DMA_buffer[w(Rf_len) + 20] == 0x40 ? True : False.

----RF TX DMA buffer struct----:
byte0    byte3   byte4       byte5      byte6  byte(6+w-1)
*-------------*----------*------------*------------------*
| DMA_len(4B) | type(1B) | Rf_len(1B) |    payload(wB)   |
|             |         Header        |     Payload      |
|             |<--               PDU                  -->|
*-------------*------------------------------------------*
note: type(1B):llid(2bit) nesn(1bit) sn(1bit) md(1bit),实际向RF 硬件FIFO中压数据，type只表示llid,其他bit位为0！
*******************************************************************************************************************************************************************************/


#define	FIX_HW_CRC24_EN							1

typedef struct{
    u8 save_flg;
    u8 sn_nesn;
    u8 dma_tx_rptr;
} bb_sts_t;

bb_sts_t blt_bb;

static inline void	blt_save_snnesn (void)
{
	blt_bb.sn_nesn = ((REG_ADDR8(0xf22) & BIT(0)) << 4) | ((REG_ADDR8(0xf23) & BIT(4)) << 1);
}

static inline void	blt_restore_snnesn (void)
{
	reg_rst0 = FLD_RST0_ZB;
	reg_rst0 = 0;
	REG_ADDR8(0xf03) =  (REG_ADDR8(0xf03) & ~ BITS(4,5)) | blt_bb.sn_nesn;
}


static inline void	bls_save_dma_tx_rptr(void)
{
	//TX Fifo: 0x52a[0:3] means rptr
	blt_bb.dma_tx_rptr = reg_dma_tx_rptr & 0x0f;
}

static inline void	bls_restore_dma_tx_rptr(void)
{
	//0x52a[6] rptr set
	reg_dma_tx_rptr = ( BIT(6) | blt_bb.dma_tx_rptr);//restore tx_rptr
}

static inline void blt_ll_set_ble_access_code_adv(void)
{
	write_reg32(0x800408, bltParam.custom_access_code ? bltParam.custom_access_code : 0xd6be898e);
}

#endif /* LL__H_ */
