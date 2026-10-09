/********************************************************************************************************
 * @file     feature_data_len_extension.c
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     May. 10, 2018
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

#include "app.h"
#include <stack/ble/ble.h>
#include "tl_common.h"
#include "drivers.h"
#include "app_config.h"
#include "vendor/common/blt_common.h"

#if (FEATURE_TEST_MODE == TEST_DATA_LENGTH_EXTENSION)

#define BLE_PM_ENABLE     0

#if (1)                  // support RF RX/TX MAX data Length: 251byte
#define RX_FIFO_SIZE 	288 //rx-28   max:251+28 = 279  16 align-> 288
#define RX_FIFO_NUM 	4

#define TX_FIFO_SIZE 	264 //tx-12   max:251+12 = 263  4 align-> 264
#define TX_FIFO_NUM 	4

#define MTU_SIZE_SETTING 			247
#define DLE_TX_SUPPORTED_DATA_LEN 	MAX_OCTETS_DATA_LEN_EXTENSION //264-12 = 252 > Tx max:251
#else
#define RX_FIFO_SIZE 	228 //rx-28   max:200+28 = 228  16 align-> 224
#define RX_FIFO_NUM 	4

#define TX_FIFO_SIZE 	212 //tx-12   max:200+12 = 212  4 align-> 212
#define TX_FIFO_NUM 	4

#define MTU_SIZE_SETTING 			196
#define DLE_TX_SUPPORTED_DATA_LEN 	(TX_FIFO_SIZE - 12)
#endif

u8 blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
my_fifo_t blt_rxfifo = {
    RX_FIFO_SIZE,
    RX_FIFO_NUM,
    0,
    0,
    blt_rxfifo_b,
};

u8 blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
my_fifo_t blt_txfifo = {
    TX_FIFO_SIZE,
    TX_FIFO_NUM,
    0,
    0,
    blt_txfifo_b,
};


u32 connect_event_occurTick = 0;
u32 mtuExchange_check_tick = 0;

int dle_started_flg = 0;

int mtuExchange_started_flg = 0;

u16 final_MTU_size = 23;

const u8 test_data[] = {
	7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,
};

int module_onReceiveData(rf_packet_att_write_t *p)
{
    u8 len = p->l2capLen - 3;
    if (len > 0)
    {
		printf("RF_RX len: %d\nc2s:write data: %d\n", p->rf_len, len);
		//array_printf(&p->value, len);
		printf("\n");

		if(BLE_SUCCESS == bls_att_pushNotifyData(SPP_SERVER_TO_CLIENT_DP_H, (u8*)&test_data[0], sizeof(test_data))){
			printf("NOTIFY OK\n");
		}
    }
    return 0;
}

void task_connect(u8 e, u8 *p, int n)
{
    printf("----- connected -----\n");
    connect_event_occurTick = clock_time() | 1;

    bls_l2cap_requestConnParamUpdate(8, 8, 19, 200); //interval=10ms latency=19 timeout=2s

    //MTU size reset to default 23 bytes every new connection, it can be only updated by MTU size exchange procedure
    final_MTU_size = 23;
}

void task_terminate(u8 e, u8 *p, int n)
{
    printf("----- terminate rsn: 0x%x -----\n", *p);
    connect_event_occurTick = 0;
    mtuExchange_check_tick = 0;

    //MTU size exchange and data length exchange procedure must be executed on every new connection,
    //so when connection terminate, relative flags must be cleared
    dle_started_flg = 0;
    mtuExchange_started_flg = 0;

    //MTU size reset to default 23 bytes when connection terminated
    final_MTU_size = 23;
}

void mtu_size_exchange_func(u16 connHandle, u16 remoteMtuSize, u16 effectMtuSize)
{
    final_MTU_size = effectMtuSize;
    mtuExchange_started_flg = 1;

    printf("------ MTU Size exchange ------\n");
    printf("remote MTU size: %d\n", remoteMtuSize);
    printf("local MTU size: %d\n",  MTU_SIZE_SETTING);
    printf("effect MTU size: %d\n", effectMtuSize);
}

void task_dle_exchange(u8 e, u8 *p, int n)
{
	dle_started_flg = 1;

	ll_data_extension_t* dle_param = (ll_data_extension_t*)p;
	printf("----- DLE exchange: -----\n");
	printf("connMaxRxOctets: %d\n", dle_param->connMaxRxOctets);
	printf("connMaxTxOctets: %d\n", dle_param->connMaxTxOctets);
	printf("connRemoteMaxRxOctets: %d\n", dle_param->connRemoteMaxRxOctets);
	printf("connRemoteMaxTxOctets: %d\n", dle_param->connRemoteMaxTxOctets);
	printf("connEffectiveMaxRxOctets: %d\n", dle_param->connEffectiveMaxRxOctets);
	printf("connEffectiveMaxTxOctets: %d\n", dle_param->connEffectiveMaxTxOctets);
}

void feature_sdle_test_init(void)
{
    u8 mac_public[6];
    u8 mac_random_static[6];
    blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);



    ////// Controller Initialization  //////////
    blc_ll_initBasicMCU(mac_public);           //mandatory
    blc_ll_initAdvertising_module(mac_public); //adv module: 		 mandatory for BLE slave,
    blc_ll_initSlaveRole_module();             //slave module: 	 mandatory for BLE slave,

    ////// Host Initialization  //////////
    my_att_init(); //GATT initialization

    //ATT initialization
    //If not set RX MTU size, default is: 23 bytes.  In this situation, if master send MtuSize Request before slave send MTU size request,
    //slave will response default RX MTU size 23 bytes, then master will not send long packet on host l2cap layer, link layer data length
    //extension feature can not be used.  So in data length extension application, RX MTU size must be enlarged when initialization.
    blc_att_setRxMtuSize(MTU_SIZE_SETTING);
    blc_att_registerMtuSizeExchangeCb(mtu_size_exchange_func);

    blc_l2cap_register_handler(blc_l2cap_packet_receive); //l2cap initialization

/*-- BLE SMP initialization ----------------------------------------------*/
#if (BLE_REMOTE_SECURITY_ENABLE)
    blc_smp_param_setBondingDeviceMaxNumber(4); //default is SMP_BONDING_DEVICE_MAX_NUM, can not bigger that this value
                                                //and this func must call before bls_smp_enableParing
    bls_smp_enableParing(SMP_PARING_CONN_TRRIGER);
