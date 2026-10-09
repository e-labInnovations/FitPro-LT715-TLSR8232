/********************************************************************************************************
 * @file     uart.c
 *
 * @brief    This is the source file for TLSR8232
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
#include "uart.h"
#include "gpio.h"
#include <common/types.h>

/**
 * @brief     data receive buffer initiate function. DMA would move received uart data to the address space,
 *            uart packet length needs to be no larger than (recBuffLen - 4).
 * @param[in] RecvAddr - pointer to the receiving buffer
 * @param[in] RecvBufLen - length in byte of the receiving buffer
 * @return    none
 */
void uart_set_recbuff(unsigned short *RecvAddr, unsigned short RecvBufLen){
	unsigned char bufLen;
	bufLen = RecvBufLen>>4;

	reg_dma0_addr = (unsigned int)(RecvAddr) & 0xffff;//set receive buffer address
	reg_dma0_size = bufLen;//set receive buffer size
	reg_dma0_mode = FLD_DMA_1WR_0RD_MEM;   //set DMA 0 mode to 0x01 for receive
}

/**
 *define the macro that configures pin port for UART interface
 */
void uart_set_pin(UART_TxPinDef tx_pin,UART_RxPinDef rx_pin)
{
	//note: pullup setting must before uart gpio config, cause it will lead to ERR data to uart RX buffer(confirmed by sihui&sunpeng)
	//PM_PIN_PULLUP_1M   PM_PIN_PULLUP_10K
	gpio_setup_up_down_resistor(tx_pin, PM_PIN_PULLUP_1M);  //must, for stability and prevent from current leakage
	gpio_setup_up_down_resistor(rx_pin, PM_PIN_PULLUP_10K);  //must  for stability and prevent from current leakage


	gpio_set_func(tx_pin,AS_UART_TX); // set tx pin
	gpio_set_func(rx_pin,AS_UART_RX); // set rx pin


	gpio_set_input_en(tx_pin, 1);  //experiment shows that tx_pin should open input en(confirmed by qiuwei)
	gpio_set_input_en(rx_pin, 1);  //

}

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
void uart_init_baudrate(unsigned short uart_div,  unsigned char bwpc, UART_ParityTypeDef Parity, UART_StopBitTypeDef StopBit)
{
	reg_clk_en1 |= FLD_CLK1_RS232_EN;//enable uart clock
	/*******************1.config bautrate and timeout********************************/
	reg_uart_ctrl0 = bwpc; //set bwpc
	reg_uart_clk_div = (uart_div | FLD_UART_CLK_DIV_EN); //set div_clock
	reg_uart_rx_timeout0 = (bwpc+1) * 12; //one byte includes 12 bits at most
	reg_uart_rx_timeout1  = FLD_UART_BW_MUL2; //if over 2*(tmp_bwpc+1),one transaction end.

	/*******************2.config parity function*************************************/
	//parity config
	if (Parity) {
		reg_uart_ctrl1  |= FLD_UART_CTRL1_PARITY_EN; //enable parity function
		if (PARITY_EVEN == Parity) {
			reg_uart_ctrl1  &= (~FLD_UART_CTRL1_PARITY_POLARITY); //enable even parity
		}
		else if (PARITY_ODD == Parity) {
			reg_uart_ctrl1  |= FLD_UART_CTRL1_PARITY_POLARITY; //enable odd parity
		}
	}
	else {
		reg_uart_ctrl1  &= (~FLD_UART_CTRL1_PARITY_EN); //disable parity function
	}

	//stop bit config
	reg_uart_ctrl1  &= (~FLD_UART_CTRL1_STOP_BIT);
	reg_uart_ctrl1  |= StopBit;
}

/**
 * @brief     enable uart DMA mode
 * @param[in] none
 * @return    none
 */
void uart_dma_en(unsigned char rx_dma_en, unsigned char tx_dma_en)
{

	//enable DMA function of tx and rx
	if(rx_dma_en){
		reg_uart_ctrl0 |= FLD_UART_RX_DMA_EN ;
	}else{
		reg_uart_ctrl0 &= (~FLD_UART_RX_DMA_EN );
	}

	if(tx_dma_en){
		reg_uart_ctrl0  |= FLD_UART_TX_DMA_EN;
	}else{
		reg_uart_ctrl0	&= (~FLD_UART_TX_DMA_EN);
	}

}

/**
 * @brief     config the irq of uart tx and rx
 * @param[in] rx_irq_en - 1:enable rx irq. 0:disable rx irq
 * @param[in] tx_irq_en - 1:enable tx irq. 0:disable tx irq
 * @return    none
 */
void uart_irq_en(unsigned char rx_irq_en, unsigned char tx_irq_en)
{
	if(rx_irq_en){
		reg_uart_ctrl0 |= FLD_UART_RX_IRQ_EN ;
	}else{
		reg_uart_ctrl0 &= (~FLD_UART_RX_IRQ_EN );
	}

	if(tx_irq_en){
		reg_uart_ctrl0  |= FLD_UART_TX_IRQ_EN;
	}else{
		reg_uart_ctrl0	&= (~FLD_UART_TX_IRQ_EN);
	}

	if(tx_irq_en||rx_irq_en)
	{
		reg_irq_mask |= FLD_IRQ_UART_EN;
	}
	else
	{
		reg_irq_mask &= ~FLD_IRQ_UART_EN;
	}
}

