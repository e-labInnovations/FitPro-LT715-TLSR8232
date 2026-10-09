/*
 * emi.h
 *
 *  Created on: 2019-8-12
 *      Author: Administrator
 */

#ifndef EMI_H_
#define EMI_H_

#include "bsp.h"
#include "register.h"

void rf_emi_single_tone(RF_TxPowerTypeDef power_level,signed char rf_chn);
void rf_continue_mode_run(void);
void rf_emi_tx_continue_setup(RF_ModeTypeDef rf_mode, RF_TxPowerTypeDef power_level,signed char rf_chn,unsigned char pkt_type);
void rf_emi_tx_brust_setup(RF_ModeTypeDef rf_mode,RF_TxPowerTypeDef power_level,signed char rf_chn,unsigned char pkt_type);
void rf_emi_tx_brust_loop(RF_ModeTypeDef rf_mode,unsigned char pkt_type);
void rf_emi_rx(RF_ModeTypeDef mode,signed char rf_chn);
void rf_emi_rx_loop(void);
unsigned int rf_emi_get_rxpkt_cnt(void);
char rf_emi_get_rssi_avg(void);

#endif /* EMI_H_ */
