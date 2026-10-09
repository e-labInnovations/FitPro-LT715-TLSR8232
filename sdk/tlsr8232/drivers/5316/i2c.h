/********************************************************************************************************
 * @file     i2c.h
 *
 * @brief    This is the header file for TLSR8232
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

//MUST NOTE: When as I2C master, A3 is clock line;A4 is data line.
//           When as I2C slave , A3 is data line; A4 is clock line. other groups pin is normal.

#pragma once
#ifndef I2C_H
#define I2C_H
#include "gpio.h"


typedef enum {
	I2C_SLAVE_DMA = 0,
	I2C_SLAVE_MAP,
}I2C_SlaveMode;

// SDA   SCL
//  C0    C1
//  C2    C3
//  A3    A4
//  B6    D7
typedef enum {
	I2C_GPIO_GROUP_M_A3A4 = 0,  // master and slave
	I2C_GPIO_GROUP_M_A5A6,  // master and slave
	I2C_GPIO_GROUP_M_B2B3,  // master only
	I2C_GPIO_GROUP_M_B6B7,  // master only
	I2C_GPIO_GROUP_M_C4C5,  // master and slave
	I2C_GPIO_GROUP_S_A3A4,  //Slave
	I2C_GPIO_GROUP_S_A5A6,  //Slave
	I2C_GPIO_GROUP_S_C4C5,  //Slave

}I2C_GPIO_GroupTypeDef;

/**
 * @brief This function reset I2C module.
 * @param[in] none
 * @return none
 */
static inline void i2c_reset(void)
{
	reg_rst0 |= FLD_RST0_I2C;
	reg_rst0 &= (~FLD_RST0_I2C);
}

/**
 * @brief This function serves to set id of I2C module.
 * @param[in] id - this id is fixed id for slave device.For master device, this id is set to access different slave devices.
 * @return none
 */
static inline void i2c_set_id(unsigned char SlaveID)
{
    reg_i2c_id	  = SlaveID; //slave address
}
/**
 * @brief      This function serves to select a pin port for I2C interface.
 * @param[in]  PinGrp - the pin port selected as I2C interface pin port.
 * @return     none
 */
void i2c_set_pin(I2C_GPIO_GroupTypeDef i2c_pin_group);

/**
 * @brief      This function set the id of slave device and the speed of I2C interface
 *             note: the param ID contain the bit of writting or reading.
 *             eg:the parameter 0x5C. the reading will be 0x5D and writting 0x5C.
 * @param[in]  SlaveID - the id of slave device.it contains write or read bit,the lsb is write or read bit.
 *                       ID|0x01 indicate read. ID&0xfe indicate write.
 * @param[in]  DivClock - the division factor of I2C clock,
 *             I2C clock = System clock / (4*DivClock);if the datasheet you look at is 2*,pls modify it.
 * @return     none
 */
void i2c_master_init(unsigned char SlaveID, unsigned char DivClock);
/**
 *  @brief      the function config the ID of slave and mode of slave.
 *  @param[in]  device_ID - it contains write or read bit,the lsb is write or read bit.
 *              ID|0x01 indicate read. ID&0xfe indicate write.
 *  @param[in]  mode - set slave mode. slave has two modes, one is DMA mode, the other is MAPPING mode.
 *  @param[in]  pBuf - if slave mode is MAPPING, set the first address of buffer master write or read slave.
 *  @return     none
 */
void i2c_slave_init(unsigned char device_ID,I2C_SlaveMode mode,unsigned char * pMapBuf);
/**
 * @brief      This function serves to write one byte to the slave device at the specified address
 * @param[in]  Addr - i2c slave address where the one byte data will be written
 * @param[in]  AddrLen - length in byte of the address, which makes this function is
 *             compatible for slave device with both one-byte address and two-byte address
 * @param[in]  Data - the one byte data will be written via I2C interface
 * @return     none
 */
void i2c_dma_write_byte(unsigned int Addr, unsigned int AddrLen, unsigned char Data);
/**
 * @brief      This function serves to read one byte from the slave device at the specified address
 * @param[in]  Addr - i2c slave address where the one byte data will be read
 * @param[in]  AddrLen - length in byte of the address, which makes this function is
 *             compatible for slave device with both one-byte address and two-byte address
 * @return     the one byte data read from the slave device via I2C interface
 */
unsigned char i2c_dma_read_byte(unsigned int Addr, unsigned int AddrLen);
/**
 *  @brief      This function serves to write a packet of data to the specified address of slave device
 *  @param[in]  Addr - the register that master write data to slave in. support one byte and two bytes. i.e param2 AddrLen may be 1 or 2.
 *  @param[in]  AddrLen - the length of register. enum 0 or 1 or 2 or 3. based on the spec of i2c slave.
 *  @param[in]  dataBuf - the first SRAM buffer address to write data to slave in.
 *  @param[in]  dataLen - the length of data master write to slave.
 *  @return     none
 */

void i2c_dma_write_buff (unsigned int Addr, unsigned int AddrLen, unsigned char * dataBuf, int dataLen);
/**
 * @brief      This function serves to read a packet of data from slave device working in mapping mode
 * @param[in]  dataBuf - the first address of SRAM buffer master store data in.
 * @param[in]  dataLen - the length of data master read from slave.
 * @return     none.
 */
void i2c_dma_read_buff(unsigned int Addr, unsigned int AddrLen, unsigned char * dataBuf, int dataLen);


/**
 *  @brief      This function serves to write a packet of data to slave device working in mapping mode
 *  @param[in]  dataBuf - the first SRAM buffer address to write data to slave in.
 *  @param[in]  dataLen - the length of data master write to slave.
 *  @return     none
 */
void i2c_map_write_buff(unsigned char * dataBuf, int dataLen);
/**
 * @brief      This function serves to read a packet of data from slave device working in mapping mode
 * @param[in]  dataBuf - the first address of SRAM buffer master store data in.
 * @param[in]  dataLen - the length of data master read from slave.
 * @return     none.
 */
void i2c_map_read_buff(unsigned char * dataBuf, int dataLen);


/***
 * brief: this function can enable i2c module interrupt.
 */
void i2c_irq_enable(void);


typedef enum{
	I2C_Irq_None     = 0,
	I2C_Read_IrqFlag = 1,
	I2C_Write_IrqFlag= 2,
}I2C_IrqFlagTypeDef;

/**
 * brief: this function will clear the i2c interrupt state flag.
 * note: no matter i2c read interrupt or i2c write interrupt,
 * user can use this function to clear the irq state.
 * this function will check which interrupt is set and it will clear relevant flag.
 */
static inline void i2c_clear_interrupt(void){
	if(reg_i2c_slave_irq_status&FLD_HOST_READ_IRQ){
		reg_i2c_slave_irq_status |= (FLD_HOST_READ_IRQ|FLD_HOST_CMD_IRQ);
	}
	else if(reg_i2c_slave_irq_status&FLD_HOST_CMD_IRQ){
		reg_i2c_slave_irq_status |= FLD_HOST_CMD_IRQ;
	}
}




#endif



