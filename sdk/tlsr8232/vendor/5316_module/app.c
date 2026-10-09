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
#include "../common/blt_led.h"
#include "../common/keyboard.h"
#include "../common/blt_soft_timer.h"
#include "../common/blt_common.h"
#include "battery_check.h"
#include "spp.h"


#if (__PROJECT_5316_MODULE__ )

#define ADV_IDLE_ENTER_DEEP_TIME	60  //60 s
#define CONN_IDLE_ENTER_DEEP_TIME	60  //60 s

#define MY_DIRECT_ADV_TMIE			2000000
#define MY_APP_ADV_CHANNEL			BLT_ENABLE_ADV_ALL

#define MY_ADV_INTERVAL_MIN			ADV_INTERVAL_30MS
#define MY_ADV_INTERVAL_MAX			ADV_INTERVAL_35MS

#define	BLE_DEVICE_ADDRESS_TYPE 	BLE_DEVICE_ADDRESS_PUBLIC

own_addr_type_t 	app_own_address_type = OWN_ADDRESS_PUBLIC;



MYFIFO_INIT(blt_rxfifo, 64, 8);
MYFIFO_INIT(blt_txfifo, 40, 16);


//module spp Tx / Rx fifo
#define SPP_RXFIFO_SIZE		64 //72
#define SPP_RXFIFO_NUM		2

#define SPP_TXFIFO_SIZE		64 //72
#define SPP_TXFIFO_NUM		8

u8 		 	spp_rx_fifo_b[SPP_RXFIFO_SIZE * SPP_RXFIFO_NUM] = {0};
my_fifo_t	spp_rx_fifo = {
												SPP_RXFIFO_SIZE,
												SPP_RXFIFO_NUM,
												0,
												0,
												spp_rx_fifo_b,};

u8 		 	spp_tx_fifo_b[SPP_TXFIFO_SIZE * SPP_TXFIFO_NUM] = {0};
my_fifo_t	spp_tx_fifo = {
												SPP_TXFIFO_SIZE,
												SPP_TXFIFO_NUM,
												0,
												0,
												spp_tx_fifo_b,};
////ending

/* ADV Packet, SCAN Response Packet define */
const u8 tbl_advData[] = {
	 0x05, 0x09, 'G', 'M', 'o', 'd',
	 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
	 0x03, 0x19, 0x80, 0x01, 					// 384, Generic Remote Control, Generic category
	 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		// incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8 tbl_scanRsp [] = {
	0x08, 0x09, 'G', 'M', 'o', 'd', 'u', 'l', 'e',
};


u32 tick_wakeup;
int	mcu_uart_working;			//depends on the wakeup scheme, attention to the use
volatile int	module_uart_working;
int module_task_busy;


#define UART_TX_BUSY			( (spp_tx_fifo.rptr != spp_tx_fifo.wptr) || uart_tx_is_busy() )
#define UART_RX_BUSY			(spp_rx_fifo.rptr != spp_rx_fifo.wptr)


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


/*----------------------------------------------------------------------------*/
/*------------- OTA  Function                                 ----------------*/
/*----------------------------------------------------------------------------*/
#if (BLE_MODULE_OTA_ENABLE)
volatile u8 	ota_is_working = 0;
void entry_ota_mode(void)
{
	ota_is_working = 1;
	device_led_setup(led_cfg[LED_SHINE_OTA]);
	bls_ota_setTimeout(50 * 1000 * 1000); //set OTA timeout  15 seconds
}

void LED_show_ota_result(int result)
{
	#if 0
		irq_disable();
		WATCHDOG_DISABLE;

		gpio_set_output_en(GPIO_LED, 1);

		if(result == OTA_SUCCESS){  //OTA success
			gpio_write(GPIO_LED, 1);
			sleep_us(2000000);  //led on for 2 second
			gpio_write(GPIO_LED, 0);
		}
		else{  //OTA fail

		}

		gpio_set_output_en(GPIO_LED, 0);
	#endif
}
#endif


int	module_uart_send_flg;
u32 module_wakeup_mcu_tick;

