/********************************************************************************************************
 * @file     i2c.c
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
#include "clock.h"
#include "i2c.h"
#include "gpio.h"
#include "common/string.h"
#include "irq.h"

/**
 * @brief      This function serves to select a pin port for I2C interface.
 * @param[in]  PinGrp - the pin port selected as I2C interface pin port.
 * @return     none
 */
void i2c_set_pin(I2C_GPIO_GroupTypeDef i2c_pin_group)
{
	GPIO_PinTypeDef sda, scl;

	if(i2c_pin_group == I2C_GPIO_GROUP_M_A3A4){

		scl = GPIO_PA3;
		sda = GPIO_PA4;

	}else if (i2c_pin_group ==I2C_GPIO_GROUP_M_A5A6 || i2c_pin_group ==I2C_GPIO_GROUP_S_A5A6 ){

		scl = GPIO_PA5;
		sda = GPIO_PA6;

	}else if (i2c_pin_group ==I2C_GPIO_GROUP_M_B2B3){

		scl = GPIO_PB2;
		sda = GPIO_PB3;

	}else if (i2c_pin_group ==I2C_GPIO_GROUP_M_B6B7){

		scl = GPIO_PB6;
		sda = GPIO_PB7;

	}else if (i2c_pin_group ==I2C_GPIO_GROUP_M_C4C5 || i2c_pin_group ==I2C_GPIO_GROUP_S_C4C5 ){

		scl = GPIO_PC5;
		sda = GPIO_PC4;

	}else if (i2c_pin_group ==I2C_GPIO_GROUP_S_A3A4){

		scl = GPIO_PA4;
		sda = GPIO_PA3;

	}
	else
	{ //ERR
		sda = 0;
		scl = 0;
	}
	gpio_setup_up_down_resistor(sda, PM_PIN_PULLUP_10K);
	gpio_setup_up_down_resistor(scl, PM_PIN_PULLUP_10K);

	if(i2c_pin_group ==I2C_GPIO_GROUP_M_A3A4 || i2c_pin_group ==I2C_GPIO_GROUP_M_A5A6 ||i2c_pin_group ==I2C_GPIO_GROUP_M_B2B3 ||i2c_pin_group ==I2C_GPIO_GROUP_M_B6B7 ||i2c_pin_group ==I2C_GPIO_GROUP_M_C4C5){

		gpio_set_func(sda, AS_I2C_MSD); ///AS_GPIO
		gpio_set_func(scl, AS_I2C_MCK);
	}
	else if (i2c_pin_group ==I2C_GPIO_GROUP_S_A3A4 || i2c_pin_group ==I2C_GPIO_GROUP_S_A5A6 || i2c_pin_group ==I2C_GPIO_GROUP_S_C4C5 ){

		gpio_set_func(sda, AS_I2C_SD);
		gpio_set_func(scl, AS_I2C_CK);
	}
	gpio_set_input_en(sda, 1);//enable sda input
	gpio_set_input_en(scl, 1);//enable scl input

}
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
void i2c_master_init(unsigned char SlaveID, unsigned char DivClock)
{
    reg_i2c_speed = DivClock; //i2c clock = system_clock/(4*DivClock)
    reg_i2c_id	  = SlaveID; //slave address
    reg_i2c_mode |= FLD_I2C_MASTER_EN; //enable master mode

    reg_clk_en0 |= FLD_CLK0_I2C_EN;    //enable i2c clock
    reg_spi_sp  &= ~FLD_SPI_ENABLE;   //force PADs act as I2C; i2c and spi share the hardware of IC

}
/**
 *  @brief      the function config the ID of slave and mode of slave.
 *  @param[in]  device_ID - it contains write or read bit,the lsb is write or read bit.
 *              ID|0x01 indicate read. ID&0xfe indicate write.
 *  @param[in]  mode - set slave mode. slave has two modes, one is DMA mode, the other is MAPPING mode.
 *  @param[in]  pBuf - if slave mode is MAPPING, set the first address of buffer master write or read slave.
 *  @return     none
 */
