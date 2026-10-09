/********************************************************************************************************
 * @file     app.c
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
#include "app.h"
#include <stack/ble/ble.h>
#include "tl_common.h"
#include "drivers.h"
#include "../common/blt_led.h"
#include "../common/keyboard.h"
#include "../common/blt_soft_timer.h"
#include "../common/blt_common.h"


#if (__PROJECT_5316_HCI__ )

#define ADV_IDLE_ENTER_DEEP_TIME	60  //60 s
#define CONN_IDLE_ENTER_DEEP_TIME	60  //60 s

#define MY_DIRECT_ADV_TMIE			2000000

#define MY_APP_ADV_CHANNEL			BLT_ENABLE_ADV_ALL

#define MY_ADV_INTERVAL_MIN			ADV_INTERVAL_30MS
#define MY_ADV_INTERVAL_MAX			ADV_INTERVAL_35MS



#define		BLE_DEVICE_ADDRESS_TYPE 			BLE_DEVICE_ADDRESS_PUBLIC

own_addr_type_t 	app_own_address_type = OWN_ADDRESS_PUBLIC;



MYFIFO_INIT(blt_rxfifo, 64, 8);
MYFIFO_INIT(blt_txfifo, 40, 16);


//module spp Tx / Rx fifo
#define HCI_RXFIFO_SIZE		80
#define HCI_RXFIFO_NUM		4

#define HCI_TXFIFO_SIZE		80
#define HCI_TXFIFO_NUM		8

u8 		 	hci_rx_fifo_b[HCI_RXFIFO_SIZE * HCI_RXFIFO_NUM] = {0};
my_fifo_t	hci_rx_fifo = {
												HCI_RXFIFO_SIZE,
												HCI_RXFIFO_NUM,
												0,
												0,
												hci_rx_fifo_b,};

u8 		 	hci_tx_fifo_b[HCI_TXFIFO_SIZE * HCI_TXFIFO_NUM] = {0};
my_fifo_t	hci_tx_fifo = {
												HCI_TXFIFO_SIZE,
												HCI_TXFIFO_NUM,
												0,
												0,
												hci_tx_fifo_b,};
////ending

/* ADV Packet, SCAN Response Packet define */
const u8 tbl_advData[] = {
	 0x05, 0x09, 'G', 'h', 'i', 'd',
	 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
	 0x03, 0x19, 0x80, 0x01, 					// 384, Generic Remote Control, Generic category
	 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		// incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8 tbl_scanRsp [] = {
	0x08, 0x09, 'G', 'R', 'e', 'm', 'o', 't', 'e',
};


u32 tick_wakeup;
int	mcu_uart_working;			//depends on the wakeup scheme, attention to the use
int	module_uart_working;
int module_task_busy;

#define UART_TX_BUSY			( (hci_tx_fifo.rptr != hci_tx_fifo.wptr) || uart_tx_is_busy() )
#define UART_RX_BUSY			(hci_rx_fifo.rptr != hci_rx_fifo.wptr)



/* LED Management define */
enum{
	LED_POWER_ON = 0,
	LED_AUDIO_ON,	//1
	LED_AUDIO_OFF,	//2
	LED_SHINE_SLOW, //3
	LED_SHINE_FAST, //4
	LED_SHINE_OTA,  //5
};

const led_cfg_t led_cfg[] = {
    {1000,    0,      1,      0x00,	 },    //power-on, 1s on
	{100,	  0 ,	  0xff,	  0x02,  },    //audio on, long on
	{0,	      100 ,   0xff,	  0x02,  },    //audio off, long off
	{500,	  500 ,   2,	  0x04,	 },    //1Hz for 3 seconds
	{250,	  250 ,   4,	  0x04,  },    //2Hz for 3 seconds
	{250,	  250 ,   200,	  0x08,  },    //2Hz for 50 seconds
};

int app_module_busy ()
{
	mcu_uart_working = gpio_read(GPIO_WAKEUP_MODULE);  //mcu use GPIO_WAKEUP_MODULE to indicate the UART data transmission or receiving state
	module_uart_working = UART_TX_BUSY || UART_RX_BUSY; //module checks to see if UART rx and tX are all processed
	module_task_busy = mcu_uart_working || module_uart_working;
	return module_task_busy;
}

void app_suspend_exit ()
{
	GPIO_WAKEUP_MODULE_HIGH;  //module enter working state
	bls_pm_setSuspendMask(SUSPEND_DISABLE);
	tick_wakeup = clock_time () | 1;
}

int app_suspend_enter ()
{
	if (app_module_busy ())
	{
		app_suspend_exit ();
		return 0;
	}
	return 1;
}

void app_power_management ()
{
#if (BLE_HCI_PM_ENABLE)

	if (!app_module_busy() && !tick_wakeup)
	{

		bls_pm_setSuspendMask(SUSPEND_ADV | SUSPEND_CONN);

		bls_pm_setWakeupSource(PM_WAKEUP_PAD);  // GPIO_WAKEUP_MODULE needs to be wakened
	}

	if (tick_wakeup && clock_time_exceed (tick_wakeup, 500))
	{
		GPIO_WAKEUP_MODULE_LOW;
		tick_wakeup = 0;
	}

#endif
}