int app_module_busy ()
{
	mcu_uart_working = gpio_read(GPIO_WAKEUP_MODULE);   // mcu use GPIO_WAKEUP_MODULE to indicate the UART data transmission or receiving state
	module_uart_working = UART_TX_BUSY || UART_RX_BUSY; // module checks to see if UART rx and tX are all processed
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

#if (BLE_MODULE_INDICATE_DATA_TO_MCU)
	module_uart_working = UART_TX_BUSY || UART_RX_BUSY;

	//When module's UART data is sent, the GPIO_WAKEUP_MCU is lowered or suspended (depending on how the user is designed)
	if(module_uart_send_flg && !module_uart_working){
		module_uart_send_flg = 0;
		module_wakeup_mcu_tick = 0;
		GPIO_WAKEUP_MCU_LOW;
	}
#endif

	// pullup GPIO_WAKEUP_MODULE: exit from suspend
	// pulldown GPIO_WAKEUP_MODULE: enter suspend

#if (BLE_MODULE_PM_ENABLE)

	if (!app_module_busy() && !tick_wakeup)
	{
		if(ota_is_working){
			return ;
		}
		//////////////////////////////
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

#define MY_MTU_SIZE      100
u8 mtu_size_exchanged = false;
void mtu_size_exchange_cb(u16 connHandle, u16 remoteMtuSize, u16 effectMtuSize)
{
	mtu_size_exchanged = true;
}

unsigned int lowBattDet_tick = 0;

void user_init()
{
	/***********************************************************************************
	 * Power Management initialization and gpio wake up source setting.
	 * Note: These section must be before battery_power_check.
	 * Because when low battery,chip will entry deep.if placed after battery_power_check,
	 * it is possible that can not wake up chip.(gpio wakeup setting will be lost in deep mode)
	 *  *******************************************************************************/
	#if (BLE_MODULE_PM_ENABLE)
		blc_ll_initPowerManagement_module();        //pm module:      	 optional

		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);

		gpio_set_wakeup(GPIO_WAKEUP_MODULE,1,1);  	   //drive pin core(gpio) high wakeup suspend
		//mcu can wake up module from suspend or deepsleep by pulling up GPIO_WAKEUP_MODULE
		cpu_set_gpio_wakeup(GPIO_WAKEUP_MODULE, GPIO_Level_High, 1);  // pad high wakeup deepsleep

		GPIO_WAKEUP_MODULE_LOW;

		bls_pm_registerFuncBeforeSuspend( &app_suspend_enter );
	#else
		bls_pm_setSuspendMask (SUSPEND_DISABLE);
	#endif

	/*****************************************************************************************
	 Note: battery check must do before any flash write/erase operation, cause flash write/erase
		   under a low or unstable power supply will lead to error flash operation

		   Some module initialization may involve flash write/erase, include: OTA initialization,
				SMP initialization, ..
				So these initialization must be done after  battery check
	*****************************************************************************************/
	#if(BATT_CHECK_ENABLE)
		if(analog_read(DEEP_ANA_REG2) == BATTERY_VOL_LOW){
			battery_power_check(BATTERY_VOL_MIN + 200);//2.2V
		}
		else{
			battery_power_check(BATTERY_VOL_MIN);//2.0 V
		}
	#endif

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
	blc_ll_initBasicMCU(mac_public);            //mandatory
	blc_ll_initAdvertising_module(mac_public);  //adv module:   mandatory for BLE slave,
	blc_ll_initSlaveRole_module();              //slave module: mandatory for BLE slave,


	////// Host Initialization  //////////
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization

	/*-- BLE SMP initialization ----------------------------------------------*/
#if (BLE_MODULE_SECURITY_ENABLE)
	blc_smp_param_setBondingDeviceMaxNumber(4);  	//default is SMP_BONDING_DEVICE_MAX_NUM, can not bigger that this value
													//and this func must call before bls_smp_enableParing
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER );
#else
	bls_smp_enableParing (SMP_PARING_DISABLE_TRRIGER );
#endif

//	HID_service_on_android7p0_init();  //hid device on android 7.0/7.1

	/*-- USER application initialization -------------------------------------*/
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam(  MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, app_own_address_type,
									 0,  NULL,
									 MY_APP_ADV_CHANNEL,
									 ADV_FP_NONE);
	//debug: ADV setting err
	if(status != BLE_SUCCESS) { write_reg8(0x8000, 0x11); 	while(1); }

	bls_ll_setAdvEnable(1);  //adv enable
	rf_set_power_level_index (RF_POWER_7P9dBm);

	//MTU Size
    blc_att_setRxMtuSize(MY_MTU_SIZE);
    blc_att_registerMtuSizeExchangeCb(mtu_size_exchange_cb);

	/*-- SPP initialization --------------------------------------------------*/
	//note: dma addr must be set first before any other uart initialization! (confirmed by sihui)
	uart_set_recbuff( (unsigned short *)spp_rx_fifo_b, spp_rx_fifo.size);
	uart_set_pin(UART_TX_PB4, UART_RX_PB5);  //UART TX/RX pin set
	uart_reset();                            //will reset UART digital registers from 0x90 ~ 0x9f, so UART setting must set after this reset

	#if (CLOCK_SYS_CLOCK_HZ == 16000000)
		uart_init_baudrate(9, 13,PARITY_NONE, STOP_BIT_ONE); //(9,13:115200;;)Baud rate's setting, please use LUA script tool to calculate.
	#elif (CLOCK_SYS_CLOCK_HZ == 32000000)
		uart_init_baudrate(30, 8,PARITY_NONE, STOP_BIT_ONE); //115200
	#elif (CLOCK_SYS_CLOCK_HZ == 48000000)
		uart_init_baudrate(25, 15,PARITY_NONE, STOP_BIT_ONE); //115200
	#endif

	uart_dma_en(1, 1); 	                                               //UART data in hardware buffer moved by DMA, so we enable them first
	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);  //UART RX/TX DMA irq enable
	uart_irq_en(0, 0);  	                                           //UART RX/TX irq no need, disable them

	extern int rx_from_uart_cb (void);
	extern int tx_to_uart_cb (void);
	blc_register_hci_handler(rx_from_uart_cb, tx_to_uart_cb);

	/////////////////
	extern int controller_event_handler(u32 h, u8 *para, int n);
	blc_hci_registerControllerEventHandler(controller_event_handler);		//register event callback
	bls_hci_mod_setEventMask_cmd(0xfffff);			//enable all 18 events,event list see ble_ll.h


