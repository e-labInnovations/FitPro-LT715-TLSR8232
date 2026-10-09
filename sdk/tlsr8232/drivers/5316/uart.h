/********************************************************************************************************
 * @file     uart.h
 *
 * @brief    This is the header file for TLSR8232
 *
 * @author	 peng.sun ; yang.ye
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

#ifndef  _UART_H
#define  _UART_H

#include "register.h"
#include "gpio.h"

/**
 *  @brief  Define mul bits
 */
enum{
	FLD_UART_BW_MUL1  = 0,  // timeout is bit_width*1
	FLD_UART_BW_MUL2  = 1,  // timeout is bit_width*2
	FLD_UART_BW_MUL3  = 2,  // timeout is bit_width*3
	FLD_UART_BW_MUL4  = 3,  // timeout is bit_width*4
};

/**
 *  @brief  Define parity type
 */
typedef enum {
    PARITY_NONE = 0,
    PARITY_EVEN,
    PARITY_ODD,
}UART_ParityTypeDef;

/**
 *  @brief  Define the length of stop bit
 */
typedef enum {
    STOP_BIT_ONE = 0,
    STOP_BIT_ONE_DOT_FIVE = BIT(12),
    STOP_BIT_TWO = BIT(13),
}UART_StopBitTypeDef;


/**
 *  @brief  Define uart tx pin
 */
typedef enum{
	UART_TX_PA3 = GPIO_PA3,
	UART_TX_PB4 = GPIO_PB4,//?
	UART_TX_PC4 = GPIO_PC4,
}UART_TxPinDef;

/**
 *  @brief  Define uart rx pin
 */
typedef enum{
	UART_RX_PA4 = GPIO_PA4,//OK
	UART_RX_PB5 = GPIO_PB5,//Uncertain
	UART_RX_PC5 = GPIO_PC5,//OK

}UART_RxPinDef;

typedef enum{

	UART_CTS_PA1 = GPIO_PA1,
	UART_CTS_PB2 = GPIO_PB2,
	UART_CTS_PB7 = GPIO_PB7,//
	UART_CTS_PC2 = GPIO_PC2,

}UART_CtsPinDef;


typedef enum{

	UART_RTS_PA2 = GPIO_PA2,//0K
	UART_RTS_PB3 = GPIO_PB3,//Failure
	UART_RTS_PB6 = GPIO_PB6,//Uncertain
	UART_RTS_PC3 = GPIO_PC3,//0K

}UART_RtsPinDef;

/**
 *  @brief  Define UART RTS mode
 */
typedef enum {
    UART_RTS_MODE_AUTO = 0,
    UART_RTS_MODE_MANUAL,
} UART_RTSModeTypeDef;

extern unsigned char uart_TxIndex;

/**
 * @brief     This function servers to indicate Tx state.
 * @param[in] none.
 * @return    the state of Tx 0:Tx done 1:not.
 */
static inline unsigned char uart_tx_is_busy(void)
{
    return ( (reg_uart_status1 & FLD_UART_TX_DONE) ? 0 : 1) ;
}

/**
 * @brief     This function is used to set the 'uart_TxIndex' to 0.
 *			  After wakeup from power-saving mode, you must call this function before sending the data.
 * @param[in] none.
 * @return    none.
 */
static inline void uart_ndma_clear_tx_index(void)
{
    uart_TxIndex=0;
}

/**
 *	@brief	reset uart module
 *	@param	none
 *	@return	none
 */
static inline void uart_reset(void){
	BM_SET(reg_rst1, FLD_RST1_RS232);
	BM_CLR(reg_rst1, FLD_RST1_RS232);
}

/**
 * @brief     data receive buffer initiate function. DMA would move received uart data to the address space,
 *            uart packet length needs to be no larger than (recBuffLen - 4).
 * @param[in] RecvAddr - pointer to the receiving buffer
 * @param[in] RecvBufLen - length in byte of the receiving buffer
 * @return    none
 */
extern void uart_set_recbuff(unsigned short *RecvAddr, unsigned short RecvBufLen);

/**
 *define the macro that configures pin port for UART interface
 */
extern void uart_set_pin(UART_TxPinDef tx_pin,UART_RxPinDef rx_pin);

