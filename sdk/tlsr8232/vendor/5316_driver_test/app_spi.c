/********************************************************************************************************
 * @file     app_spi.c
 *
 * @brief    This is the source file for TLSR8232
 *
 * @author	 peng.sun
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
#include "tl_common.h"
#include "drivers.h"

#if (DRIVER_TEST_MODE == TEST_SPI)

#define SPI_DEVICE_MASTER           1 ///1:master ; 0:slave

#if (SPI_DEVICE_MASTER)  ///just for spi master
	#define BUFF_DATA_LEN    		16
	#define SPI_CS_PIN				GPIO_PC2//SPI CS pin
	#define SLAVE_ADDR				0x8000
	#define SLAVE_ADDR_LEN			2

	volatile unsigned char spi_tx_buff[BUFF_DATA_LEN]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
	volatile unsigned char spi_rx_buff[BUFF_DATA_LEN]={0x00};
	u32 spi_start_tick = 0;
#endif


//////////////////////////////
void app_spi_test_init(void){
	#if (SPI_DEVICE_MASTER) ///master
		spi_master_init((unsigned char)(CLOCK_SYS_CLOCK_HZ/(2*500000)-1),SPI_MODE0);//div_clock. spi_clk = sys_clk/((div_clk+1)*2),mode select
		spi_master_set_pin(SPI_GPIO_GROUP_C2C3C4C5);//master mode: spi pin set
		spi_start_tick = clock_time();
	#else ///SPI_SLAVE_DEVICE //slave
		spi_slave_init((unsigned char)(CLOCK_SYS_CLOCK_HZ/(2*500000)-1),SPI_MODE0);           //slave mode init
		spi_slave_set_pin(SPI_GPIO_GROUP_C2C3C4C5);      //slave mode spi pin set

		spi_irq_enable();

	#endif
}

////////////////////////
#if (SPI_DEVICE_MASTER)
	void spi_master_mainloop(void){
		spi_tx_buff[0] ++;
		spi_write_buff(SLAVE_ADDR, SLAVE_ADDR_LEN,(unsigned char*)spi_tx_buff, BUFF_DATA_LEN,SPI_CS_PIN);
		spi_read_buff(SLAVE_ADDR, SLAVE_ADDR_LEN,(unsigned char*)spi_rx_buff, BUFF_DATA_LEN,SPI_CS_PIN);
		WaitMs(100);
	}
#else
	void spi_slave_mainloop(void){

	}
#endif

////////////////////////////
void app_spi_test_start(void){
	#if (SPI_DEVICE_MASTER)
		spi_master_mainloop();
	#else
		spi_slave_mainloop();
	#endif
}

u32 spi_irq_cnt = 0;
_attribute_ram_code_ void app_spi_test_irq_proc(void){

	unsigned char  irq_status = reg_spi_slave_irq_status;
	//irq will occur when cs low then high
	if(irq_status & FLD_HOST_CMD_IRQ) ///spi can not distinguish host write and read.
	{
		reg_spi_slave_irq_status = irq_status;
		spi_irq_cnt ++;  ///just for debug
		DBG_CHN0_TOGGLE;
	}
}


#endif  ///end of #if (DRIVER_TEST_MODE == TEST_SPI)