void i2c_slave_init(unsigned char device_ID,I2C_SlaveMode mode,unsigned char * pMapBuf)
{
	reg_i2c_slave_id 	 = device_ID; //slave device id


	reg_clk_en0 |= FLD_CLK0_I2C_EN;    //enable i2c clock
	reg_spi_sp  &= ~FLD_SPI_ENABLE;   //force PADs act as I2C; i2c and spi share the hardware of IC


	reg_i2c_mode &= (~FLD_I2C_MASTER_EN); //enable slave mode
	 //notice that: both dma and mapping mode need this to trigger data address auto increase(confirmed by congqing and sihui)
	reg_i2c_mode |= FLD_I2C_ADDR_AUTO_INC ;

	if(mode == I2C_SLAVE_MAP){
		reg_i2c_mode |= FLD_I2C_SLAVE_MAPPING ;
		reg_i2c_slave_map_addr = (unsigned int) pMapBuf & 0xffff;
	}
	else{
		reg_i2c_mode &= ~FLD_I2C_SLAVE_MAPPING;
	}
}

/**
 * @brief      This function serves to write one byte to the slave device at the specified address
 * @param[in]  Addr - i2c slave address where the one byte data will be written
 * @param[in]  AddrLen - length in byte of the address, which makes this function is
 *             compatible for slave device with both one-byte address and two-byte address
 * @param[in]  Data - the one byte data will be written via I2C interface
 * @return     none
 */
void i2c_dma_write_byte(unsigned int Addr, unsigned int AddrLen, unsigned char Data)
{

	reg_i2c_id	 &= (~FLD_I2C_WRITE_READ_BIT); //SlaveID & 0xfe,.i.e write data. R:High  W:Low

		//start + id(Write) + address

		if (AddrLen == 1) {
			reg_i2c_adr = (unsigned char)Addr;; //address
			//lanuch start /id/04    start
			reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_START);
		}
		else if (AddrLen == 2) {
			reg_i2c_adr = (unsigned char)(Addr>>8); //address high
			reg_i2c_do = (unsigned char)Addr; //address low
			//lanuch start /id/04/05    start
			reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_DO | FLD_I2C_CMD_START);
		}

		while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

		//write data
		reg_i2c_di = Data;
		reg_i2c_ctrl = FLD_I2C_CMD_DI; //launch data read cycle
		while(reg_i2c_status & FLD_I2C_CMD_BUSY	);
		//stop
		reg_i2c_ctrl = FLD_I2C_CMD_STOP; //launch stop cycle
		while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

}
/**
 * @brief      This function serves to read one byte from the slave device at the specified address
 * @param[in]  Addr - i2c slave address where the one byte data will be read
 * @param[in]  AddrLen - length in byte of the address, which makes this function is
 *             compatible for slave device with both one-byte address and two-byte address
 * @return     the one byte data read from the slave device via I2C interface
 */
unsigned char i2c_dma_read_byte(unsigned int Addr, unsigned int AddrLen)
{
	 unsigned char ret = 0;
	 reg_i2c_id	 &= (~FLD_I2C_WRITE_READ_BIT); //SlaveID & 0xfe,.i.e write data. R:High  W:Low


	 	//start + id(Write) + address

	 	if (AddrLen == 1) {
	 		reg_i2c_adr = (unsigned char)Addr;; //address
	 		//lanuch start /id/04    start
	 		reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_START);
	 	}
	 	else if (AddrLen == 2) {
	 		reg_i2c_adr = (unsigned char)(Addr>>8); //address high
	 		reg_i2c_do = (unsigned char)Addr; //address low
	 		//lanuch start /id/04/05    start
	 		reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_DO | FLD_I2C_CMD_START);
	 	}

	 	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

	 	//start + id(Read)
	 	reg_i2c_id	 |= FLD_I2C_WRITE_READ_BIT;  //SlaveID & 0xfe,.i.e write data. Read:High  Write:Low
	 	reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_START);
	 	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

	 	//read data
	 	 reg_i2c_ctrl = (FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID | FLD_I2C_CMD_ACK);
	 	 while(reg_i2c_status & FLD_I2C_CMD_BUSY	);
	 	 ret = reg_i2c_di;

	    //stop
	 	 reg_i2c_ctrl = FLD_I2C_CMD_STOP; //launch stop cycle
	 	 while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

	 	 return ret;
}