/**
 * @brief     uart send data function, this  function tell the DMA to get data from the RAM and start
 *            the DMA transmission
 * @param[in] Addr - pointer to the buffer containing data need to send
 * @return    1: send success ;
 *            0: DMA busy
 */
volatile unsigned char uart_dma_send(unsigned short* Addr)
{
    if (reg_uart_status1 & FLD_UART_TX_DONE )
    {
    	reg_dma1_addr = (unsigned short)(unsigned int)Addr; //packet data, start address is sendBuff+1
        reg_dma_tx_rdy0	 |= FLD_DMA_CHN_UART_TX;
        return 1;
    }

    return 0;
}

/**
 * @brief     uart send data function, this  function tell the DMA to get data from the RAM and start
 *            the DMA transmission
 * @param[in] byte - single byte data need to send
 * @return    1: send success ;
 *            0: DMA busy
 */
volatile unsigned char uart_dma_send_byte(unsigned char byte)
{
	unsigned int addr;

	unsigned char b[5] = {1, 0,0,0,0};

	addr = (unsigned int)b;

	b[4] = byte;
	if (reg_uart_status1 & FLD_UART_TX_DONE ) {
		reg_dma1_addr = addr; //packet data, start address is sendBuff+1
		reg_dma_tx_rdy0	 = FLD_DMA_CHN1;
		return 1;
	}

	   return 0;
}
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
void uart_ndma_set_triglevel(unsigned char rx_level, unsigned char tx_level)
{
	reg_uart_ctrl3 = rx_level | (tx_level<<4);
}

/**
 * @brief     get the status of uart irq.
 * @param[in] none
 * @return    0: not uart irq ;
 *            not 0: indicate tx or rx irq
 */
unsigned char uart_ndma_get_irq(void)
{
	return  (reg_uart_status0&FLD_UART_IRQ_FLAG );
}


/**
 * @brief     uart send data function with not DMA method.
 *            variable uart_TxIndex,it must cycle the four registers 0x90 0x91 0x92 0x93 for the design of SOC.
 *            so we need variable to remember the index.
 * @param[in] uartData - the data to be send.
 * @return    none
 */
unsigned char uart_TxIndex = 0;
void uart_ndma_send_byte(unsigned char uartData)
{
	int t;

	t = 0;
	while( uart_tx_is_busy() && (t<0xfffff))
	{
		t++;
	}
	if(t >= 0xfffff)
		return;

	reg_uart_data_buf(uart_TxIndex) = uartData;

	uart_TxIndex++;
	uart_TxIndex &= 0x03;// cycle the four register 0x90 0x91 0x92 0x93.
}




/**
 * @Brief:  UART CTS initialization.
 * @Param:
 * @Retval: None.
 */
void uart_set_cts(unsigned char ctsEnable,unsigned char pinValue,UART_CtsPinDef pin )
{
	if(pinValue)
	{
		reg_uart_ctrl1  |= FLD_UART_CTRL1_CTS_SELECT;
	}
	else
	{
		reg_uart_ctrl1 &= ~FLD_UART_CTRL1_CTS_SELECT;;
	}

	if(ctsEnable)
	{
		gpio_set_func(pin,AS_UART_CTS);
		gpio_set_input_en(pin, 1);
		reg_uart_ctrl1 |= FLD_UART_CTRL1_CTS_EN;
	}
	else
	{
		reg_uart_ctrl1 &= ~FLD_UART_CTRL1_CTS_EN;
	}
}

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
void uart_set_rts(unsigned char Enable, UART_RTSModeTypeDef Mode, unsigned char Thresh, unsigned char Invert, UART_RtsPinDef pin)
{
    if (Enable)
    {
    	gpio_set_func(pin,AS_UART_RTS);
    	gpio_set_input_en(pin, 1);//enable input
    	gpio_set_output_en(pin, 1);//enable output

        reg_uart_ctrl2 |= FLD_UART_CTRL2_RTS_EN; //enable RTS function
    }
    else
    {
        reg_uart_ctrl2 &= (~FLD_UART_CTRL2_RTS_EN); //disable RTS function
    }

    if (Mode)
    {
    	reg_uart_ctrl2 |= FLD_UART_CTRL2_RTS_MANUAL_EN;
    }
    else {
    	reg_uart_ctrl2 &= (~FLD_UART_CTRL2_RTS_MANUAL_EN);
    }

    if (Invert) {
    	reg_uart_ctrl2 |= FLD_UART_CTRL2_RTS_PARITY;
    }
    else {
    	reg_uart_ctrl2 &= (~FLD_UART_CTRL2_RTS_PARITY);
    }

    //set threshold
    reg_uart_ctrl2 &= (~FLD_UART_CTRL2_RTS_TRIG_LVL);
    reg_uart_ctrl2 |= (Thresh & FLD_UART_CTRL2_RTS_TRIG_LVL);
}

/**
 * @brief     This function determines whether parity error occurs once a packet arrives.
 * @param[in] none
 * @return    1: parity error ;
 *            0: no parity error
 */
unsigned char uart_is_parity_error(void)
{
    return (reg_uart_status0 & FLD_UART_RX_ERR_FLAG);
}

/**
 * @brief     This function clears parity error status once when it occurs.
 * @param[in] none
 * @return    none
 */
void uart_clear_parity_error(void)
{
	reg_uart_status0|= FLD_UART_RX_ERR_CLR; //write 1 to clear
}


/*-------------------------- End of File -------------------------------------*/

