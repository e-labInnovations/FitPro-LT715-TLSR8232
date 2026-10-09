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
#include "rf_2p4g.h"
#include "battery_check.h"
#include "rc_ir.h"


#if (__PROJECT_5316_DUAL_MODE__)

systemStatus_t sys_status = {
		0,
		0,
		0,
		SYS_2P4G_MODE,
};

#define RC_DEEP_SLEEP_EN            1
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

u32 interval_update_tick = 0;
int device_in_connection_state;

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

u32	advertise_begin_tick;


unsigned int lowBattDet_tick = 0;

int ui_mtu_size_exchange_req = 0;

/* Key type Macro */
#define IDLE_KEY	   			0
#define CONSUMER_KEY   	   		1
#define KEYBOARD_KEY   	   		2
#define IR_KEY   	   			3

u8 	key_type;
u8 	user_key_mode;

u8 	key_buf[8] = {0};

int key_not_released;

int ir_not_released;

u32 latest_user_event_tick;

u8 	user_task_flg;
u8 	sendTerminate_before_enterDeep = 0;
u8 	ota_is_working = 0;

/* User Consumer Key Map */
static u16 vk_consumer_map[16] = {
	MKEY_VOL_UP,
	MKEY_VOL_DN,
	MKEY_MUTE,
	MKEY_CHN_UP,

	MKEY_CHN_DN,
	MKEY_POWER,
	MKEY_AC_SEARCH,
	MKEY_RECORD,

	MKEY_PLAY,
	MKEY_PAUSE,
	MKEY_STOP,
	MKEY_FAST_FORWARD,  //can not find fast_backword in <<HID Usage Tables>>

	MKEY_FAST_FORWARD,
	MKEY_AC_HOME,
	MKEY_AC_BACK,
	MKEY_MENU,
};

#if (STUCK_KEY_PROCESS_ENABLE)
	u32 stuckKey_keyPressTime;
#endif

/*----------------------------------------------------------------------------*/
/*------------- IR  Function                                  ----------------*/
/*----------------------------------------------------------------------------*/
#if (REMOTE_IR_ENABLE)
//ir key
#define TYPE_IR_SEND			1
#define TYPE_IR_RELEASE			2

///////////////////// key mode //////////////////////
#define KEY_MODE_BLE	   		0    //ble key
#define KEY_MODE_IR        		1    //ir  key

static const u8 kb_map_ble[] = KB_MAP_BLE;  //5*6
static const u8 kb_map_ir[]  = KB_MAP_IR;   //5*6

void ir_dispatch(u8 type, u8 syscode ,u8 ircode){
	if(type == TYPE_IR_SEND){
		ir_nec_send(syscode,~(syscode),ircode);

	}
	else if(type == TYPE_IR_RELEASE){
		ir_send_release();
	}
}
#endif


