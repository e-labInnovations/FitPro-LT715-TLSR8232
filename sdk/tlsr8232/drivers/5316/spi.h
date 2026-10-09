/********************************************************************************************************
 * @file     spi.h
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

#pragma once

#ifndef SPI_H
#define SPI_H

#include "bsp.h"
#include "gpio.h"

#define SPI_WRITE_CMD   0x00
#define SPI_READ_CMD    0x80

/**
 *  @brief  Define the mode for SPI interface
 */
typedef enum {
    SPI_MODE0 = 0,
    SPI_MODE2,
    SPI_MODE1,
    SPI_MODE3,
} SPI_ModeTypeDef;


/**
 *  @brief  Define the spi pin group
 */
// CN  SDO  SDI  SCK
// C2   C3   A3   A4
// B0   B1   B2   B3
// C2   C3   C4   C5
typedef enum{
	SPI_GPIO_GROUP_B0B1B2B3=0, //Master
	SPI_GPIO_GROUP_C2C3A3A4,    //Slave
	SPI_GPIO_GROUP_C2C3C4C5,   //Master and Slave

}SPI_GPIO_GroupTypeDef;

/**
 * @brief This function reset SPI module.
 * @param[in] none
 * @return none
 */
static inline void spi_reset(void)
{
	reg_rst0 |= FLD_RST0_SPI;
	reg_rst0 &= (~FLD_RST0_SPI);
}

/**
 * @brief     This function configures the clock and working mode for SPI interface
 * @param[in] DivClock - the division factor for SPI module
 *            SPI clock = System clock / ((DivClock+1)*2)
 * @param[in] Mode - the selected working mode of SPI module
 *            Telink spi supports four standard working modes
 *            register  0x0b set working mode
 *            bit0:CPOL-Clock Polarity  ; bit1:CPHA-Clock Phase
 *            MODE0: CPOL = 0 , CPHA =0;
 *            MODE1: CPOL = 0 , CPHA =1;
 *            MODE2: CPOL = 1 , CPHA =0;
 *            MODE3: CPOL = 1 , CPHA =1;
 * @return    none
 */
extern void spi_master_init(unsigned char DivClock, SPI_ModeTypeDef Mode);

/**
 *  @brief  This function configures the spi pins for a master device
 */
extern void spi_master_set_pin(SPI_GPIO_GroupTypeDef PinGrp);

/**
 * @brief      This function serves to write a bulk of data to the SPI slave
 *             device specified by the CS pin
 * @param[in]  Addr - pointer to the address  needed written into the
 *             slave device first before the writing operation of actual data
 * @param[in]  AddrLen - length in byte of the address of slave device
 * @param[in]  Data - pointer to the data need to write
 * @param[in]  DataLen - length in byte of the data need to write
 * @param[in]  CSPin - the CS pin specifing the slave device
 * @return     none
 */
extern void spi_write_buff(unsigned int Addr, unsigned char AddrLen,  unsigned char *Data, int DataLen, GPIO_PinTypeDef CSPin);

/**
 * @brief      This function serves to read a bulk of data from the SPI slave
 *             device specified by the CS pin
 * @param[in]  Addr - pointer to the target address needed written into the
 *             slave device first before the reading operation of actual data
 * @param[in]  AddrLen - length in byte of the address of slave device
 * @param[out] Data - pointer to the buffer that will cache the reading out data
 * @param[in]  DataLen - length in byte of the data need to read
 * @param[in]  CSPin - the CS pin specifing the slave device
 * @return     none
 */
extern void spi_read_buff(unsigned int Addr, unsigned char AddrLen, unsigned char *Data, int DataLen, GPIO_PinTypeDef CSPin);

/**
 * @brief     This function selects a GPIO pin as CS of SPI function.
 * @param[in] CSPin - the selected CS pin
 * @return    none
 */
extern void spi_master_set_cs_pin(GPIO_PinTypeDef CSPin);

/**
 * @brief     This function configures the clock and working mode for SPI interface
 * @param[in] DivClock - the division factor for SPI module
 *            SPI clock = System clock / ((DivClock+1)*2)
 * @param[in] Mode - the selected working mode of SPI module
 *            Telink spi supports four standard working modes
 *            register  0x0b set working mode
 *            bit0:CPOL-Clock Polarity  ; bit1:CPHA-Clock Phase
 *            MODE0: CPOL = 0 , CPHA =0;
 *            MODE1: CPOL = 0 , CPHA =1;
 *            MODE2: CPOL = 1 , CPHA =0;
 *            MODE3: CPOL = 1 , CPHA =1;
 * @return    none
 */
extern void spi_slave_init(unsigned char DivClock, SPI_ModeTypeDef Mode);

/**
 *  @brief  This function sets the spi pins for a slave device
 */
extern void spi_slave_set_pin(SPI_GPIO_GroupTypeDef PinGrp);

/***
 * brief: this function can enable spi module interrupt.
 */
void spi_irq_enable(void);

#endif /* End of SPI_H */

