/********************************************************************************************************
 * @file     app_uart.c
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
 * @par      History:
 * 			 1.initial release(DEC. 26 2018)
 *
 * @version  A001
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"

#if (DRIVER_TEST_MODE == TEST_UART)

#define UART_DMA      1
#define UART_NDMA     2
#define UART_MODE     UART_DMA

#if (UART_MODE == UART_NDMA)
	#define NORMAL	   		1
	#define USE_CTS    		2
	#define USE_RTS    		3

	#define FLOW_CTR  		NORMAL

	#if( FLOW_CTR==NORMAL)
		u8 uart_rx_flag = 0;
		u8 uart_ndmairq_index = 0;
		u8 uart_ndmairq_cnt = 0;
	#endif
	#if( FLOW_CTR==USE_CTS)
		u8 uart_cts_count = 0;
		#define STOP_VOLT   	1			//0 :Low level stops TX.  1 :High level stops TX.
	#endif

	#if (FLOW_CTR==USE_RTS)
		#define RTS_MODE		UART_RTS_MODE_AUTO 		    //It can be UART_RTS_MODE_AUTO/UART_RTS_MODE_MANUAL.
		#define RTS_THRESH		4			//UART_RTS_MODE_AUTO need.It indicates RTS trigger threshold.
		#define RTS_INVERT		1			//UART_RTS_MODE_AUTO need.0 indicates RTS_pin will change from low to hign.
		#define RTS_POLARITY	0			//UART_RTS_MODE_MANUAL need. It indicates RTS_POLARITY .
	#endif
#endif

volatile unsigned char uart_dmairq_tx_cnt;
volatile unsigned char uart_dmairq_rx_cnt;


#define REV_BUFF_LE     16
#define TRANS_BUFF_LEN  16

__attribute__((aligned(4))) unsigned char rec_buff[REV_BUFF_LE] = {0};
// dma len must be 4 byte
__attribute__((aligned(4))) unsigned char trans_buff[TRANS_BUFF_LEN] = {0x0c, 0x00, 0x00, 0x00, 0x11, 0x22, 0x33, 0x44,
																		0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};


#if(UART_MODE == UART_DMA)
	void app_uart_test_init(void){
		WaitMs(2000);  //leave enough time for SWS_reset when power on
		//note: dma addr must be set first before any other uart initialization! (confirmed by sihui)
		uart_set_recbuff( (unsigned short *)&rec_buff, sizeof(rec_buff));

		uart_set_pin(UART_TX_PA3, UART_RX_PA4);// uart tx/rx pin set

		uart_reset();  //will reset uart digital registers from 0x90 ~ 0x9f, so uart setting must set after this reset

		////9&13 indicate baud rate is 115200. other baud rate, pls use lua tool to calculate.
		uart_init_baudrate(9, 13,PARITY_NONE, STOP_BIT_ONE);

		uart_dma_en(1, 1); 	//uart data in hardware buffer moved by dma, so we need enable them first

		irq_set_mask(FLD_IRQ_DMA_EN);

		dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);   	//uart Rx/Tx dma irq enable

		uart_irq_en(0, 0);  	//uart Rx/Tx irq no need, disable them

		irq_enable();
	}

	void app_uart_test_start(void){
		WaitMs(1000);
		uart_dma_send((unsigned short*)&rec_buff);
		WaitMs(300);
		uart_dma_send((unsigned short*)&trans_buff);
	}

#elif(UART_MODE == UART_NDMA)
	void app_uart_test_init(void){
		WaitMs(2000);  //leave enough time for SWS_reset when power on
		//note: dma addr must be set first before any other uart initialization! (confirmed by sihui)
		uart_set_recbuff( (unsigned short *)&rec_buff, sizeof(rec_buff));

		uart_set_pin(UART_TX_PA3, UART_RX_PA4);// uart tx/rx pin set

		uart_reset();  //will reset uart digital registers from 0x90 ~ 0x9f, so uart setting must set after this reset

		//9&13 indicate the baud rate is 115200. other baud rate, pls use lua tool to calculate.
		uart_init_baudrate(9, 13, PARITY_NONE, STOP_BIT_ONE);

		#if( FLOW_CTR == NORMAL)

			uart_dma_en(0, 0);

			irq_clr_mask(FLD_IRQ_DMA_EN);

			dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 0);

			uart_irq_en(1,0);   //uart RX irq enable

			uart_ndma_set_triglevel(1,0);	//set the trig level. 1 indicate one byte will occur interrupt

		#elif( FLOW_CTR ==  USE_CTS )
			//CTS pin.It can be A1/B2/B7/C2.
			uart_set_cts(1, STOP_VOLT, UART_CTS_PC2);

			uart_dma_en(0, 0);				       //Disable DMA

			irq_clr_mask(FLD_IRQ_DMA_EN);

			dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 0);	//Disable DMA irq

			uart_irq_en(0,0);   //uart RX irq disable

		#elif( FLOW_CTR ==   USE_RTS )
			// RTS pin : A2  B3 B6 C3
			uart_set_rts(1, RTS_MODE, RTS_THRESH, RTS_INVERT,UART_RTS_PA2);
			uart_dma_en(0, 0);

			irq_clr_mask(FLD_IRQ_DMA_EN);
			dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 0);

			uart_irq_en(1,0);   //uart RX irq enable

			uart_ndma_set_triglevel(RTS_THRESH,0);
		#endif

			irq_enable();
	}


	void app_uart_test_start(void){
		WaitMs(1000);
		#if( FLOW_CTR == NORMAL)

			for(unsigned char i=0;i<TRANS_BUFF_LEN;i++){
				uart_ndma_send_byte(trans_buff[i]);
			}
			if(uart_rx_flag>0){
				uart_ndmairq_cnt=0; //Clear uart_ndmairq_cnt
				uart_rx_flag=0;
				for(unsigned char i=0;i<REV_BUFF_LE;i++){
					uart_ndma_send_byte(rec_buff[i]);
				}
			}
		#elif( FLOW_CTR ==  USE_CTS )
			uart_ndma_send_byte(trans_buff[uart_cts_count]);
			uart_cts_count++;
			if(uart_cts_count == 16)
			{
				uart_cts_count=0;
			}

		#elif( FLOW_CTR ==  USE_RTS )

		#endif
	}

#endif ///end of (UART_MODE == UART_NDMA)


_attribute_ram_code_  void app_uart_test_irq_proc(void){
#if (UART_MODE==UART_DMA)

	unsigned char uart_dma_irqsrc;
	//1. UART irq
	uart_dma_irqsrc = dma_chn_irq_status_get();///in function,interrupt flag have already been cleared,so need not to clear DMA interrupt flag here

	if(uart_dma_irqsrc & FLD_DMA_CHN_UART_RX){
		dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX);
		uart_dmairq_rx_cnt++;
		//Received uart data in rec_buff, user can copy data in here
	}
	if(uart_dma_irqsrc & FLD_DMA_CHN_UART_TX){
		dma_chn_irq_status_clr(FLD_DMA_CHN_UART_TX);
		uart_dmairq_tx_cnt++;
	}

#elif(UART_MODE==UART_NDMA)
	#if( FLOW_CTR == NORMAL)
		static unsigned char uart_ndma_irqsrc;
		uart_ndma_irqsrc = uart_ndma_get_irq();  ///get the status of uart irq.
		if(uart_ndma_irqsrc){

	//cycle the four registers 0x90 0x91 0x92 0x93,in addition reading will clear the irq.
		if(uart_rx_flag==0){
			rec_buff[uart_ndmairq_cnt++] = reg_uart_data_buf(uart_ndmairq_index);
			uart_ndmairq_index++;
			uart_ndmairq_index &= 0x03;// cycle the four registers 0x90 0x91 0x92 0x93, it must be done like this for the design of SOC.
			if(uart_ndmairq_cnt%16==0&&uart_ndmairq_cnt!=0){
				uart_rx_flag=1;
			}
		}
		else{
			READ_REG8(0x90+ uart_ndmairq_index);
			uart_ndmairq_index++;
			uart_ndmairq_index &= 0x03;
		}
	}
	#endif

#endif
}


#endif ///// end of #if (DRIVER_TEST_MODE == TEST_UART)


