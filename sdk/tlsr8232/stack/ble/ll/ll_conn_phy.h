/********************************************************************************************************
 * @file     ll_conn_phy.h
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
#ifndef LL_CONN_PHY_H_
#define LL_CONN_PHY_H_

#if(LL_FEATURE_SUPPORT_LE_2M_PHY)

/* Type define */
typedef struct{
    u8 llid;
    u8 rf_len;
    u8 opcode;
    u8 tx_phys;
    u8 rx_phys;
} rf_pkt_ll_phy_req_rsp_t; //phy_req, phy_rsp

typedef struct{
    u8 llid;
    u8 rf_len;
    u8 opcode;
    u8 m_to_s_phy;
    u8 s_to_m_phy;
    u8 instant0;
    u8 instant1;
} rf_pkt_ll_phy_update_ind_t; //phy_req, phy_rsp

typedef struct{
    u8 dft_tx_prefer_phys;
    u8 dft_rx_prefer_phys;
    u8 dft_prefer_phy;
    u8 cur_llPhy;
} ll_phy_t;


//do not support Asymmetric PHYs, conn_phys = tx_phys & rx_phys
typedef struct{
    u8 conn_prefer_phys; // conn_prefer_phys = tx_prefer_phys & rx_prefer_phys
    u8 conn_cur_phy;     //
    u8 conn_next_phy;    //
    u8 phy_req_pending;
    //u8 phy_req_trigger;  // 1: means current device triggers phy_req, due to API "blc_ll_setPhy" called by Host or Application
} ll_conn_phy_t;


//extern u8 tx_settle_adv[];
extern u8 tx_settle_slave[];

typedef enum{
    BLE_PHY_1M    = BIT(0),
    BLE_PHY_2M    = BIT(1),
    TLK_PRIVATE_2M = BIT(5),
} le_phy_type_t;

typedef enum{
    PHY_PREFER_1M = BIT(0),
    PHY_PREFER_2M = BIT(1),
} le_phy_prefer_type_t;

typedef enum{
    PHY_TRX_PREFER    = 0,                 //has preference among TX & RX PHYs
    PHY_TX_NO_PREFER  = BIT(0),            //has no preference among TX PHYs
    PHY_RX_NO_PREFER  = BIT(1),            //has no preference among RX PHYs
    PHY_TRX_NO_PREFER = (BIT(0) | BIT(1)), //has no preference among TX & RX PHYs
} le_phy_prefer_mask_t;

/*-------------------------------- User Interface ----------------------------*/
void blc_ll_init2MPhy_feature(void);

ble_sts_t blc_ll_setDefaultPhy(le_phy_prefer_mask_t all_phys, le_phy_prefer_type_t tx_phys,
		                       le_phy_prefer_type_t rx_phys);
ble_sts_t blc_ll_setPhy(u16 connHandle, le_phy_prefer_mask_t all_phys,
                        le_phy_prefer_type_t tx_phys, le_phy_prefer_type_t rx_phys);


/*---------------- Stack Interface, user can not use!!! ----------------------*/
extern ll_phy_t bltPHYs;
extern ll_conn_phy_t blt_conn_phy;

typedef void (*ll_phy_switch_callback_t)(le_phy_type_t);
typedef void (*ll_conn_phy_update_callback_t)(void);

//extern ll_phy_switch_callback_t ll_phy_switch_cb;
//extern ll_conn_phy_update_callback_t ll_conn_phy_update_cb;

void blt_reset_conn_phy_param(void);

void blt_ll_sendPhyReq(void);
void blt_ll_updateConnPhy(void);


#endif /* LL_FEATURE_SUPPORT_LE_2M_PHY */

#endif /* LL_CONN_PHY_H_ */