/*----------------------------------------------------------------------------*/
/*------------- OTA  Function                                 ----------------*/
/*----------------------------------------------------------------------------*/
#if (BLE_REMOTE_OTA_ENABLE)
void entry_ota_mode(void)
{
	ota_is_working = 1;
	device_led_setup(led_cfg[LED_SHINE_OTA]);
	bls_ota_setTimeout(15 * 1000 * 1000); //set OTA timeout  15 seconds
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




/*----------------------------------------------------------------------------*/
/*------------- CallBack function of BLE                      ----------------*/
/*----------------------------------------------------------------------------*/
void app_switch_to_indirect_adv(u8 e, u8 *p, int n)
{

	bls_ll_setAdvParam( MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
						ADV_TYPE_CONNECTABLE_UNDIRECTED, app_own_address_type,
						0,  NULL,
						MY_APP_ADV_CHANNEL,
						ADV_FP_NONE);

	bls_ll_setAdvEnable(1);  //must: set adv enable
}

void ble_remote_terminate(u8 e,u8 *p, int n) //*p is terminate reason
{
	device_in_connection_state = 0;

	if(*p == HCI_ERR_CONN_TIMEOUT){

	}else if(*p == HCI_ERR_REMOTE_USER_TERM_CONN){  //0x13

	}else if(*p == HCI_ERR_CONN_TERM_MIC_FAILURE){

	}else{

	}

#if (BLE_REMOTE_PM_ENABLE)
	 //user has push terminate pkt to ble TX buffer before deepsleep
	if(sendTerminate_before_enterDeep == 1){
		sendTerminate_before_enterDeep = 2;
	}
#endif


	advertise_begin_tick = clock_time();
}

void task_connect (u8 e, u8 *p, int n)
{
	bls_l2cap_requestConnParamUpdate (8, 8, 99, 400);  //interval=10ms latency=99 timeout=4s

	latest_user_event_tick = clock_time();

	ui_mtu_size_exchange_req = 1;

	device_in_connection_state = 1;//

	interval_update_tick = clock_time() | 1; //none zero
}

/*----------------------------------------------------------------------------*/
/*------------- Key Function                                  ----------------*/
/*----------------------------------------------------------------------------*/
#if(RC_BTN_ENABLE)
void deep_wakeup_proc(void)
{
#if(DEEPBACK_FAST_KEYSCAN_ENABLE)

#if (REMOTE_IR_ENABLE)
	if(KEY_MODE_IR == analog_read(DEEP_ANA_REG1)){
		return ;
	}
#endif
	//if deepsleep wakeup is wakeup by GPIO(key press), we must quickly scan this
	//press, hold this data to the cache, when connection established OK, send to master
	//deepsleep_wakeup_fast_keyscan
	if(analog_read(DEEP_ANA_REG0) == CONN_DEEP_FLG){
		if(kb_scan_key (KB_NUMLOCK_STATUS_POWERON, 1) && kb_event.cnt){
			deepback_key_state = DEEPBACK_KEY_CACHE;
			key_not_released = 1;
			memcpy(&kb_event_cache,&kb_event,sizeof(kb_event));
		}

		analog_write(DEEP_ANA_REG0, 0);
	}
#endif
}

void deepback_pre_proc(int *det_key)
{
#if (DEEPBACK_FAST_KEYSCAN_ENABLE)
	// to handle deepback key cache
	if(!(*det_key) && deepback_key_state == DEEPBACK_KEY_CACHE
			&& blc_ll_getCurrentState() == BLS_LINK_STATE_CONN
			&& clock_time_exceed(bls_ll_getConnectionCreateTime(), 25000))
	{
		memcpy(&kb_event,&kb_event_cache,sizeof(kb_event));
		*det_key = 1;

		if(key_not_released || kb_event_cache.keycode[0] == VOICE){  //no need manual release
			deepback_key_state = DEEPBACK_KEY_IDLE;
		}
		else{  //need manual release
			deepback_key_tick = clock_time();
			deepback_key_state = DEEPBACK_KEY_WAIT_RELEASE;
		}
	}
#endif
}

void deepback_post_proc(void)
{
#if (DEEPBACK_FAST_KEYSCAN_ENABLE)
	//manual key release
	if(deepback_key_state == DEEPBACK_KEY_WAIT_RELEASE && clock_time_exceed(deepback_key_tick,150000)){
		key_not_released = 0;

		key_buf[2] = 0;
		bls_att_pushNotifyData (HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8); //release
		deepback_key_state = DEEPBACK_KEY_IDLE;
	}
#endif
}

void key_change_proc(void)
{
	latest_user_event_tick = clock_time();  //record latest key change time

	u8 key0 = kb_event.keycode[0];
	//u8 key1 = kb_event.keycode[1];
	u8 key_value;

	key_not_released = 1;
	if (kb_event.cnt == 2)   //two key press, do  not process
	{
#if KB_MODE_EXCHANGE
		if((kb_event.keycode[0] == VK_ENTER && kb_event.keycode[1] == VK_RIGHT)
		 ||(kb_event.keycode[0] == VK_RIGHT && kb_event.keycode[1] == VK_ENTER) ){
			combination_key.key_2p4g_ui_press_flg = 1;
		}
#endif
	}
	else if(kb_event.cnt == 1)
	{
		if(key0 == KEY_MODE_SWITCH)
		{
			user_key_mode = !user_key_mode;
			device_led_setup(led_cfg[LED_SHINE_SLOW + user_key_mode]);
		}

#if (REMOTE_IR_ENABLE)
		else if(user_key_mode == KEY_MODE_BLE)
		{
			key_value = kb_map_ble[key0];
			if(key_value >= 0xf0 ){
				key_type = CONSUMER_KEY;
				u16 consumer_key = vk_consumer_map[key_value & 0x0f];
				bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
			}
			else
			{
				key_type = KEYBOARD_KEY;
				key_buf[2] = key_value;
				bls_att_pushNotifyData (HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8);
			}

		}
		else if(user_key_mode == KEY_MODE_IR)
		{  //IR mode
			key_value = kb_map_ir[key0];
			key_type = IR_KEY;
			if(!ir_not_released){
				ir_dispatch(TYPE_IR_SEND, 0x88, key_value);
				ir_not_released = 1;
			}
		}
		else
		{
			key_type = IDLE_KEY;
		}
#else
		else
		{
			key_value = key0;
			if(key_value >= 0xf0 ){
				key_type = CONSUMER_KEY;
				u16 consumer_key = vk_consumer_map[key_value & 0x0f];
				bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
			}
			else
			{
				key_type = KEYBOARD_KEY;
				key_buf[2] = key_value;
				bls_att_pushNotifyData (HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8);
			}
		}
#endif
	}
	else   //kb_event.cnt == 0,  key release
	{
		key_not_released = 0;
#if KB_MODE_EXCHANGE
		if(combination_key.key_2p4g_ui_press_flg){
			combination_key.key_2p4g_ui_press_flg = 0;
			if(clock_time_exceed(combination_key.key_vaild_tick , 1000000)){
				sys_status.sys_2p4g_mode_chg_flg = 1;
			}
		}
#endif
		if(key_type == CONSUMER_KEY)
		{
			u16 consumer_key = 0;
			bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
		}
		else if(key_type == KEYBOARD_KEY)
		{
			key_buf[2] = 0;
			bls_att_pushNotifyData (HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8); //release
		}
#if (REMOTE_IR_ENABLE)
		else if(key_type == IR_KEY)
		{
			if(ir_not_released){
				ir_not_released = 0;
				ir_dispatch(TYPE_IR_RELEASE, 0, 0);  //release
			}
		}
#endif
	}
}



#define GPIO_WAKEUP_KEYPROC_CNT				3

void proc_keyboard (u8 e, u8 *p, int n)
{
	static int gpioWakeup_keyProc_cnt = 0;
	static u32 keyScanTick = 0;
	//when key press gpio wakeup suspend, proc keyscan at least GPIO_WAKEUP_KEYPROC_CNT times
	//regardless of 8000 us interval
	if(e == BLT_EV_FLAG_GPIO_EARLY_WAKEUP){
		gpioWakeup_keyProc_cnt = GPIO_WAKEUP_KEYPROC_CNT;
	}
	else if(gpioWakeup_keyProc_cnt){
		gpioWakeup_keyProc_cnt --;
	}

	if(gpioWakeup_keyProc_cnt || clock_time_exceed(keyScanTick, 8000)){
		keyScanTick = clock_time();
	}
	else{
		return;
	}

	kb_event.keycode[0] = 0;
	int det_key = kb_scan_key (0, 1);

	#if(DEEPBACK_FAST_KEYSCAN_ENABLE)
		if(deepback_key_state != DEEPBACK_KEY_IDLE){
			deepback_pre_proc(&det_key);
		}
	#endif

		if (det_key){
			key_change_proc();
		}


	#if(DEEPBACK_FAST_KEYSCAN_ENABLE)
		if(deepback_key_state != DEEPBACK_KEY_IDLE){
			deepback_post_proc();
		}
	#endif
}
#endif

void sys_mode_chg_proc(void)
{
	if((sys_status.sys_mode == SYS_BLE_MODE_1M) && sys_status.sys_2p4g_mode_chg_flg){//switch to 2p4g mode
		u8 mode_chg = 0;

		if(sendTerminate_before_enterDeep == 2){  //Terminate OK
			mode_chg = 1;
			sendTerminate_before_enterDeep = 0;
			analog_write(DEEP_ANA_REG0, CONN_DEEP_FLG);
		}
		else{
			if(blc_ll_getCurrentState() == BLS_LINK_STATE_ADV){
				mode_chg = 1;
				bls_ll_setAdvEnable(0);   //disable adv
			}
			else if(blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){
				if(!sendTerminate_before_enterDeep){
					bls_ll_terminateConnection(HCI_ERR_REMOTE_USER_TERM_CONN); //push terminate cmd into ble TX buffer
					bls_ll_setAdvEnable(0);   //disable adv
					sendTerminate_before_enterDeep = 1;
				}
			}
			else if(blc_ll_getCurrentState() == BLS_LINK_STATE_IDLE){
				mode_chg = 1;
			}
		}

		if(mode_chg){
			#if (REMOTE_IR_ENABLE)
				analog_write(DEEP_ANA_REG1, user_key_mode);
			#endif
			sys_status.sys_2p4g_mode_chg_flg = 0;
			rf_ble_switch_phy(TLK_PRIVATE_2M);

			user_2p4g_init();
			analog_write (PM_REG_SYS_MODE, SYS_2P4G_MODE);
			sys_status.sys_mode = SYS_2P4G_MODE;
		}
	}
	else if((sys_status.sys_mode == SYS_2P4G_MODE) && sys_status.sys_ble_1m_chg_flg){// to ble mode
		sys_status.sys_ble_1m_chg_flg = 0;

		rf_ble_switch_phy(BLE_PHY_1M);
		user_ble_init ();

		analog_write(PM_REG_SYS_MODE,SYS_BLE_MODE_1M);
		sys_status.sys_mode = SYS_BLE_MODE_1M;
	}
}


extern u32	scan_pin_need;
//_attribute_ram_code_
void blt_pm_proc(void)
{
	if(sys_status.sys_2p4g_mode_chg_flg){
		return;
	}
	else {
	#if(BLE_REMOTE_PM_ENABLE)
		#if(REMOTE_IR_ENABLE)
			if( ir_send_ctrl.is_sending)
			{
				bls_pm_setSuspendMask(SUSPEND_DISABLE);
			}
			else
		#endif
		{
			bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);

			user_task_flg = ota_is_working || scan_pin_need || key_not_released || DEVICE_LED_BUSY;

			if(user_task_flg){
				#if (LONG_PRESS_KEY_POWER_OPTIMIZE)
					extern int key_matrix_same_as_last_cnt;
					if(!ota_is_working && key_matrix_same_as_last_cnt > 5){  //key matrix stable can optize
						bls_pm_setManualLatency(3);
					}
					else{
						bls_pm_setManualLatency(0);  //latency off: 0
					}
				#else
					bls_pm_setManualLatency(0);
				#endif
			}


		#if(RC_DEEP_SLEEP_EN) //deepsleep
			if(sendTerminate_before_enterDeep == 1){ //sending Terminate and wait for ack before enter deepsleep
				if(user_task_flg){  //detect key Press again,  can not enter deep now
					sendTerminate_before_enterDeep = 0;
					bls_ll_setAdvEnable(1);   //enable adv again
				}
			}
			else if(sendTerminate_before_enterDeep == 2){  //Terminate OK
				analog_write(DEEP_ANA_REG0, CONN_DEEP_FLG);

				#if (REMOTE_IR_ENABLE)
					analog_write(DEEP_ANA_REG1, user_key_mode);
				#endif
				cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
			}

			//adv 60s, deepsleep
			if( blc_ll_getCurrentState() == BLS_LINK_STATE_ADV && !sendTerminate_before_enterDeep && \
				clock_time_exceed(advertise_begin_tick , ADV_IDLE_ENTER_DEEP_TIME * 1000000))
			{

				#if (REMOTE_IR_ENABLE)
					analog_write(DEEP_ANA_REG1, user_key_mode);
				#endif
				cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
			}
			//conn 60s no event(key/voice/led), enter deepsleep
			else if( device_in_connection_state && !user_task_flg && \
					clock_time_exceed(latest_user_event_tick, CONN_IDLE_ENTER_DEEP_TIME * 1000000) )
			{
				bls_ll_terminateConnection(HCI_ERR_REMOTE_USER_TERM_CONN); //push terminate cmd into ble TX buffer
				bls_ll_setAdvEnable(0);   //disable adv
				sendTerminate_before_enterDeep = 1;
			}
		#endif
		}
	#endif  //END of  BLE_REMOTE_PM_ENABLE
	}
}

