/********************************************************************************************************
 * @file     watchdog.h
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

#include "register.h"


static inline void wd_start(void){
#if(MODULE_WATCHDOG_ENABLE)		//  if watchdog not set,  start wd would cause problem
	BM_SET(reg_tmr_ctrl, FLD_TMR_WD_EN);
#endif
}

static inline void wd_startEx(unsigned int timer_ms)
{
#if(MODULE_WATCHDOG_ENABLE)
	reg_tmr_ctrl = MASK_VAL(FLD_TMR_WD_CAPT, ((timer_ms*CLOCK_SYS_CLOCK_1MS >> WATCHDOG_TIMEOUT_COEFF)));
	reg_tmr_ctrl |= FLD_TMR_WD_EN;
#endif
}

static inline void wd_stop(void){
#if(MODULE_WATCHDOG_ENABLE)
	BM_CLR(reg_tmr_ctrl, FLD_TMR_WD_EN);
#endif
}

static inline void wd_clear(void)
{
#if(MODULE_WATCHDOG_ENABLE)
	reg_tmr_sta = FLD_TMR_STA_WD;
#endif
}
/*----------------------------- End of File ----------------------------------*/
