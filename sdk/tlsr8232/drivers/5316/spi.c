/********************************************************************************************************
 * @file     spi.c
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

#include "spi.h"
#include "register.h"
#include "gpio.h"
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
void spi_master_init(unsigned char DivClock, SPI_ModeTypeDef Mode)
{

	reg_clk_en0 |= FLD_CLK0_SPI_EN;//enable spi clock
	reg_spi_sp = 0;            //clear register(0x0a) value
	reg_spi_sp|= DivClock;     //0x0a: bit0~bit6 set spi clock ; spi clock=system clock/((DivClock+1)*2)
	reg_spi_sp|=FLD_SPI_ENABLE;//0x0a: bit7 enables spi function mode
	reg_spi_ctrl|= FLD_SPI_MASTER_MODE_EN ; //0x09: bit1 enables master mode
	reg_spi_inv_clk&= FLD_SPI_MODE_WORK_MODE;// clear spi working mode
	reg_spi_inv_clk |= Mode;// select SPI mode,surpport four modes
}

/**
 *  @brief  This function configures the spi pins for a master device
 */
void spi_master_set_pin(SPI_GPIO_GroupTypeDef PinGrp)
{
	GPIO_PinTypeDef sclk = GPIO_PB3 ,cs=GPIO_PB0 ;
	GPIO_PinTypeDef sdo= GPIO_PB1   ,sdi=GPIO_PB2;

	if( PinGrp== SPI_GPIO_GROUP_B0B1B2B3 )
	{
		 gpio_set_func( GPIO_PC3 ,AS_GPIO);
		 gpio_set_func( GPIO_PC4 ,AS_GPIO);
		 gpio_set_func( GPIO_PC5 ,AS_GPIO);

	}
	else if(PinGrp == SPI_GPIO_GROUP_C2C3C4C5)
	{
		cs   = GPIO_PC2;
		sdo  = GPIO_PC3;
		sdi  = GPIO_PC4;
		sclk = GPIO_PC5;

		gpio_set_func( GPIO_PB1 ,AS_GPIO);
		gpio_set_func( GPIO_PB2 ,AS_GPIO);
		gpio_set_func( GPIO_PB3 ,AS_GPIO);
	}

	gpio_set_func(sdo  ,AS_SPI_MDO);
	gpio_set_func(sdi  ,AS_SPI_MDI);
	gpio_set_func(sclk ,AS_SPI_MCK);

	gpio_set_input_en(sdo, 1);
	gpio_set_input_en(sdi, 1);
	gpio_set_input_en(sclk, 1);


	gpio_set_data_strength(sdo ,0);
	gpio_set_data_strength(sdi,0);
	gpio_set_data_strength(sclk,0);

	spi_master_set_cs_pin(cs);
}


/**
 * @brief      This function serves to write a bulk of data to the SPI slave
 *             device specified by the CS pin
 * @param[in]  Addr - pointer to the target address  needed written into the
 *             slave device first before the writing operation of actual data
 * @param[in]  AddrLen - length in byte of the address of slave device
 * @param[in]  Data - pointer to the data need to write
 * @param[in]  DataLen - length in byte of the data need to write
 * @param[in]  CSPin - the CS pin specifing the slave device
 * @return     none
 */
void spi_write_buff(unsigned int Addr, unsigned char AddrLen,  unsigned char *Data, int DataLen, GPIO_PinTypeDef CSPin)
{
   int i = 0;
   gpio_write(CSPin,0);
   reg_spi_ctrl &= ~(FLD_SPI_DATA_OUT_DIS|FLD_SPI_RD);/* Enable SPI data output and SPI write command. */

   /***write Cmd***/
   for (i = 0; i < AddrLen; i++) {
	   reg_spi_data = (Addr>>((AddrLen-i-1)*8))&0xff;
	   while(reg_spi_ctrl& FLD_SPI_BUSY); //wait writing finished
   }

   reg_spi_data = SPI_WRITE_CMD;			   		// write cmd:0x00
   while(reg_spi_ctrl& FLD_SPI_BUSY);

   /***write Data***/
   for (i = 0; i < DataLen; i++) {
	   reg_spi_data = Data[i];
	   while(reg_spi_ctrl & FLD_SPI_BUSY); //wait writing finished
  }

   /***pull up CS***/
   gpio_write(CSPin,1);//CS level is high

}
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
void spi_read_buff(unsigned int Addr, unsigned char AddrLen, unsigned char *Data, int DataLen, GPIO_PinTypeDef CSPin)
{
	 int i = 0;
	 unsigned char temp = 0;
	 gpio_write(CSPin,0); //CS level is low
	 reg_spi_ctrl &= ~FLD_SPI_DATA_OUT_DIS; ////0x09- bit2 enables spi data output

	/***write cmd***/
	 for (i = 0; i <  AddrLen; i++) {
		 reg_spi_data = (Addr>>((AddrLen-i-1)*8))&0xff;
		 while(reg_spi_ctrl& FLD_SPI_BUSY ); //wait writing finished
	}

	reg_spi_data = SPI_READ_CMD;			   		//read cmd:0x80
	while(reg_spi_ctrl& FLD_SPI_BUSY);

	/***when the read_bit was set 1,you can read 0x800008 to take eight clock cycle***/
	 reg_spi_ctrl |= FLD_SPI_RD; //enable read,0x09-bit3 : 0 for read ,1 for write
	 temp = reg_spi_data; //first byte isn't useful data,only take 8 clock cycle
	 while(reg_spi_ctrl &FLD_SPI_BUSY ); //wait reading finished

	 /***read data***/
	 for (i = 0; i < DataLen; i++) {
		 Data[i] = reg_spi_data; //take 8 clock cycles
	     while(reg_spi_ctrl & FLD_SPI_BUSY ); //wait reading finished
	 }
	 //pull up CS
	 gpio_write(CSPin,1);//CS level is high
}
/**
 * @brief     This function selects a GPIO pin as CS of SPI function.
 * @param[in] CSPin - the selected CS pin
 * @return    none
 */