void  ble_remote_set_sleep_wakeup(u8 e, u8 *p, int n)
{                                                                                                                //3995*16     sys_tick_per_us
	if( blc_ll_getCurrentState() == BLS_LINK_STATE_CONN && ((u32)(bls_pm_getSystemWakeupTick() - clock_time())) > 80 *CLOCK_16M_SYS_TIMER_CLK_1MS ){  //suspend time > 30ms.add gpio wakeup
		bls_pm_setWakeupSource(PM_WAKEUP_CORE);  //gpio CORE wakeup suspend
	}
}


/////////////////////////////////////////////////
/////////////////////////////////////////////////
#if (BLT_TEST_SOFT_TIMER_ENABLE)

int gpio_test0(void)
{
	//gpio 0 toggle to see the effect
	DBG_CHN0_TOGGLE;

	return 0;
}

static u8 timer_change_flg = 0;
int gpio_test1(void)
{
	//gpio 1 toggle to see the effect
	DBG_CHN1_TOGGLE;


	timer_change_flg = !timer_change_flg;
	if(timer_change_flg){
		return 7000;
	}
	else{
		return 17000;
	}

}

int gpio_test2(void)
{
	//gpio 2 toggle to see the effect
	DBG_CHN2_TOGGLE;

	//timer last for 5 second
	if(clock_time_exceed(0, 5000000)){
		return -1;
		//blt_soft_timer_delete(&gpio_test2);
	}
	else{

	}

	return 0;
}

