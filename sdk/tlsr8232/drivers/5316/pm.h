/********************************************************************************************************
 * @file     pm.h
 *
 * @brief    This is the header file for TLSR8232
 *
 * @author	 junwei.lu
 * @date     May 8, 2018
 *
 * @par      Copyright (c) 2018, Telink Semiconductor (Shanghai) Co., Ltd.
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

#pragma once

#include "../../common/types.h"
#include "bsp.h"
#include "compiler.h"
#include "analog.h"

#define XTAL_EXCEPTION_FIX_EN              1

#ifndef PM_TIM_RECOVER_MODE
#define PM_TIM_RECOVER_MODE					1
#endif


static inline void usb_dp_pullup_en (int en)
{
	unsigned char dat = ReadAnalogReg(0x08);
	if (en) {
		dat = (dat & 0x3f) | BIT(7);
	}
	else {
		dat = (dat & 0x3f) | BIT(6);
	}

	WriteAnalogReg (0x08, dat);
}



//#define SUSPEND_MODE	0
//#define DEEPSLEEP_MODE	1


//5316 analog register 0x34-0x3e can store infomation when MCU in deepsleep mode
//store your information in these ana_regs before deepsleep by calling analog_write function
//when MCU wakeup from deepsleep, read the information by by calling analog_read function

//these five below are stable
#define DEEP_ANA_REG0    0x3a
#define DEEP_ANA_REG1    0x3b
#define DEEP_ANA_REG2    0x3c
#define DEEP_ANA_REG3    0x3d

//these six below may have some problem when user enter deepsleep but ERR wakeup
// for example, when set a GPIO PAD high wakeup deepsleep, but this gpio is high before
// you call func cpu_sleep_wakeup, then deepsleep will be ERR wakeup, these 6 register
//   infomation loss.
#define DEEP_ANA_REG5    0x34
#define DEEP_ANA_REG6    0x35
#define DEEP_ANA_REG7    0x36
#define DEEP_ANA_REG8    0x37
#define DEEP_ANA_REG9    0x38
#define DEEP_ANA_REG10   0x39


#define ADV_DEEP_FLG	 0x01
#define CONN_DEEP_FLG	 0x02



#define SYS_DEEP_ANA_REG	0x3e  //ana_3e system use for external 32k mode, user can not use




#define 	BLT_RESET_WAKEUP_TIME_2000		1

#if (BLT_RESET_WAKEUP_TIME_2000)
	#define RESET_TIME_US	    	  2000//1500
	#define EARLYWAKEUP_TIME_US       2150//1620
	#define EMPTYRUN_TIME_US       	  2450//1920
#elif(BLT_RESET_WAKEUP_TIME_2200)
	#define RESET_TIME_US	    	  2200
	#define EARLYWAKEUP_TIME_US       2300
	#define EMPTYRUN_TIME_US       	  2600
#elif(BLT_RESET_WAKEUP_TIME_2400)
	#define RESET_TIME_US	    	  2400
	#define EARLYWAKEUP_TIME_US       2500
	#define EMPTYRUN_TIME_US       	  2800
#elif(BLT_RESET_WAKEUP_TIME_2600)
	#define RESET_TIME_US	    	  2600
	#define EARLYWAKEUP_TIME_US       2700
	#define EMPTYRUN_TIME_US       	  3000
#elif(BLT_RESET_WAKEUP_TIME_2800)
	#define RESET_TIME_US	    	  2800
	#define EARLYWAKEUP_TIME_US       2900
	#define EMPTYRUN_TIME_US       	  3200
#else
#endif

/* Sleep mode define */
typedef enum {
	PM_SleepMode_Suspend = 0x00,          SUSPEND_MODE	    = 0x00,
	PM_SLeepMode_Deep    = 0x01,          DEEPSLEEP_MODE	= 0x01,
}SleepMode_TypeDef;

/* Wakeup source define */
typedef enum {
	 PM_WAKEUP_PAD   = BIT(4),
	 PM_WAKEUP_CORE  = BIT(5),
	 PM_WAKEUP_TIMER = BIT(6),


	 PM_TIM_RECOVER_START =	BIT(14),
	 PM_TIM_RECOVER_END   =	BIT(15),
}SleepWakeupSrc_TypeDef;


//wakeup status from return value of "cpu_sleep_wakeup"
enum {
	 WAKEUP_STATUS_COMP   = BIT(0),  //wakeup by comparator
	 WAKEUP_STATUS_TIMER  = BIT(1),
	 WAKEUP_STATUS_CORE   = BIT(2),
	 WAKEUP_STATUS_PAD    = BIT(3),

	 STATUS_GPIO_ERR_NO_ENTER_PM  = BIT(7), //Abnormal wakeup

	 STATUS_ENTER_SUSPEND  = BIT(30),
}PM_WakeupStatusTypeDef;

#define 	WAKEUP_STATUS_TIMER_CORE	( WAKEUP_STATUS_TIMER | WAKEUP_STATUS_CORE)

typedef void (*pm_optimize_handler_t)(void);
typedef int (*suspend_handler_t)(void);
typedef int (*cpu_pm_handler_t)(SleepMode_TypeDef,  SleepWakeupSrc_TypeDef, unsigned int);
typedef void (*pCheck32kXtalStable)(void);
typedef unsigned int (*pGet32kXtalTickCb)(void);
typedef unsigned char (*pKick32kXtal)(int kickCnt);

extern cpu_pm_handler_t cpu_sleep_wakeup;
extern suspend_handler_t func_before_suspend;
extern unsigned char PM_PAD_FILTER_EN;
extern pCheck32kXtalStable check32kXtalStable;
extern pKick32kXtal pm_kick_32k_xtal_cb;

//void cpu_wakeup_init(int);
void cpu_wakeup_init(void);
_attribute_ram_code_ _attribute_no_inline_ void  sleep_start(void);

void pm_32kRcTrackDelayEnable(u32 timeUs, bool en);
void mcu_32k_rc_clock_calibration(void);
u32  pm_get_32k_tick(void); ////_attribute_no_inline_
void pm_set_32k_tick(u32 tick);///_attribute_no_inline_
unsigned int get_32k_xtal_tick(void);
unsigned int get_32k_xtal_set_value(void);

void bls_pm_registerFuncBeforeSuspend (suspend_handler_t func );
//int cpu_sleep_wakeup (int deepsleep, int wakeup_src, u32 wakeup_tick);
int cpu_sleep_wakeup_32krc (SleepMode_TypeDef sleep_mode,  SleepWakeupSrc_TypeDef wakeup_src, unsigned int  wakeup_tick);
int cpu_sleep_wakeup_32kpad (SleepMode_TypeDef sleep_mode,  SleepWakeupSrc_TypeDef wakeup_src, unsigned int  wakeup_tick);


/******************************* User Interface  ************************************/
void blc_pm_select_internal_32k_crystal(void);
void blc_pm_select_external_32k_crystal(void);
void blt_pm_ext32k_crystal_init(void);

void cpu_set_gpio_wakeup (int pin, int pol, int en);
void pm_set_filter(u8 en);


#if (PM_TIM_RECOVER_MODE)
	typedef struct{
		unsigned int   tick_sysClk;
		unsigned int   tick_32k;
		unsigned char  recover_flag;
	}pm_tim_recover_t;

	extern pm_tim_recover_t	pm_timRecover;
#endif

//void cpu_stall_wakeup_by_timer0(u32 tick_stall);
void cpu_stall_wakeup_by_timer1(u32 tick_stall);
void cpu_stall_wakeup_by_timer2(u32 tick_stall);