void spi_master_set_cs_pin(GPIO_PinTypeDef CSPin)
{
	gpio_set_func(CSPin,AS_GPIO);//enable GPIO function
	gpio_set_input_en(CSPin,0); //disable input function
	gpio_set_output_en(CSPin,1);//enable out put
	gpio_write(CSPin,1);//output high level in idle state
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
void spi_slave_init(unsigned char DivClock, SPI_ModeTypeDef Mode)
{

	reg_clk_en0 |= FLD_CLK0_SPI_EN;//enable spi clock
	reg_spi_sp = 0;            //clear register(0x0a) value
	reg_spi_sp|= DivClock;     //0x0a: bit0~bit6 set spi clock ; spi clock=system clock/((DivClock+1)*2)
	reg_spi_sp|=FLD_SPI_ENABLE;//0x0a: bit7 enables spi function mode

	reg_spi_ctrl &= ~(FLD_SPI_MASTER_MODE_EN|FLD_SPI_SLAVE_EN);
	reg_spi_ctrl |= FLD_SPI_SLAVE_EN;//Enable SPI slave

	reg_spi_inv_clk&= FLD_SPI_MODE_WORK_MODE;// clear spi working mode
	reg_spi_inv_clk|= Mode; //select SPI mode,surpport four modes
}

/**
 *  @brief  This function sets the spi pins for a slave device
 */
void spi_slave_set_pin(SPI_GPIO_GroupTypeDef PinGrp)
{
	GPIO_PinTypeDef cs  = GPIO_PB0,sclk = GPIO_PB3;
	GPIO_PinTypeDef sdo = GPIO_PB1,sdi  = GPIO_PB2;

	if(PinGrp== SPI_GPIO_GROUP_C2C3A3A4){
        /*C2 C3 A3 A4*/
		cs   = GPIO_PC2;
		sdo  = GPIO_PC3;
		sdi  = GPIO_PA3;
		sclk = GPIO_PA4;

		gpio_set_func( GPIO_PC4 ,AS_GPIO);
		gpio_set_func( GPIO_PC5 ,AS_GPIO);
		gpio_set_func( GPIO_PB0 ,AS_GPIO);
		gpio_set_func( GPIO_PB1 ,AS_GPIO);
		gpio_set_func( GPIO_PB2 ,AS_GPIO);
		gpio_set_func( GPIO_PB3 ,AS_GPIO);

	}
	else if(PinGrp == SPI_GPIO_GROUP_C2C3C4C5){
		/*C2 C3 C3 C4*/
		cs   = GPIO_PC2;
		sdo  = GPIO_PC3;
		sdi  = GPIO_PC4;
		sclk = GPIO_PC5;

		gpio_set_func( GPIO_PA3 ,AS_GPIO);
		gpio_set_func( GPIO_PA4 ,AS_GPIO);
		gpio_set_func( GPIO_PB0 ,AS_GPIO);
		gpio_set_func( GPIO_PB1 ,AS_GPIO);
		gpio_set_func( GPIO_PB2 ,AS_GPIO);
		gpio_set_func( GPIO_PB3 ,AS_GPIO);

	}

	gpio_set_func(cs   ,AS_SPI_CN);
	gpio_set_func(sdo  ,AS_SPI_DO);
	gpio_set_func(sdi  ,AS_SPI_DI);
	gpio_set_func(sclk ,AS_SPI_CK);

	gpio_set_input_en(cs,  1);
	gpio_set_input_en(sdo, 1);
	gpio_set_input_en(sdi, 1);
	gpio_set_input_en(sclk,1);

	gpio_set_data_strength(cs,0);
	gpio_set_data_strength(sdo ,0);
	gpio_set_data_strength(sdi,0);
	gpio_set_data_strength(sclk,0);
}

/***
 * brief: this function can enable spi module interrupt.
 */
void spi_irq_enable(void){
	reg_clk_en0  |= FLD_CLK0_HOSTIRQ_EN;  // host irq enable; i2c and spi need to set this bit.
	reg_irq_mask |= FLD_IRQ_HOST_CMD_EN;  // i2c and spi need to set this bit

	irq_enable();  // enable IRQ
}