int gpio_test3(void)
{
	//gpio 3 toggle to see the effect
	DBG_CHN3_TOGGLE;

	return 0;
}

#endif
//////////////////////////////////////////////////////


void user_ble_init()
{
	blt_txfifo.wptr = 0;
	blt_txfifo.rptr = 0;
	blt_rxfifo.wptr = 0;
	blt_rxfifo.rptr = 0;

	/***********************************************************************************
	 * Keyboard matrix initialization. These section must be before battery_power_check.
	 * Because when low battery,chip will entry deep.if placed after battery_power_check,
	 * it is possible that can not wake up chip.
	 *  *******************************************************************************/
#if(RC_BTN_ENABLE)
	u32 pin[] = KB_DRIVE_PINS;
	for(int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
	{
		gpio_set_wakeup(pin[i],1,1);  	   //drive pin core(gpio) high wakeup suspend
		cpu_set_gpio_wakeup (pin[i],1,1);  //drive pin pad high wakeup deepsleep
	}

	bls_app_registerEventCallback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, &proc_keyboard);
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
	blc_ll_initBasicMCU(mac_public);//mandatory
	blc_ll_initAdvertising_module(mac_public);//adv module: mandatory for BLE slave,
	blc_ll_initSlaveRole_module();//slave module: mandatory for BLE slave,

	/*-- BLE Host initialization ---------------------------------------------*/
	extern void my_att_init(void);
	//GATT initialization
	my_att_init();
	//L2CAP initialization
	blc_l2cap_register_handler(blc_l2cap_packet_receive);

	/*-- BLE SMP initialization ----------------------------------------------*/
#if (BLE_REMOTE_SECURITY_ENABLE)
	blc_smp_param_setBondingDeviceMaxNumber(4);  	//default is SMP_BONDING_DEVICE_MAX_NUM, can not bigger that this value
													//and this func must call before bls_smp_enableParing
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER );
#else
	bls_smp_enableParing (SMP_PARING_DISABLE_TRRIGER );