#else
    bls_smp_enableParing(SMP_PARING_DISABLE_TRRIGER);
#endif

    ///////////////////// USER application initialization ///////////////////
    u8 tbl_advData[] = {
        0x08,
        0x09,'t','e','s','t','D','L','E',
    };
    u8 tbl_scanRsp[] = {
        0x08,
        0x09,'t','e','s','t','D','L','E',
    };
    bls_ll_setAdvData((u8 *)tbl_advData, sizeof(tbl_advData));
    bls_ll_setScanRspData((u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

    bls_ll_setAdvParam(ADV_INTERVAL_30MS, ADV_INTERVAL_30MS,
                       ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
                       0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);

    bls_ll_setAdvEnable(1); //adv enable
    rf_set_power_level_index(RF_POWER_7P9dBm);

    //ble event call back
    bls_app_registerEventCallback(BLT_EV_FLAG_CONNECT, &task_connect);
    bls_app_registerEventCallback(BLT_EV_FLAG_TERMINATE, &task_terminate);
    bls_app_registerEventCallback(BLT_EV_FLAG_DATA_LENGTH_EXCHANGE, &task_dle_exchange);

#if (BLE_PM_ENABLE)
    blc_ll_initPowerManagement_module();
    bls_pm_setSuspendMask(SUSPEND_ADV | SUSPEND_CONN);
    //bls_app_registerEventCallback(BLT_EV_FLAG_SUSPEND_EXIT, &func_suspend_exit);
#else
    bls_pm_setSuspendMask(SUSPEND_DISABLE);
#endif

    blc_att_setRxMtuSize(MTU_SIZE_SETTING);

    //printf debug
    uart_set_pin(UART_TX_PB4,UART_RX_PB5);
    uart_init_baudrate(9,13,PARITY_NONE, STOP_BIT_ONE);
}

void feature_sdle_test_mainloop(void)
{
    if(connect_event_occurTick && clock_time_exceed(connect_event_occurTick, 1500000))
    {   //1.5s after connection established
        connect_event_occurTick = 0;

        if(!mtuExchange_started_flg)
        {   //master do not send MTU exchange request in time
            blc_att_requestMtuSizeExchange(BLS_CONN_HANDLE, MTU_SIZE_SETTING);
            printf("After conn 1.5s, S send  MTU size req to the Master.\n");
        }

        mtuExchange_check_tick = clock_time() | 1;
    }

    if (mtuExchange_check_tick && clock_time_exceed(mtuExchange_check_tick, 500000))
    {
    	//2s after connection established
        mtuExchange_check_tick = 0;

        if(!dle_started_flg)
        {   //master do not send data length request in time
            printf("Master hasn't initiated the DLE yet, S send DLE req to the Master.\n");
            blc_ll_exchangeDataLength(LL_LENGTH_REQ, DLE_TX_SUPPORTED_DATA_LEN);
        }
    }

	#if(BLE_PM_ENABLE)
		bls_pm_setSuspendMask(SUSPEND_ADV | SUSPEND_CONN);
	#endif
}

#endif