/**
 *  @brief      This function serves to write a packet of data to the specified address of slave device
 *  @param[in]  Addr - the register that master write data to slave in. support one byte and two bytes. i.e param2 AddrLen may be 1 or 2.
 *  @param[in]  AddrLen - the length of register. enum 0 or 1 or 2 or 3. based on the spec of i2c slave.
 *  @param[in]  dataBuf - the first SRAM buffer address to write data to slave in.
 *  @param[in]  dataLen - the length of data master write to slave.
 *  @return     none
 */
void i2c_dma_write_buff (unsigned int Addr, unsigned int AddrLen, unsigned char * dataBuf, int dataLen)
{
	reg_i2c_id	 &= (~FLD_I2C_WRITE_READ_BIT); //SlaveID & 0xfe,.i.e write data. R:High  W:Low

	//start + id(Write) + address

	if (AddrLen == 1) {
		reg_i2c_adr = (unsigned char)Addr;; //address
		//lanuch start /id/04    start
		reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_START);
	}
	else if (AddrLen == 2) {
		reg_i2c_adr = (unsigned char)(Addr>>8); //address high
		reg_i2c_do = (unsigned char)Addr; //address low
		//lanuch start /id/04/05    start
		reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_DO | FLD_I2C_CMD_START);
	}

	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

	//write data
	unsigned int buff_index = 0;
	for(buff_index=0;buff_index<dataLen;buff_index++){
		reg_i2c_di = dataBuf[buff_index];
		reg_i2c_ctrl = FLD_I2C_CMD_DI; //launch data read cycle
		while(reg_i2c_status & FLD_I2C_CMD_BUSY	);
	}


	//stop
	reg_i2c_ctrl = FLD_I2C_CMD_STOP; //launch stop cycle
	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

}
/**
 * @brief      This function serves to read a packet of data from the specified address of slave device
 * @param[in]  Addr - the register master read data from slave in. support one byte and two bytes.
 * @param[in]  AddrLen - the length of register. enum 0 or 1 or 2 or 3 based on the spec of i2c slave.
 * @param[in]  dataBuf - the first address of SRAM buffer master store data in.
 * @param[in]  dataLen - the length of data master read from slave.
 * @return     none.
 */
void i2c_dma_read_buff(unsigned int Addr, unsigned int AddrLen, unsigned char * dataBuf, int dataLen)
{
	reg_i2c_id	 &= (~FLD_I2C_WRITE_READ_BIT); //SlaveID & 0xfe,.i.e write data. R:High  W:Low


	//start + id(Write) + address

	if (AddrLen == 1) {
		reg_i2c_adr = (unsigned char)Addr;; //address
		//lanuch start /id/04    start
		reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_START);
	}
	else if (AddrLen == 2) {
		reg_i2c_adr = (unsigned char)(Addr>>8); //address high
		reg_i2c_do = (unsigned char)Addr; //address low
		//lanuch start /id/04/05    start
		reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_ADDR | FLD_I2C_CMD_DO | FLD_I2C_CMD_START);
	}

	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

	//start + id(Read)
	reg_i2c_id	 |= FLD_I2C_WRITE_READ_BIT;  //SlaveID & 0xfe,.i.e write data. Read:High  Write:Low
	reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_START);
	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);


	//read data
	unsigned int bufIndex = 0;

	dataLen--;    //the length of reading data must larger than 0
	//if not the last byte master read slave, master wACK to slave
	while(dataLen){  //
		reg_i2c_ctrl = (FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID);
		while(reg_i2c_status & FLD_I2C_CMD_BUSY	);
		dataBuf[bufIndex] = reg_i2c_di;
		bufIndex++;
		dataLen--;
	}
	//when the last byte, master will ACK to slave
	reg_i2c_ctrl = (FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID |FLD_I2C_CMD_ACK);
	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);
	dataBuf[bufIndex] = reg_i2c_di;

	//termiante
	reg_i2c_ctrl = FLD_I2C_CMD_STOP; //launch stop cycle
	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

}