#endif

	//HID_service_on_android7p0_init();  //hid device on android 7.0/7.1

	/*-- USER application initialization -------------------------------------*/
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));


	/* Configure ADV packet */
#if(BLE_REMOTE_SECURITY_ENABLE)
	//get bonded device number
	u8 bond_number = blc_smp_param_getCurrentBondingDeviceNumber();
	smp_param_save_t  bondInfo;
	if(bond_number)   //at least 1 bonding device exist
	{
		//get the latest bonding device (index: bond_number-1 )
		blc_smp_param_loadByIndex( bond_number - 1, &bondInfo);
	}

	if(bond_number)//set direct adv
	{
		//set direct adv
		u8 status = bls_ll_setAdvParam( MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
										ADV_TYPE_CONNECTABLE_DIRECTED_LOW_DUTY, app_own_address_type,
										bondInfo.peer_addr_type,  bondInfo.peer_addr,
										MY_APP_ADV_CHANNEL,
										ADV_FP_NONE);
		//debug: ADV setting err
		if(status != BLE_SUCCESS) { write_reg8(0x8000, 0x11); 	while(1); }

		//it is recommended that direct adv only last for several seconds, then switch to indirect adv
		bls_ll_setAdvDuration(MY_DIRECT_ADV_TMIE, 1);
		bls_app_registerEventCallback (BLT_EV_FLAG_ADV_DURATION_TIMEOUT, &app_switch_to_indirect_adv);
	}
	else//set indirect ADV
