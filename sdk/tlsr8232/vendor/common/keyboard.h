/********************************************************************************************************
 * @file     keyboard.h
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

#pragma once

#include "drivers.h"

#define KB_RETURN_KEY_MAX	6

#define	KB_NUMLOCK_STATUS_INVALID			BIT(7)
#define	KB_NUMLOCK_STATUS_POWERON			BIT(15)

#define DEEPBACK_KEY_IDLE					0
#define DEEPBACK_KEY_CACHE					1   //there is deepback key cached in kb_event_cache
#define DEEPBACK_KEY_WAIT_RELEASE   		2


#ifndef		KB_MODE_EXCHANGE
#define		KB_MODE_EXCHANGE				0
#endif

#ifndef		KB_REPEAT_KEY_ENABLE
#define		KB_REPEAT_KEY_ENABLE			0
#endif

#define KEY_NONE	  	0
#define KEY_CHANGE  	1
#define KEY_SAME		2
typedef struct{
	unsigned char key_change_flg;
	unsigned char key_repeat_flg;
	unsigned char keycode0;
	unsigned char keycode1;
	unsigned int key_change_tick;
}repeatKey_t;


extern repeatKey_t repeat_key;

#if KB_MODE_EXCHANGE
typedef struct{
	unsigned char key_2p4g_ui_press_flg;
	unsigned char key_ble_1m_ui_press_flg;
	unsigned char key_ble_2m_ui_press_flg;
	unsigned char rsvd1;
	unsigned int key_vaild_tick;
}combinationKey_t;

extern combinationKey_t combination_key;

#endif

#ifndef		KEYSCAN_IRQ_TRIGGER_MODE
#define		KEYSCAN_IRQ_TRIGGER_MODE		0
#endif

typedef struct{
	unsigned char cnt;
	unsigned char ctrl_key;
	unsigned char keycode[KB_RETURN_KEY_MAX];
	//unsigned char padding[2];	//  for  32 bit padding,  if KB_RETURN_KEY_MAX change,  this should be changed
}kb_data_t;

extern kb_data_t	kb_event;
extern kb_data_t	kb_event_cache;
extern unsigned char deepback_key_state;
extern unsigned int deepback_key_tick;


#ifndef		LONG_PRESS_KEY_POWER_OPTIMIZE
#define		LONG_PRESS_KEY_POWER_OPTIMIZE		0
#endif


#ifndef		STUCK_KEY_PROCESS_ENABLE
#define		STUCK_KEY_PROCESS_ENABLE			0
#endif


#define 	SCAN_PIN_MAX_NUM_8					1
#define		SCAN_PIN_MAX_NUM_16					2
#define		SCAN_PIN_MAX_NUM_32					3

#ifndef     SCAN_PIN_SCALE
#define 	SCAN_PIN_SCALE   					SCAN_PIN_MAX_NUM_32
#endif


#if (SCAN_PIN_SCALE == SCAN_PIN_MAX_NUM_8)
	#define	  DTYPE_MATRIX      unsigned char
#else
	#define   DTYPE_MATRIX		unsigned int
#endif




int kb_is_data_same(kb_data_t *a, kb_data_t *b);

static inline int kb_is_key_valid(kb_data_t *p){
	return (p->cnt || p->ctrl_key);
}
static inline void kb_set_key_invalid(kb_data_t *p){
	p->cnt = p->ctrl_key = 0;
}


extern unsigned int kb_key_pressed(unsigned char * gpio);
extern unsigned int kb_scan_key_value (int numlock_status, int read_key,unsigned char * gpio);

extern unsigned int	scan_pin_need;


static inline unsigned int kb_scan_key (int numlock_status, int read_key) {
	unsigned char gpio[8];

#if(KEYSCAN_IRQ_TRIGGER_MODE)
	static unsigned char key_not_released = 0;

	if(numlock_status & KB_NUMLOCK_STATUS_POWERON){
		key_not_released = 1;
	}

	if(reg_irq_src & FLD_IRQ_GPIO_EN){  //FLD_IRQ_GPIO_RISC2_EN
		key_not_released = 1;
		reg_irq_src = FLD_IRQ_GPIO_EN;  //FLD_IRQ_GPIO_RISC2_EN
	}
	else{  //no key press
		if(!key_not_released && !(numlock_status & KB_NUMLOCK_STATUS_POWERON)){
			return 0;
		}
	}
#endif

	scan_pin_need = kb_key_pressed (gpio);
	if(scan_pin_need){
		return  kb_scan_key_value(numlock_status,read_key,gpio);
	}
	else{
#if (KB_REPEAT_KEY_ENABLE)
		repeat_key.key_change_flg = KEY_NONE;
#endif
#if (KEYSCAN_IRQ_TRIGGER_MODE)
		key_not_released = 0;
#endif
		return 0;
	}
}
