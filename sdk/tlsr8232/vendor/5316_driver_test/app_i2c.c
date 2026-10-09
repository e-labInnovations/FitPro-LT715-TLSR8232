/********************************************************************************************************
 * @file     app_i2c.c
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
 * @par      History:
 * 			 1.initial release(DEC. 26 2018)
 *
 * @version  A001
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"

#if (DRIVER_TEST_MODE == TEST_IIC)

#define I2C_DEVICE_MASTER           0 ///1:master ; 0:slave

//////I2C slave mode selection ///////
#define I2C_SLAVE_DMA_MODE          0
#define I2C_SLAVE_MAP_MODE          1
#define I2C_SLAVE_MODE              I2C_SLAVE_DMA_MODE

#if (!I2C_DEVICE_MASTER)
	#if (I2C_SLAVE_MODE == I2C_SLAVE_MAP_MODE)
		__attribute__((aligned(128))) unsigned char i2c_slave_mapping_buff[128] = {0};
	#endif
#endif

///////////////////////////////////////////////////
#define     BUFF_DATA_LEN				16
#define     SLAVE_DEVICE_ADDR			0x8000
#define     SLAVE_DEVICE_ADDR_LEN		2
#define     I2C_CLK_SPEED				200000
volatile unsigned char i2c_tx_buff[BUFF_DATA_LEN] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
volatile unsigned char i2c_rx_buff[BUFF_DATA_LEN] = {0};


void app_i2c_test_init(void){

	#if (I2C_DEVICE_MASTER)
		i2c_set_pin(I2C_GPIO_GROUP_M_A3A4);  	// SDA/CK : C0/C1
		i2c_master_init(0x5C, (unsigned char)(CLOCK_SYS_CLOCK_HZ/(4*I2C_CLK_SPEED)) ); // 200KHz
	#else
		i2c_set_pin(I2C_GPIO_GROUP_S_A3A4);  	//SDA/CK : C0/C1

		#if (I2C_SLAVE_MODE == I2C_SLAVE_DMA_MODE)
			i2c_slave_init(0x5C, I2C_SLAVE_DMA, NULL);
		#else
			i2c_slave_init(0x5C, I2C_SLAVE_MAP, (unsigned char *)i2c_slave_mapping_buff+64);
		#endif

			i2c_irq_enable();
	#endif
}


void i2c_master_mainloop(void){
	#if (I2C_SLAVE_MODE == I2C_SLAVE_DMA_MODE)
		i2c_tx_buff[0]++;
		i2c_dma_write_buff(SLAVE_DEVICE_ADDR, SLAVE_DEVICE_ADDR_LEN, (unsigned char *)i2c_tx_buff, BUFF_DATA_LEN);
		WaitMs(100);
		i2c_dma_read_buff(SLAVE_DEVICE_ADDR,  SLAVE_DEVICE_ADDR_LEN, (unsigned char *)i2c_rx_buff, BUFF_DATA_LEN);
		WaitMs(100);
	#else
		i2c_tx_buff[0]++;
		i2c_map_write_buff((unsigned char*)i2c_tx_buff, BUFF_DATA_LEN);
		WaitMs(100);
		i2c_map_read_buff((unsigned char*)i2c_rx_buff, BUFF_DATA_LEN);
		WaitMs(100);
	#endif
}

void i2c_slave_mainloop(void){

}

void app_i2c_test_start(void){
	#if (I2C_DEVICE_MASTER)
		i2c_master_mainloop();
	#else
		i2c_slave_mainloop();
	#endif
}


u32 i2c_read_cnt = 0;
u32 i2c_write_cnt = 0;

///FLD_HOST_CMD_IRQ&FLD_HOST_READ_IRQ indicate host reading operation.
///only FLD_HOST_CMD_IRQ indicate host write operation
_attribute_ram_code_ void app_i2c_test_irq_proc(void){
	unsigned char  irq_status = reg_i2c_slave_irq_status;

	if(irq_status & FLD_HOST_CMD_IRQ)
	{
		reg_i2c_slave_irq_status = irq_status; //clear all irq status

		if(irq_status & FLD_HOST_READ_IRQ)   ////read operation
		{
			i2c_read_cnt ++;
			DBG_CHN0_TOGGLE; ///just for debug
		}
		else     ///write operation
		{
			i2c_write_cnt ++;
			DBG_CHN0_TOGGLE; ///just for debug
		}
	}
}


#endif ///end of (#if (DRIVER_TEST_MODE == TEST_IIC))