/////////////////////////////////////blc_register_hci_handler for spp////////////////////////////
int rx_from_uart_cb (void)//UART data send to Master,we will handler the data as CMD or DATA
{
	u8 *p = my_fifo_get(&hci_rx_fifo);
	if(p == NULL){
		return 0;
	}

	//u8* p = my_fifo_get(&hci_rx_fifo);
	u32 rx_len = p[0]; //usually <= 255 so 1 byte should be sufficient

	if (rx_len)
	{
		blc_hci_handler(&p[4], rx_len - 4);//
		*((u32*)p) = 0;//Clear DMA length field for distributing HCI Rx buffer of interrupt
		my_fifo_pop(&hci_rx_fifo);
	}

	return 0;
}

uart_data_t T_txdata_buf;
int tx_to_uart_cb (void)
{
	u8 *p = my_fifo_get (&hci_tx_fifo);
	if (p && !uart_tx_is_busy ())
	{
		memcpy(&T_txdata_buf.data, p + 2, p[0]+p[1]*256);
		T_txdata_buf.len = p[0]+p[1]*256 ;

		if (uart_dma_send((u16 *)(&T_txdata_buf)))
		{
			my_fifo_pop (&hci_tx_fifo);
		}
	}
	return 0;
}



void user_init()
{
	/*-- BLE stack initialization --------------------------------------------*/
	u8  mac_public[6];
	u8  mac_random_static[6];
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

	#if(BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_PUBLIC)
		app_own_address_type = OWN_ADDRESS_PUBLIC;
	#elif(BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_RANDOM_STATIC)
		app_own_address_type = OWN_ADDRESS_RANDOM;
		blc_ll_setRandomAddr(mac_random_static);
	#endif

	/*-- BLE Controller initialization ---------------------------------------*/
	blc_ll_initBasicMCU(mac_public);//mandatory
	blc_ll_initAdvertising_module(mac_public);//adv module: mandatory for BLE slave,
	blc_ll_initSlaveRole_module();//slave module: mandatory for BLE slave,

	//L2CAP initialization
	blc_l2cap_register_handler(blc_hci_sendACLData2Host);///  blc_l2cap_packet_receive
	blc_hci_registerControllerEventHandler(blc_hci_send_data);

	///////////////////// USER application initialization ///////////////////
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam(  MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, app_own_address_type,
									 0,  NULL,
									 MY_APP_ADV_CHANNEL,
									 ADV_FP_NONE);
	//debug: ADV setting err
	if(status != BLE_SUCCESS) { write_reg8(0x8000, 0x11); 	while(1); }

	bls_ll_setAdvEnable(0);  //adv disable

	rf_set_power_level_index (RF_POWER_7P9dBm);//OK

	////////////////// SPP initialization ///////////////////////////////////

	//note: dma addr must be set first before any other uart initialization! (confirmed by sihui)
	uart_set_recbuff( (unsigned short *)hci_rx_fifo_b, hci_rx_fifo.size);
	uart_set_pin(UART_TX_PB4, UART_RX_PB5);// uart tx/rx pin set   UART_TX_PA3, UART_RX_PA4
	uart_reset();  //will reset uart digital registers from 0x90 ~ 0x9f, so uart setting must set after this reset

	///it is only for 16000000 system clock. other baud rate setting, need to user lua tool to calculate.
	uart_init_baudrate(9, 13,PARITY_NONE, STOP_BIT_ONE); ///115200

	uart_dma_en(1, 1); 	//uart data in hardware buffer moved by dma, so we need enable them first
	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);   	//uart Rx/Tx dma irq enable
	uart_irq_en(0, 0);  	//uart Rx/Tx irq no need, disable them

	blc_register_hci_handler(rx_from_uart_cb, tx_to_uart_cb);

	/* Power Management initialization */
#if (BLE_HCI_PM_ENABLE)
	blc_ll_initPowerManagement_module();        //pm module:      	 optional

	bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);

	//mcu can wake up module from suspend or deepsleep by pulling up GPIO_WAKEUP_MODULE
	cpu_set_gpio_wakeup (GPIO_WAKEUP_MODULE, GPIO_Level_High, 1);  // pad high wakeup deepsleep

	GPIO_WAKEUP_MODULE_LOW;

	bls_pm_registerFuncBeforeSuspend( &app_suspend_enter );
#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);
#endif
	/////
}

/*----------------------------------------------------------------------------*/
/*--------- Main Loop                                             ------------*/
/*----------------------------------------------------------------------------*/
u32 tick_loop;
void main_loop (void)
{
	tick_loop ++;

	/* BLE entry -------------------------------------------------------------*/
	blt_sdk_main_loop();

	/* HCI power management --------------------------------------------------*/
	app_power_management();
}
#endif  //end of__PROJECT_5316_BLE_REMOTE__