#endif
	{
		u8 status = bls_ll_setAdvParam(  MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
										 ADV_TYPE_CONNECTABLE_UNDIRECTED, app_own_address_type,
										 0,  NULL,
										 MY_APP_ADV_CHANNEL,
										 ADV_FP_NONE);
		//debug: ADV setting err
		if(status != BLE_SUCCESS) { write_reg8(0x8000, 0x11); 	while(1); }
	}

	bls_ll_setAdvEnable(1);  //adv enable
	rf_set_power_level_index (RF_POWER_7P9dBm);//OK

	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &ble_remote_terminate);


	/* Power Management initialization */
#if(BLE_REMOTE_PM_ENABLE)
	blc_ll_initPowerManagement_module();        //pm module:      	 optional
	bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_ENTER, &ble_remote_set_sleep_wakeup);
#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);
#endif


	/* IR Function initialization */
#if(REMOTE_IR_ENABLE)
	user_key_mode = analog_read(DEEP_ANA_REG1);
	analog_write(DEEP_ANA_REG1, 0x00);
	rc_ir_init();
#endif



	/* OTA Function Initialization  */
#if(BLE_REMOTE_OTA_ENABLE)
	bls_ota_clearNewFwDataArea(); //must
	bls_ota_registerStartCmdCb(entry_ota_mode);
	bls_ota_registerResultIndicateCb(LED_show_ota_result);
#endif


	/* LED Indicator Initialization */
#if (BLT_APP_LED_ENABLE)
	device_led_init(GPIO_LED, 1);
#endif

#if (BLT_TEST_SOFT_TIMER_ENABLE)
	blt_soft_timer_init();
	blt_soft_timer_add(&gpio_test0, 23000);//23ms
	blt_soft_timer_add(&gpio_test1, 7000); //7ms <-> 17ms
	blt_soft_timer_add(&gpio_test2, 13000);//13ms
	blt_soft_timer_add(&gpio_test3, 27000);//27ms
#endif

	advertise_begin_tick = clock_time();
}


void user_init(){
	if(sys_status.sys_mode == SYS_BLE_MODE_1M){//ble mode

		#if(RC_BTN_ENABLE)
			deep_wakeup_proc();
		#endif

		#if FIRMWARES_SIGNATURE_ENABLE
			blt_firmware_signature_check();
		#endif

		user_ble_init ();
	}
	else{									//2p4g mode
		user_2p4g_init();
	}
}

void main_loop_ble(void){

	/* BLE entry -------------------------------------------------------------*/
	blt_sdk_main_loop();

	/* UI entry --------------------------------------------------------------*/
	#if (BATT_CHECK_ENABLE)
	if(clock_time_exceed(lowBattDet_tick, 500*1000)){
		lowBattDet_tick = clock_time();
		battery_power_check(BATTERY_VOL_MIN);
	}
	#endif

	#if (BLT_TEST_SOFT_TIMER_ENABLE)
		blt_soft_timer_process(MAINLOOP_ENTRY);
	#endif

	#if(RC_BTN_ENABLE)
		proc_keyboard(0, 0, 0);
	#endif

	#if (BLT_APP_LED_ENABLE)
		device_led_process();
	#endif

	/*-- Power Management  -------------------------------------------------------*/
	blt_pm_proc();
}

/*----------------------------------------------------------------------------*/
/*--------- Main Loop                                             ------------*/
/*----------------------------------------------------------------------------*/
u32 tick_loop;
void main_loop (void)
{
	tick_loop ++;

	sys_mode_chg_proc();

	if(sys_status.sys_mode == SYS_BLE_MODE_1M){
		main_loop_ble();
	}
	else if(sys_status.sys_mode == SYS_2P4G_MODE){
		main_loop_2p4g();
	}
}
#endif  //end of__PROJECT_5316_BLE_REMOTE__