#if (BLE_MODULE_OTA_ENABLE)
	// OTA init
	bls_ota_clearNewFwDataArea(); //must
	bls_ota_registerStartCmdCb(entry_ota_mode);
	bls_ota_registerResultIndicateCb(LED_show_ota_result);
#endif
}

/*----------------------------------------------------------------------------*/
/*--------- Main Loop                                             ------------*/
/*----------------------------------------------------------------------------*/
u32 tick_loop;
void main_loop (void)
{
	tick_loop ++;

	if(!mtu_size_exchanged && blc_ll_getCurrentState() == BLS_LINK_STATE_CONN
	   && clock_time_exceed(connected_start_tick, 5000*1000))
	{
		blc_att_requestMtuSizeExchange(BLS_CONN_HANDLE, MY_MTU_SIZE);
		mtu_size_exchanged = true;
		//write_reg32(0x8000, 0xaaaa5555);
	}

	/* BLE entry -------------------------------------------------------------*/
	blt_sdk_main_loop(); ///rx_from_uart_cb  tx_to_uart_cb

	/* UI entry --------------------------------------------------------------*/
	#if (BATT_CHECK_ENABLE)
	if(clock_time_exceed(lowBattDet_tick, 500*1000)){
		lowBattDet_tick = clock_time();
		battery_power_check(BATTERY_VOL_MIN);
	}
	#endif

	/* module power management -----------------------------------------------*/
	app_power_management();

	spp_restart_proc();
}
#endif  //end of__PROJECT_5316_BLE_REMOTE__