/**
 *  @brief      This function serves to write a packet of data to slave device working in mapping mode
 *  @param[in]  dataBuf - the first SRAM buffer address to write data to slave in.
 *  @param[in]  dataLen - the length of data master write to slave.
 *  @return     none
 */
void i2c_map_write_buff(unsigned char * dataBuf, int dataLen)
{
	reg_i2c_id	 &= (~FLD_I2C_WRITE_READ_BIT); //SlaveID & 0xfe,.i.e write data. R:High  W:Low

	 //lanuch start /id    start
	reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_START );
	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

	//write data
	unsigned int buff_index = 0;
	for(buff_index=0;buff_index<dataLen;buff_index++){
		reg_i2c_di = dataBuf[buff_index];
		reg_i2c_ctrl = FLD_I2C_CMD_DI; //launch data read cycle
		while(reg_i2c_status & FLD_I2C_CMD_BUSY	);
	}

	  //stop
	    reg_i2c_ctrl = FLD_I2C_CMD_STOP; //launch stop cycle
	    while(reg_i2c_status & FLD_I2C_CMD_BUSY	);

}

/**
 * @brief      This function serves to read a packet of data from slave device working in mapping mode
 * @param[in]  dataBuf - the first address of SRAM buffer master store data in.
 * @param[in]  dataLen - the length of data master read from slave.
 * @return     none.
 */
void i2c_map_read_buff(unsigned char * dataBuf, int dataLen)
{
	reg_i2c_id	 &= (~FLD_I2C_WRITE_READ_BIT); //SlaveID & 0xfe,.i.e write data. R:High  W:Low
	 //lanuch start /id    start
	 reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_START );
	 while(reg_i2c_status & FLD_I2C_CMD_BUSY	);
	 //start + id(Read)
	 	reg_i2c_id	 |= FLD_I2C_WRITE_READ_BIT;  //SlaveID & 0xfe,.i.e write data. Read:High  Write:Low
	 	reg_i2c_ctrl = (FLD_I2C_CMD_ID | FLD_I2C_CMD_START);
	 	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);


	 	//read data
	 	unsigned int bufIndex = 0;

	 	dataLen--;    //the length of reading data must larger than 0
	 	//if not the last byte master read slave, master wACK to slave
	 	while(dataLen){  //
	 		reg_i2c_ctrl = (FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID);
	 		while(reg_i2c_status & FLD_I2C_CMD_BUSY	);
	 		dataBuf[bufIndex] = reg_i2c_di;
	 		bufIndex++;
	 		dataLen--;
	 	}
	 	//when the last byte, master will ACK to slave
	 	reg_i2c_ctrl = (FLD_I2C_CMD_DI | FLD_I2C_CMD_READ_ID |FLD_I2C_CMD_ACK);
	 	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);
	 	dataBuf[bufIndex] = reg_i2c_di;

	 	//termiante
	 	reg_i2c_ctrl = FLD_I2C_CMD_STOP; //launch stop cycle
	 	while(reg_i2c_status & FLD_I2C_CMD_BUSY	);
}

/***
 * brief: this function can enable i2c module interrupt.
 */
void i2c_irq_enable(void){
	reg_clk_en0  |= FLD_CLK0_HOSTIRQ_EN;  // host irq enable; i2c and spi need to set this bit.
	reg_irq_mask |= FLD_IRQ_HOST_CMD_EN;  // i2c interrupt

	irq_enable();  // enable IRQ
}

/****
 * brief: this function will return I2C irq state
 * 1: i2c read interrupt; 2: i2c write interrupt; 0: no i2c interrupt.
 */
I2C_IrqFlagTypeDef i2c_get_irq_state(void){
	if( reg_i2c_slave_irq_status &FLD_HOST_CMD_IRQ  ) {
		if( reg_i2c_slave_irq_status & FLD_HOST_READ_IRQ ){
			return I2C_Read_IrqFlag;
		}
		return I2C_Write_IrqFlag;
	}
	return I2C_Irq_None;
}



