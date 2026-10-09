/********************************************************************************************************
 * @file     clock.h
 *
 * @brief    This is the header file for TLSR8232
 *
 * @author	 BLE Group
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

#include "driver_config.h"
#include "compiler.h"
#include "register.h"
#include "../../common/config/user_config.h"
#include "watchdog.h"

extern int SYS_TICK_DIV;

typedef enum{
	SYS_CLK_16M_Crystal = 0x43,
	SYS_CLK_32M_Crystal = 0x60,
	SYS_CLK_48M_Crystal = 0x20,
}SYS_CLK_TYPEDEF;



/* 5316 system clock source define. */
#define SYS_CLK_SRC_24M_RC              0
#define SYS_CLK_SRC_FHS                 1
#define SYS_CLK_SRC_FHS_DIV             2
#define SYS_CLK_SRC_FHS_2V3_DIV         3

/* 5316 FHS clock source define. */
//use for 0x66[7]
#define FHS_CLK_SRC_L_48M_OR_24M_PAD     0
#define FHS_CLK_SRC_L_24M_RC             1

//use for 0x70[0]
#define FHS_CLK_SRC_H_48M_OR_24M_RC      0
#define FHS_CLK_SRC_H_24M_PAD            1

//system timer clock source is constant 16M, never change (Use for software timer)
enum{
	CLOCK_16M_SYS_TIMER_CLK_1S =  16000000,
	CLOCK_16M_SYS_TIMER_CLK_1MS = 16000,
	CLOCK_16M_SYS_TIMER_CLK_1US = 16,
};
#define sys_tick_per_us		 16

enum{
	CLOCK_MODE_SCLK = 0,
	CLOCK_MODE_GPIO = 1,
	CLOCK_MODE_WIDTH_GPI = 2,
	CLOCK_MODE_TICK = 3
};

void clock_init(SYS_CLK_TYPEDEF SYS_CLK);

static inline unsigned int clock_time(void)
{
 	return reg_system_tick;
}

unsigned int clock_time_exceed(unsigned int ref, unsigned int span_us);

void pwm_kick_32k_pad(void);

void sleep_us (unsigned int microsec);		//  use register counter to delay

void MCU_24M_RC_ClockCalibrate(void);

/* Delay precisely -----------------------------------------------------------*/
#define WaitUs	     sleep_us
#define WaitMs(ms)	 sleep_us((ms)*1000)
#define sleep_ms(ms) sleep_us((ms)*1000)

#define _ASM_NOP_          asm("tnop")

#define	CLOCK_DLY_1_CYC    _ASM_NOP_
#define	CLOCK_DLY_2_CYC    _ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_3_CYC    _ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_4_CYC    _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_5_CYC    _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_6_CYC    _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_7_CYC    _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_8_CYC    _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_9_CYC    _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_10_CYC   _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_

/*----------------------------- End of File ----------------------------------*/