/**
 *	@brief	uart initiate, set uart clock divider, bitwidth and the uart work mode
 *	@param	uartCLKdiv - uart clock divider
 *			bwpc - bitwidth, should be set to larger than 2
 *	@return	'1' set success; '0' set error probably bwpc smaller than 3.
 *		BaudRate = sclk/((uartCLKdiv+1)*(bwpc+1))
 *		SYCLK = 16Mhz
 *		115200		9			13
 *		9600		103			15
 *
 *		SYCLK = 32Mhz
 *		115200		19			13
 *		9600		237			13
 */
extern void uart_init_baudrate(unsigned short uart_div,  unsigned char bwpc,UART_ParityTypeDef Parity, UART_StopBitTypeDef StopBit);

/**
 * @brief     enable uart DMA mode
 * @param[in] none
 * @return    none
 */
extern void uart_dma_en(unsigned char rx_dma_en, unsigned char tx_dma_en);

/**
 * @brief     config the irq of uart tx and rx
 * @param[in] rx_irq_en - 1:enable rx irq. 0:disable rx irq
 * @param[in] tx_irq_en - 1:enable tx irq. 0:disable tx irq
 * @return    none
 */
extern void uart_irq_en(unsigned char rx_irq_en,unsigned char tx_irq_en);

/**
 * @brief     uart send data function, this  function tell the DMA to get data from the RAM and start
 *            the DMA transmission
 * @param[in] Addr - pointer to the buffer containing data need to send
 * @return    1: send success ;
 *            0: DMA busy
 */
extern volatile unsigned char uart_dma_send(unsigned short* Addr);

/**
 * @brief     uart send data function, this  function tell the DMA to get data from the RAM and start
 *            the DMA transmission
 * @param[in] byte - single byte data need to send
 * @return    1: send success ;
 *            0: DMA busy
 */
volatile unsigned char uart_dma_send_byte(unsigned char byte);

/**
 * @brief     config the number level setting the irq bit of status register 0x9d
 *            ie 0x9d[3].
 *            If the cnt register value(0x9c[0,3]) larger or equal than the value of 0x99[0,3]
 *            or the cnt register value(0x9c[4,7]) less or equal than the value of 0x99[4,7],
 *            it will set the irq bit of status register 0x9d, ie 0x9d[3]
 * @param[in] rx_level - receive level value. ie 0x99[0,3]
 * @param[in] tx_level - transmit level value.ie 0x99[4,7]
 * @return    none
 */
extern void uart_ndma_set_triglevel(unsigned char rx_level, unsigned char tx_level);

/**
 * @brief     get the status of uart irq.
 * @param[in] none
 * @return    0: not uart irq ;
 *            not 0: indicate tx or rx irq
 */
extern unsigned char uart_ndma_get_irq(void);

/**
 * @brief     uart send data function with not DMA method.
 *            variable uart_TxIndex,it must cycle the four registers 0x90 0x91 0x92 0x93 for the design of SOC.
 *            so we need variable to remember the index.
 * @param[in] uartData - the data to be send.
 * @return    none
 */
extern void uart_ndma_send_byte(unsigned char uartData);

/**
 * @Brief:  UART CTS initialization.
 * @Param:
 * @Retval: None.
 */
extern void uart_set_cts(unsigned char ctsEnable,unsigned char pinValue,UART_CtsPinDef pin);

/**
 * @brief     UART hardware flow control configuration. Configure RTS pin.
 * @param[in] Enable - enable or disable RTS function.
 * @param[in] Mode - set the mode of RTS(auto or manual).
 * @param[in] Thresh - threshold of trig RTS pin's level toggle(only for auto mode),
 *                     it means the number of bytes that has arrived in Rx buf.
 * @param[in] Invert - whether invert the output of RTS pin(only for auto mode)
 * @param[in] GPIO   - RTS pin select,it can be GPIO_PA4/GPIO_PB3/GPIO_PB6/GPIO_PC0.
 * @return    none
 */
extern void uart_set_rts(unsigned char Enable, UART_RTSModeTypeDef Mode, unsigned char Thresh, unsigned char Invert, UART_RtsPinDef pin);

/**
 * @brief     This function determines whether parity error occurs once a packet arrives.
 * @param[in] none
 * @return    1: parity error ;
 *            0: no parity error
 */
extern unsigned char uart_is_parity_error(void);

/**
 * @brief     This function clears parity error status once when it occurs.
 * @param[in] none
 * @return    none
 */
extern void uart_clear_parity_error(void);


#endif
