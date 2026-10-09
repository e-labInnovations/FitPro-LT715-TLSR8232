/********************************************************************************************************
 * @file     register.h
 *
 * @brief    This is the header file for TLSR8232
 *
 * @author	 BLE Group
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

#if(__TL_LIB_5316__ || MCU_CORE_TYPE == MCU_CORE_5316)

#pragma once

#include "bsp.h"

#if defined(__cplusplus)
	extern "C" {
#endif


/*----------------------------------------------------------------------------*/
/*------- IIC register define                                         --------*/
/*----------------------------------------------------------------------------*/
#define reg_i2c_set				REG_ADDR32(0x00)
#define reg_i2c_speed			REG_ADDR8(0x00)
#define reg_i2c_id		        REG_ADDR8(0x01)//I2C Master device Id
#define reg_i2c_slave_id        REG_ADDR8(0x28)//I2C Slave device Id
enum //Use for "reg_i2c_id" and "reg_i2c_slave_id"
{
	FLD_I2C_WRITE_READ_BIT  = BIT(0),
	FLD_I2C_ID              = BIT_RNG(1,7),
};

#define reg_i2c_status			REG_ADDR8(0x02)
enum
{
	FLD_I2C_CMD_BUSY = BIT(0),
	FLD_I2C_BUS_BUSY = BIT(1),
	FLD_I2C_NAK		 = BIT(2),
};

#define reg_i2c_mode			REG_ADDR8(0x03)
enum
{
	FLD_I2C_ADDR_AUTO_INC   = BIT(0),
	FLD_I2C_MASTER_EN		= BIT(1),//1:Enable master.
	FLD_I2C_SLAVE_MAPPING   = BIT(2),//1:Mapping mode;0:DMA mode.
	FLD_I2C_HOLD_MASTER     = BIT(3),
	FLD_I2C_SLAVE_EN        = BIT(4),//1:Enable slave.
};

#define reg_i2c_adr_dat			REG_ADDR16(0x04)
#define reg_i2c_dat_ctrl		REG_ADDR32(0x04)
#define reg_i2c_di_ctrl			REG_ADDR16(0x06)
#define reg_i2c_adr				REG_ADDR8(0x04)
#define reg_i2c_do				REG_ADDR8(0x05)
#define reg_i2c_di				REG_ADDR8(0x06)
#define reg_i2c_ctrl			REG_ADDR8(0x07)
enum //Use for "reg_i2c_ctrl"
{
	FLD_I2C_CMD_ID		= BIT(0),
	FLD_I2C_CMD_ADDR	= BIT(1),
	FLD_I2C_CMD_DO		= BIT(2),
	FLD_I2C_CMD_DI		= BIT(3),
	FLD_I2C_CMD_START	= BIT(4),
	FLD_I2C_CMD_STOP	= BIT(5),
	FLD_I2C_CMD_READ_ID	= BIT(6),
	FLD_I2C_CMD_ACK		= BIT(7),
};

#define reg_i2c_slave_irq_status		REG_ADDR8(0x21)
#define reg_spi_slave_irq_status 		REG_ADDR8(0x21)
enum{
	FLD_HOST_CMD_IRQ 	= 	BIT(0), ///both host read & write can trigger this interrupt
	FLD_HOST_READ_IRQ	= 	BIT(1), ///just host read can trigger this interrupt.
};

#define reg_i2c_slave_map_addr		REG_ADDR16(0x22)
#define reg_i2c_slave_id        	REG_ADDR8(0x28)//I2C Slave device Id

/*----------------------------------------------------------------------------*/
/*------- SPI register define                                         --------*/
/*----------------------------------------------------------------------------*/
#define reg_spi_data			REG_ADDR8(0x08)
#define reg_spi_ctrl			REG_ADDR8(0x09)
enum
{
	FLD_SPI_CS           		= BIT(0),
	FLD_SPI_MASTER_MODE_EN    	= BIT(1),
	FLD_SPI_DATA_OUT_DIS 		= BIT(2),
	FLD_SPI_RD           		= BIT(3),
	FLD_SPI_ADDR_AUTO_EN 		= BIT(4),
	FLD_SPI_SHARE_MODE 			= BIT(5),
	FLD_SPI_SLAVE_EN     		= BIT(6),
	FLD_SPI_BUSY         		= BIT(7),
};

#define reg_spi_sp				REG_ADDR8(0x0a)
enum{
	FLD_MASTER_SPI_CLK = BIT_RNG(0,6),
	FLD_SPI_ENABLE     = BIT(7),
};

#define reg_spi_inv_clk			REG_ADDR8(0x0b)
enum {
	FLD_SPI_MODE_WORK_MODE = BIT_RNG(0,1),
};


/*----------------------------------------------------------------------------*/
/*------- MSPI(Memory SPI) register define                            --------*/
/*----------------------------------------------------------------------------*/
#define reg_master_spi_data		REG_ADDR8(0x0c)
#define reg_master_spi_ctrl		REG_ADDR8(0x0d)
enum //Use for "reg_master_spi_ctrl"
{
	FLD_MASTER_SPI_CS   = BIT(0),
	FLD_MASTER_SPI_SDO  = BIT(1),
	FLD_MASTER_SPI_CONT = BIT(2),
	FLD_MASTER_SPI_RD   = BIT(3),
	FLD_MASTER_SPI_BUSY = BIT(4),
};

#define reg_mspi_mode      		REG_ADDR8(0x0f)
enum
{
	FLD_MSPI_Mode_Dual_Data  = BIT(0),
	FLD_MSPI_Mode_Dual_Addr  = BIT(1),
	FLD_MSPI_Speed     	     = BIT_RNG(2,7),
};


/****************************************************
 otp regs struct: begin  addr : 0x10
 *****************************************************/
#define reg_otp_addr_para		REG_ADDR16(0x10)
enum{
	FLD_OTP_PARA_ADDR = BIT_RNG(0,12),
	FLD_OTP_PARA_PTM  = BIT_RNG(13,15),
};

#define reg_otp_ctrl			REG_ADDR8(0x12)
enum{
	FLD_OTP_CTRL_PCEN = BIT(0),
	FLD_OTP_FAST_CLK  = BIT(1),
	FLD_OTP_OEN       = BIT(2),
	FLD_OTP_CLK       = BIT(3),
	FLD_OTP_PCEN_PWDN = BIT(4),
	FLD_OTP_WEN_PWDN  = BIT(5),
	FLD_OTP_OEN_PWDN  = BIT(6),
	FLD_OTP_CLK_PWDN  = BIT(7),
};

#define reg_otp_byte_dat		REG_ADDR8(0x13)
#define reg_otp_dat				REG_ADDR32(0x14)
#define reg_otp_blk_code		REG_ADDR8(0x18)


/*----------------------------------------------------------------------------*/
/*------ Reset And Clock register define                                ------*/
/*----------------------------------------------------------------------------*/
#define reg_rst0				REG_ADDR8(0x60)
enum{
	FLD_RST0_SPI = 				BIT(0),
	FLD_RST0_I2C = 				BIT(1),
	FLD_RST0_MCU = 				BIT(4),
	FLD_RST0_AIF = 				BIT(6),
	FLD_RST0_ZB = 				BIT(7),
};

#define reg_rst1				REG_ADDR8(0x61)
enum{
	FLD_RST1_SYS_TIMER = 		BIT(0),
	FLD_RST1_ALGM = 			BIT(1),
	FLD_RST1_DMA =				BIT(2),
	FLD_RST1_RS232 = 			BIT(3),
	FLD_RST1_PWM = 				BIT(4),
	FLD_RST1_AES = 				BIT(5),
	FLD_RST1_SWIRE = 			BIT(7),
};

#define reg_rst2				REG_ADDR8(0x62)
enum{
	FLD_RST2_ADC =				BIT(3),
	FLD_RST2_MCIC = 			BIT(4),
	FLD_RST2_SOFT =				BIT(5),
};


#define reg_clk_en0				REG_ADDR8(0x63)
enum{
	FLD_CLK0_SPI_EN = 			BIT(0),   // 31 uA
	FLD_CLK0_I2C_EN = 			BIT(1),   // 36 uA
	FLD_CLK0_HOSTIRQ_EN = 		BIT(2),   // 11 uA
	FLD_CLK0_MCU_EN = 			BIT(4),
	FLD_CLK0_FPU_EN = 			BIT(5),
	FLD_CLK0_AIF_EN = 			BIT(6),
	FLD_CLK0_ZB_EN = 			BIT(7),   // 140 uA

};

#define reg_clk_en1				REG_ADDR8(0x64)
enum{
	FLD_CLK1_SYS_TIMER_EN = 	BIT(0),
	FLD_CLK1_ALGM_EN = 			BIT(1),
	FLD_CLK1_DMA_EN = 			BIT(2),
	FLD_CLK1_RS232_EN = 		BIT(3),   // 77uA
	FLD_CLK1_PWM_EN = 			BIT(4),   // 65 uA
	FLD_CLK1_AES_EN = 			BIT(5),
	FLD_CLK1_32K_CLK_EN = 		BIT(6),
	FLD_CLK1_SWIRE_EN = 		BIT(7),

};

#define reg_clk_en2				REG_ADDR8(0x65)
enum{
	FLD_CLK2_32K_QDEC_EN = 		BIT(0),   // 20uA

	FLD_CLK2_MCIC_EN = 			BIT(4),   // add 900uA
	FLD_CLK2_QDEC_EN = 			BIT(5),   // 18 uA
};

#define reg_clk_sel				REG_ADDR8(0x66)
enum{
	FLD_CLK_SEL_FHS_DIV         = BIT_RNG(0,4),
	FLD_CLK_SEL_SYS_CLK_SRC_SEL = BIT_RNG(5,6),
	FLD_CLK_SEL_FHS_CLK_SRC_SEL = BIT(7),
};

#define reg_fhs_sel				REG_ADDR8(0x70)
enum{
	FLD_FHS_SEL = BIT(0),
};

#define reg_wakeup_en			REG_ADDR8(0x6e)
enum{

	//Digital wake-up source.
	FLD_WAKEUP_SRC_GPIO    = BIT(3),
	FLD_WAKEUP_SRC_QDEC    = BIT(4),
	FLD_WAKEUP_SRC_RST_SYS = BIT(7),
};

#define reg_pwdn_ctrl			REG_ADDR8(0x6f)
enum
{
	FLD_PWDN_CTRL_REBOOT = BIT(5),
	FLD_PWDN_CTRL_SLEEP  = BIT(7),
};

/* RESET and CLKEN register bit define. --------------------------------------*/

/*  RST0 register bit define. */
typedef union
{
	unsigned char All;
	struct
	{
		unsigned char SPI_Bit :1;
		unsigned char I2C_Bit :1;
		unsigned char :1;
		unsigned char :1;
		unsigned char MCU_Bit :1;
		unsigned char :1;
		unsigned char AIF_Bit :1;
		unsigned char ZB_Bit  :1;
	}Bit;
}RST0_Type;

/*  RST1 register bit define. */
typedef union
{
	unsigned char All;
	struct
	{
		unsigned char SysTimer_16M_Bit :1;
		unsigned char ALGM_Bit :1;
		unsigned char DMA_Bit  :1;
		unsigned char UART_Bit :1;
		unsigned char PWM_Bit  :1;
		unsigned char AES_Bit  :1;
		unsigned char :1;
		unsigned char SWS_Bit :1;
	}Bit;
}RST1_Type;

/*  RST2 register bit define. */
typedef union
{
	unsigned char All;
	struct
	{
		unsigned char :1;
		unsigned char :1;
		unsigned char :1;
		unsigned char ADC_Bit :1;
		unsigned char MCIC_Bit :1;
		unsigned char MCIC_SOFT_RST_Bit :1;
		unsigned char :1;
		unsigned char ALG_Bit :1;
	}Bit;
}RST2_Type;


/*  CLKEn0 register bit define. */
typedef union
{
	unsigned char All;
	struct
	{
		unsigned char SPI_Bit   : 1;
		unsigned char I2C_Bit   : 1;
		unsigned char HOST_IRQ  : 1;
		unsigned char  : 1;
		unsigned char MCU_Bit   : 1;
		unsigned char FPU_Bit   : 1;
		unsigned char AIF_Bit   : 1;
		unsigned char ZB_Bit    : 1;
	}Bit;
}CLKEn0_Type;

/*  CLKEn1 register bit define. */
typedef union
{
	unsigned char All;
	struct
	{
		unsigned char SysTimer_16M_Bit : 1;
		unsigned char ALGM_Bit		    : 1;
		unsigned char DMA_Bit          : 1;
		unsigned char UART_Bit         : 1;
		unsigned char PWM_Bit          : 1;
		unsigned char AES_Bit          : 1;
		unsigned char SysTimer_32K_Bit : 1;
		unsigned char SWS_Bit : 1;
	}Bit;
}CLKEn1_Type;

/*  CLKEn2 register bit define. */
typedef union
{
	unsigned char All;
	struct
	{
		unsigned char QDEC_32K_Bit : 1;
		unsigned char DFIFO_Bit: 1;
		unsigned char  : 1;
		unsigned char  : 1;
		unsigned char  MCIC_Bit: 1;
		unsigned char  QDEC_SysClk_Bit : 1;
		unsigned char  : 1;
		unsigned char  : 1;
	}Bit;
}CLKEn2_Type;


/* RESET and CLKEN register struct define. -----------------------------------*/

/*  RST register struct define. */
typedef struct
{
	RST0_Type   RST0;
	RST1_Type	RST1;
	RST2_Type   RST2;
}RESET_TypeDef;

/*  CLKEn register struct define. */
typedef struct
{
	CLKEn0_Type ClkEn0;
	CLKEn1_Type ClkEn1;
	CLKEn2_Type ClkEn2;
}CLK_TypeDef;

/* System Clock Select register bit define. */
typedef union
{
	volatile unsigned char ClkSelAll;
	struct
	{
		unsigned char SYS_CLK_SRC_DIV :5;
		unsigned char SYS_CLK_SRC     :2;
		unsigned char FHS_CLK_SRC_L   :1;
	}ClkSelBits;
}SYSCLK_TypeDef;

#define RESET  		  ((RESET_TypeDef*)(REG_BASE_ADDR + 0x60))
#define CLOCK_EN      ((CLK_TypeDef*)(REG_BASE_ADDR + 0x63))
#define SYSCLK_SEL    ((SYSCLK_TypeDef*)(REG_BASE_ADDR + 0x66))


/****************************************************
  OTP  addr : 0x71
 *****************************************************/
#define reg_dcdc_clk			REG_ADDR8(0x71)

#define reg_mcu_wakeup_mask		REG_ADDR32(0x78)


/*----------------------------------------------------------------------------*/
/*------  UART register define                                          ------*/
/*----------------------------------------------------------------------------*/
#define reg_uart_data_buf0		REG_ADDR8(0x90)
#define reg_uart_data_buf1		REG_ADDR8(0x91)
#define reg_uart_data_buf2		REG_ADDR8(0x92)
#define reg_uart_data_buf3		REG_ADDR8(0x93)

#define reg_uart_data_buf(i)    REG_ADDR8(0x90 + (i))
#define reg_uart_clk_div		REG_ADDR16(0x94)
enum{
	FLD_UART_CLK_DIV    = BIT_RNG(0,14),
	FLD_UART_CLK_DIV_EN = BIT(15)
};

#define reg_uart_ctrl0			REG_ADDR8(0x96)
enum{
	FLD_UART_BPWC = 			BIT_RNG(0,3),
	FLD_UART_RX_DMA_EN = 		BIT(4),
	FLD_UART_TX_DMA_EN =		BIT(5),
	FLD_UART_RX_IRQ_EN = 		BIT(6),
	FLD_UART_TX_IRQ_EN =		BIT(7),
};

#define reg_uart_ctrl1         		REG_ADDR8(0x97)
enum {
    FLD_UART_CTRL1_CTS_SELECT	   = BIT(0),
    FLD_UART_CTRL1_CTS_EN 		   = BIT(1),
    FLD_UART_CTRL1_PARITY_EN 	   = BIT(2),
    FLD_UART_CTRL1_PARITY_POLARITY = BIT(3),   //1:odd parity   0:even parity
    FLD_UART_CTRL1_STOP_BIT 	   = BIT_RNG(4,5),
    FLD_UART_CTRL1_TTL 			   = BIT(6),
    FLD_UART_CTRL1_LOOPBACK 	   = BIT(7),
};


#define reg_uart_ctrl2         REG_ADDR16(0x98)
enum {
    FLD_UART_CTRL2_RTS_TRIG_LVL   = BIT_RNG(0,3),
    FLD_UART_CTRL2_RTS_PARITY     = BIT(4),
    FLD_UART_CTRL2_RTS_MANUAL_VAL = BIT(5),
    FLD_UART_CTRL2_RTS_MANUAL_EN  = BIT(6),
    FLD_UART_CTRL2_RTS_EN         = BIT(7),
	FLD_UART_CTRL3_RX_IRQ_TRIG_LEVEL = BIT_RNG(8,11),
	FLD_UART_CTRL3_TX_IRQ_TRIG_LEVEL = BIT_RNG(12,15),
};

#define reg_uart_ctrl3        	REG_ADDR8(0x99)
enum {
	FLD_UART_RX_IRQ_TRIG_LEV = BIT_RNG(0,3),
	FLD_UART_TX_IRQ_TRIG_LEV = BIT_RNG(4,7),
};

#define reg_uart_rx_timeout0	REG_ADDR8(0x9a)
enum{
	FLD_UART_TIMEOUT_BW = 		BIT_RNG(0,7),		//  timeout bit width
};

#define reg_uart_rx_timeout1    REG_ADDR8(0x9b)
enum{
	FLD_UART_TIMEOUT_MUL	 = 	BIT_RNG(0,1),
};

#define reg_uart_buf_cnt       REG_ADDR8(0x9c)
enum{
	FLD_UART_RX_BUF_CNT		=  BIT_RNG(0,3),
	FLD_UART_TX_BUF_CNT		=  BIT_RNG(4,7),
};

#define reg_uart_status0       REG_ADDR8(0x9d)
enum{
	FLD_UART_RBCNT 	     =  BIT_RNG(0,2),
	FLD_UART_IRQ_FLAG    =  BIT(3),
	FLD_UART_WBCNT 	     =  BIT_RNG(4,6),
	FLD_UART_RX_ERR_CLR  =  BIT(6),
	FLD_UART_RX_ERR_FLAG =  BIT(7),
};

#define reg_uart_status1       REG_ADDR8(0x9e)
enum{
	FLD_UART_TX_DONE   	  =  BIT(0),
	FLD_UART_TX_BUF_IRQ   =  BIT(1),
	FLD_UART_RX_DONE   	  =  BIT(2),
	FLD_UART_RX_BUF_IRQ   =  BIT(3),
};

#define reg_uart_state       REG_ADDR8(0x9f)
enum{
	FLD_UART_TSTATE_I 	     =  BIT_RNG(0,2),
	FLD_UART_RSTATE_I	     =  BIT_RNG(4,7),
};

/*----------------------------------------------------------------------------*/
/*------  SWS register define                                           ------*/
/*----------------------------------------------------------------------------*/
#define reg_swire_data			REG_ADDR8(0xb0)
#define reg_swire_ctrl1			REG_ADDR8(0xb1)
enum //Use for "reg_swire_ctrl1"
{
	FLD_SWIRE_WR      = BIT(0),
	FLD_SWIRE_RD      = BIT(1),
	FLD_SWIRE_CMD     =	BIT(2),
	FLD_SWIRE_USB_DET =	BIT(6),
	FLD_SWIRE_USB_EN  = BIT(7),
};

#define reg_swire_clk_div		REG_ADDR8(0xb2)
enum
{
	FLD_SWIRE_CLK_DIV_Pos = BIT_RNG(0,6),
};

#define reg_swire_id      		REG_ADDR8(0xb3)
enum
{
	FLD_SWIRE_ID_SLAVE_ID_Pos      = BIT_RNG(0,6),
	FLD_SWIRE_ID_SLAVE_FIFO_EN_Pos = BIT(7),
};

/*----------------------------------------------------------------------------*/
/*------  Analog register(ALG) Read/Write Control register define       ------*/
/*----------------------------------------------------------------------------*/
#define reg_ana_ctrl32			REG_ADDR32(0xb8)	// for performance, set addr and data at a time
#define reg_ana_addr_data		REG_ADDR16(0xb8)	// for performance, set addr and data at a time
#define reg_ana_addr			REG_ADDR8(0xb8)
#define reg_ana_data			REG_ADDR8(0xb9)
#define reg_ana_ctrl			REG_ADDR8(0xba)
enum // datasheet is not right. please use the following define.Use for "reg_ana_ctrl"
{
	FLD_ANA_BUSY  = BIT(0),
	FLD_ANA_RSV	  =	BIT(4),
	FLD_ANA_RW    = BIT(5),
	FLD_ANA_START = BIT(6),
	FLD_ANA_CYC   = BIT(7),
};

/****************************************************
 USB regs struct: begin  addr : 0x100
 *****************************************************/
#define reg_ctrl_ep_ptr			REG_ADDR8(0x100)
#define reg_ctrl_ep_dat			REG_ADDR8(0x101)
#define reg_ctrl_ep_ctrl		REG_ADDR8(0x102)

// same for all endpoints
enum{
	FLD_EP_DAT_ACK   = BIT(0),
	FLD_EP_DAT_STALL = BIT(1),
	FLD_EP_STA_ACK   = BIT(2),
	FLD_EP_STA_STALL = BIT(3),
};

#define reg_ctrl_ep_irq_sta		REG_ADDR8(0x103)
enum{
	FLD_CTRL_EP_IRQ_TRANS = BIT_RNG(0,3),
	FLD_CTRL_EP_IRQ_SETUP =	BIT(4),
	FLD_CTRL_EP_IRQ_DATA  =	BIT(5),
	FLD_CTRL_EP_IRQ_STA   = BIT(6),
	FLD_CTRL_EP_IRQ_INTF  = BIT(7),
};

#define reg_ctrl_ep_irq_mode	REG_ADDR8(0x104)
enum{
	FLD_CTRL_EP_AUTO_ADDR = BIT(0),
	FLD_CTRL_EP_AUTO_CFG  =	BIT(1),
	FLD_CTRL_EP_AUTO_INTF =	BIT(2),
	FLD_CTRL_EP_AUTO_STA  =	BIT(3),
	FLD_CTRL_EP_AUTO_SYN  =	BIT(4),
	FLD_CTRL_EP_AUTO_DESC =	BIT(5),
	FLD_CTRL_EP_AUTO_FEAT =	BIT(6),
	FLD_CTRL_EP_AUTO_STD  =	BIT(7),
};

#define reg_usb_ctrl			REG_ADDR8(0x105)
enum{
	FLD_USB_CTRL_AUTO_CLK = BIT(0),
	FLD_USB_CTRL_LOW_SPD  = BIT(1),
	FLD_USB_CTRL_LOW_JITT =	BIT(2),
	FLD_USB_CTRL_TST_MODE = BIT(3),
};

#define reg_usb_cyc_cali		REG_ADDR16(0x106)
#define reg_usb_mdev			REG_ADDR8(0x10a)
#define reg_usb_host_conn		REG_ADDR8(0x10b)
enum{
	FLD_USB_MDEV_SELF_PWR = BIT(0),
	FLD_USB_MDEV_SUSP_STA = BIT(1),
};

#define reg_usb_sups_cyc_cali	REG_ADDR8(0x10c)
#define reg_usb_intf_alt		REG_ADDR8(0x10d)

#define reg_usb_ep8123_ptr		REG_ADDR32(0x110)
#define reg_usb_ep8_ptr			REG_ADDR8(0x110)
#define reg_usb_ep1_ptr			REG_ADDR8(0x111)
#define reg_usb_ep2_ptr			REG_ADDR8(0x112)
#define reg_usb_ep3_ptr			REG_ADDR8(0x113)
#define reg_usb_ep4567_ptr		REG_ADDR32(0x114)
#define reg_usb_ep4_ptr			REG_ADDR8(0x114)
#define reg_usb_ep5_ptr			REG_ADDR8(0x115)
#define reg_usb_ep6_ptr			REG_ADDR8(0x116)
#define reg_usb_ep7_ptr			REG_ADDR8(0x117)
#define reg_usb_ep_ptr(i)		REG_ADDR8(0x110+((i) & 0x07))

#define reg_usb_ep8123_dat		REG_ADDR32(0x118)
#define reg_usb_ep8_dat			REG_ADDR8(0x118)
#define reg_usb_ep1_dat			REG_ADDR8(0x119)
#define reg_usb_ep2_dat			REG_ADDR8(0x11a)
#define reg_usb_ep3_dat			REG_ADDR8(0x11b)
#define reg_usb_ep4567_dat		REG_ADDR32(0x11c)
#define reg_usb_ep4_dat			REG_ADDR8(0x11c)
#define reg_usb_ep5_dat			REG_ADDR8(0x11d)
#define reg_usb_ep6_dat			REG_ADDR8(0x11e)
#define reg_usb_ep7_dat			REG_ADDR8(0x11f)
#define reg_usb_ep_dat(i)		REG_ADDR8(0x118+((i) & 0x07))

#define reg_usb_ep8_ctrl		REG_ADDR8(0x120)
#define reg_usb_ep1_ctrl		REG_ADDR8(0x121)
#define reg_usb_ep2_ctrl		REG_ADDR8(0x122)
#define reg_usb_ep3_ctrl		REG_ADDR8(0x123)
#define reg_usb_ep4_ctrl		REG_ADDR8(0x124)
#define reg_usb_ep5_ctrl		REG_ADDR8(0x125)
#define reg_usb_ep6_ctrl		REG_ADDR8(0x126)
#define reg_usb_ep7_ctrl		REG_ADDR8(0x127)
#define reg_usb_ep_ctrl(i)		REG_ADDR8(0x120+((i) & 0x07))

enum{
	FLD_USB_EP_BUSY    = BIT(0),
	FLD_USB_EP_STALL   = BIT(1),
	FLD_USB_EP_DAT0    = BIT(2),
	FLD_USB_EP_DAT1    = BIT(3),
	FLD_USB_EP_MONO    = BIT(6),
	FLD_USB_EP_EOF_ISO = BIT(7),
};

#define reg_usb_ep8123_buf_addr	REG_ADDR32(0x128)
#define reg_usb_ep8_buf_addr	REG_ADDR8(0x128)
#define reg_usb_ep1_buf_addr	REG_ADDR8(0x129)
#define reg_usb_ep2_buf_addr	REG_ADDR8(0x12a)
#define reg_usb_ep3_buf_addr	REG_ADDR8(0x12b)
#define reg_usb_ep4567_buf_addr	REG_ADDR32(0x12c)
#define reg_usb_ep4_buf_addr	REG_ADDR8(0x12c)
#define reg_usb_ep5_buf_addr	REG_ADDR8(0x12d)
#define reg_usb_ep6_buf_addr	REG_ADDR8(0x12e)
#define reg_usb_ep7_buf_addr	REG_ADDR8(0x12f)
#define reg_usb_ep_buf_addr(i)	REG_ADDR8(0x128+((i) & 0x07))

#define reg_usb_ram_ctrl		REG_ADDR8(0x130)
enum{
	FLD_USB_CEN_PWR_DN = BIT(0),
	FLD_USB_CLK_PWR_DN = BIT(1),
	FLD_USB_WEN_PWR_DN = BIT(3),
	FLD_USB_CEN_FUNC   = BIT(4),
};

#define reg_usb_iso_mode		REG_ADDR8(0x138)
#define reg_usb_irq				REG_ADDR8(0x139)
#define reg_usb_mask			REG_ADDR8(0x13a)
#define reg_usb_ep8_send_max	REG_ADDR8(0x13b)
#define reg_usb_ep8_send_thre	REG_ADDR8(0x13c)
#define reg_usb_ep8_fifo_mode	REG_ADDR8(0x13d)
#define reg_usb_ep_max_size		REG_ADDR8(0x13e)

enum{
	FLD_USB_ENP8_FIFO_MODE = BIT(0),
	FLD_USB_ENP8_FULL_FLAG = BIT(1),
};

/****************************************************
	RF : begin  addr : 0x4e8
 *****************************************************/
#define reg_rf_tx_mode1			REG_ADDR8(0x400)
#define reg_rf_tx_mode			REG_ADDR16(0x400)
enum{
	FLD_RF_TX_DMA_EN       = BIT(0),
	FLD_RF_TX_CRC_EN       = BIT(1),
	FLD_RF_TX_BANDWIDTH    = BIT_RNG(2,3),
	FLD_RF_TX_OUTPUT       = BIT(4),
	FLD_RF_TX_TST_OUT      = BIT(5),
	FLD_RF_TX_TST_EN       = BIT(6),
	FLD_RF_TX_TST_MODE     = BIT(7),
	FLD_RF_TX_ZB_PN_EN     = BIT(8),
	FLD_RF_TX_ZB_FEC_EN    = BIT(9),
	FLD_RF_TX_ZB_INTL_EN   = BIT(10),	// interleaving
	FLD_RF_TX_1M2M_PN_EN   = BIT(11),
	FLD_RF_TX_1M2M_FEC_EN  = BIT(12),
	FLD_RF_TX_1M2M_INTL_EN = BIT(13), 	// interleaving
};
#define reg_rf_tx_buf_sta		REG_ADDR32(0x41c)

#define reg_rf_rx_sense_thr		REG_ADDR8(0x422)
#define reg_rf_rx_auto			REG_ADDR8(0x426)
enum{
	FLD_RF_RX_IRR_GAIN  = BIT(0),
	FLD_RF_RX_IRR_PHASE = BIT(1),
	FLD_RF_RX_DAC_I     = BIT(2),
	FLD_RF_RX_DAC_Q     = BIT(3),
	FLD_RF_RX_LNA_GAIN  = BIT(4),
	FLD_RF_RX_MIX2_GAIN = BIT(5),
	FLD_RF_RX_PGA_GAIN  = BIT(6),
	FLD_RF_RX_CAL_EN    = BIT(7),
};
#define reg_rf_rx_sync			REG_ADDR8(0x427)
enum{
	FLD_RF_FREQ_COMP_EN     = BIT(0),
	FLD_RF_ADC_SYNC         = BIT(1),
	FLD_RF_ADC_INP_SIGNED   = BIT(2),
	FLD_RF_SWAP_ADC_IQ      = BIT(3),
	FLD_RF_NOTCH_FREQ_SEL   = BIT(4),
	FLD_RF_NOTCH_BAND_SEL   = BIT(5),
	FLD_RF_NOTCH_EN         = BIT(6),
	FLD_RF_DN_CONV_FREQ_SEL = BIT(7),
};

#define reg_rf_rx_mode			REG_ADDR8(0x428)
enum{
	FLD_RF_RX_EN              =	BIT(0),
	FLD_RF_RX_MODE_1M         =	BIT(1),
	FLD_RF_RX_MODE_2M         =	BIT(2),
	FLD_RF_RX_LOW_IF          =	BIT(3),
	FLD_RF_RX_BYPASS_DCOC     =	BIT(4),
	FLD_RF_RX_MAN_FINE_TUNE   = BIT(5),
	FLD_RF_RX_SINGLE_CAL      =	BIT(6),
	FLD_RF_RX_LOW_PASS_FILTER =	BIT(7),
};

#define reg_rf_rx_pilot			REG_ADDR8(0x42b)
enum{
	FLD_RF_PILOT_LEN  =	BIT_RNG(0,3),
	FLD_RF_ZB_SFD_CHK =	BIT(4),
	FLD_RF_1M_SFD_CHK =	BIT(5),
	FLD_RF_2M_SFD_CHK = BIT(6),
	FLD_RF_ZB_OR_AUTO = BIT(7),
};

#define reg_rf_rx_chn_dc		REG_ADDR32(0x42c)
#define reg_rf_rx_q_chn_cal		REG_ADDR8(0x42f)
enum{
	FLD_RF_RX_DCQ_HIGH      = BIT_RNG(0,6),
	FLD_RF_RX_DCQ_CAL_START = BIT(7),
};
#define reg_rf_rx_pel			REG_ADDR16(0x434)
#define reg_rf_rx_pel_gain		REG_ADDR32(0x434)
#define reg_rf_rx_rssi_offset	REG_ADDR8(0x439)

#define reg_rf_rx_hdx			REG_ADDR8(0x43b)
enum{
	FLD_RX_HEADER_LEN   = BIT_RNG(0,3),
	FLD_RT_TICK_LO_SEL  = BIT(4),
	FLD_RT_TICK_HI_SEL  = BIT(5),
	FLD_RT_TICK_FRAME   = BIT(6),
	FLD_PKT_LEN_OUTP_EN = BIT(7),
};

#define reg_rf_rx_gctl			REG_ADDR8(0x43c)
enum{
	FLD_RX_GCTL_CIC_SAT_LO_EN =	BIT(0),
	FLD_RX_GCTL_CIC_SAT_HI_EN = BIT(1),
	FLD_RX_GCTL_AUTO_PWR      =	BIT(2),
	FLD_RX_GCTL_ADC_RST_VAL   =	BIT(4),
	FLD_RX_GCTL_ADC_RST_EN    =	BIT(5),
	FLD_RX_GCTL_PWR_CHG_DET_S =	BIT(6),
	FLD_RX_GCTL_PWR_CHG_DET_N = BIT(7),
};
#define reg_rf_rx_peak			REG_ADDR8(0x43d)
enum{
	FLD_RX_PEAK_DET_SRC_EN = BIT_RNG(0,2),
	FLD_TX_PEAK_DET_EN     = BIT(3),
	FLD_PEAK_DET_NUM       = BIT_RNG(4,5),
	FLD_PEAK_MAX_CNT_PRD   = BIT_RNG(6,7),
};

#define reg_rf_rx_status		REG_ADDR8(0x443)
enum{
	FLD_RF_RX_STATE   =	BIT_RNG(0,3),
	FLD_RF_RX_STA_RSV = BIT_RNG(4,5),
	FLD_RF_RX_INTR    = BIT(6),
	FLD_RF_TX_INTR    =	BIT(7),
};

#define reg_rf_irq_mask			REG_ADDR16(0xf1c)
#define reg_rf_irq_status		REG_ADDR16(0xf20)
#define reg_rf_fsm_timeout		REG_ADDR32(0xf2c)

#define		CLEAR_ALL_RFIRQ_STATUS   ( reg_rf_irq_status = 0xffff )

enum{
	FLD_RF_IRQ_RX            = BIT(0),
	FLD_RF_IRQ_TX            = BIT(1),
	FLD_RF_IRQ_RX_TIMEOUT    = BIT(2),
	FLD_RF_IRQ_RX_CRC_2      = BIT(4),
	FLD_RF_IRQ_CMD_DONE      = BIT(5),
	FLD_RF_IRQ_FSM_TIMEOUT   = BIT(6),
	FLD_RF_IRQ_RETRY_HIT     = BIT(7),
	FLD_RF_IRQ_FIRST_TIMEOUT = BIT(10),
};


enum{
	FLD_RF_IRX_RX_TIMEOUT =	BIT(2),
	FLD_RF_IRX_CMD_DONE   =	BIT(5),
	FLD_RF_IRX_RETRY_HIT  =	BIT(7),
};

// The value for FLD_RF_RX_STATE
enum{
	RF_RX_STA_IDLE       = 0,
	RF_RX_STA_SET_GAIN   = 1,
	RF_RX_STA_CIC_SETTLE = 2,
	RF_RX_STA_LPF_SETTLE = 3,
	RF_RX_STA_PE         = 4,
	RF_RX_STA_SYN_START  = 5,
	RF_RX_STA_GLOB_SYN   = 6,
	RF_RX_STA_GLOB_LOCK  = 7,
	RF_RX_STA_LOCAL_SYN  = 8,
	RF_RX_STA_LOCAL_LOCK = 9,
	RF_RX_STA_ALIGN  = 10,
	RF_RX_STA_ADJUST = 11,
	RF_RX_STA_DEMOD  = 12,		// de modulation
	RF_RX_STA_FOOTER = 13,
};

#define reg_rx_rnd_mode			REG_ADDR8(0x447)
enum{
	FLD_RX_RND_SRC       = BIT(0),
	FLD_RX_RND_MANU_MODE = BIT(1),
	FLD_RX_RND_AUTO_RD   = BIT(2),
	FLD_RX_RND_FREE_MODE = BIT(3),
	FLD_RX_RND_CLK_DIV   = BIT_RNG(4,7),
};
#define reg_rnd_number			REG_ADDR16(0x448)

#define reg_bb_max_tick			REG_ADDR16(0x44c)
#define reg_rf_rtt				REG_ADDR32(0x454)
enum{
	FLD_RTT_CAL          = BIT_RNG(0,7),
	FLD_RTT_CYC1         = BIT_RNG(8,15),
	FLD_RTT_LOCK         = BIT_RNG(16,23),
	FLD_RT_SD_DLY_40M    = BIT_RNG(24,27),
	FLD_RT_SD_DLY_BYPASS = BIT(28),
};

#define reg_rf_chn_rssi			REG_ADDR8(0x458)
#define reg_rf_timestamp		REG_ADDR32(0x460)

#define reg_rf_rx_gain_agc(i)	REG_ADDR32(0x480+((i)<<2))

#define reg_rf_rx_dci			REG_ADDR8(0x4cb)	//  different from the document, why
#define reg_rf_rx_dcq			REG_ADDR8(0x4cf)	//  different from the document, why

#define reg_pll_rx_coarse_tune	REG_ADDR16(0x4d0)
#define reg_pll_rx_coarse_div	REG_ADDR8(0x4d2)
#define reg_pll_rx_fine_tune	REG_ADDR16(0x4d4)
#define reg_pll_rx_fine_div		REG_ADDR8(0x4d6)
#define reg_pll_tx_coarse_tune	REG_ADDR16(0x4d8)
#define reg_pll_tx_coarse_div	REG_ADDR8(0x4da)
#define reg_pll_tx_fine_tune	REG_ADDR16(0x4dc)
#define reg_pll_tx_fine_div		REG_ADDR8(0x4de)

#define reg_pll_rx_frac			REG_ADDR32(0x4e0)
#define reg_pll_tx_frac			REG_ADDR32(0x4e4)

#define reg_pll_tx_ctrl			REG_ADDR8(0x4e8)
#define reg_pll_ctrl16			REG_ADDR16(0x4e8)
#define reg_pll_ctrl			REG_ADDR32(0x4e8)
enum{
	FLD_PLL_TX_CYC0        =BIT(0),
	FLD_PLL_TX_SOF         =BIT(1),
	FLD_PLL_TX_CYC1        =BIT(2),
	FLD_PLL_TX_PRE_EN      =BIT(3),
	FLD_PLL_TX_VCO_EN      =BIT(4),
	FLD_PLL_TX_PWDN_DIV    =BIT(5),
	FLD_PLL_TX_MOD_EN      =BIT(6),
	FLD_PLL_TX_MOD_TRAN_EN =BIT(7),
	FLD_PLL_RX_CYC0        =BIT(8),
	FLD_PLL_RX_SOF         =BIT(9),
	FLD_PLL_RX_CYC1        =BIT(10),
	FLD_PLL_RX_PRES_EN     =BIT(11),
	FLD_PLL_RX_VCO_EN      =BIT(12),
	FLD_PLL_RX_PWDN_DIV    =BIT(13),
	FLD_PLL_RX_PEAK_EN     =BIT(14),
	FLD_PLL_RX_TP_CYC      =BIT(15),
	FLD_PLL_SD_RSTB        =BIT(16),
	FLD_PLL_SD_INTG_EN     =BIT(17),
	FLD_PLL_CP_TRI         =BIT(18),
	FLD_PLL_PWDN_INTG1     =BIT(19),
	FLD_PLL_PWDN_INTG2     =BIT(20),
	FLD_PLL_PWDN_INTG_DIV  =BIT(21),
	FLD_PLL_PEAK_DET_EN    =BIT(22),
	FLD_PLL_OPEN_LOOP_EN   =BIT(23),
	FLD_PLL_RX_TICK_EN     =BIT(24),
	FLD_PLL_TX_TICK_EN     =BIT(25),
	FLD_PLL_RX_ALWAYS_ON   =BIT(26),
	FLD_PLL_TX_ALWAYS_ON   =BIT(27),
	FLD_PLL_MANUAL_MODE_EN =BIT(28),
	FLD_PLL_CAL_DONE_EN    =BIT(29),
	FLD_PLL_LOCK_EN        =BIT(30),
};
#define reg_pll_rx_ctrl			REG_ADDR8(0x4e9)
enum{
	FLD_PLL_RX2_CYC0    =BIT(0),
	FLD_PLL_RX2_SOF     =BIT(1),
	FLD_PLL_RX2_CYC1    =BIT(2),
	FLD_PLL_RX2_PRES_EN =BIT(3),
	FLD_PLL_RX2_VCO_EN  =BIT(4),
	FLD_PLL_RX2_PD_DIV  =BIT(5),
	FLD_PLL_RX2_PEAK_EN =BIT(6),
	FLD_PLL_RX2_TP_CYC  =BIT(7),
};

#define reg_pll_ctrl_a			REG_ADDR8(0x4eb)
enum{
	FLD_PLL_A_RX_TICK_EN     =BIT(0),
	FLD_PLL_A_TX_TICK_EN     =BIT(1),
	FLD_PLL_A_RX_ALWAYS_ON   =BIT(2),
	FLD_PLL_A_TX_ALWAYS_ON   =BIT(3),
	FLD_PLL_A_MANUAL_MODE_EN =BIT(4),
	FLD_PLL_A_CAL_DONE_EN    =BIT(5),
	FLD_PLL_A_LOCK_EN        =BIT(6),
};
// pll polarity
#define reg_pll_pol_ctrl		REG_ADDR16(0x4ec)
enum{
	FLD_PLL_POL_TX_PRE_EN   =BIT(0),
	FLD_PLL_POL_TX_VCO_EN   =BIT(1),
	FLD_PLL_POL_TX_PD_DIV   =BIT(2),
	FLD_PLL_POL_MOD_EN      =BIT(3),
	FLD_PLL_POL_MOD_TRAN_EN =BIT(4),
	FLD_PLL_POL_RX_PRE_EN   =BIT(5),
	FLD_PLL_POL_RX_VCO_EN   =BIT(6),
	FLD_PLL_POL_RX_PD_DIV   =BIT(7),
	FLD_PLL_POL_SD_RSTB     =BIT(8),
	FLD_PLL_POL_SD_INTG_EN  =BIT(9),
	FLD_PLL_POL_CP_TRI      =BIT(10),
	FLD_PLL_POL_TX_SOF      =BIT(11),
	FLD_PLL_POL_RX_SOF      =BIT(12),
};

#define reg_rf_rx_cap			REG_ADDR16(0x4f0)		//  电容
#define reg_rf_tx_cap			REG_ADDR16(0x4f0)		//  电容

/*----------------------------------------------------------------------------*/
/*------  DMA register define                                           ------*/
/*----------------------------------------------------------------------------*/
#define reg_dma0_addr			REG_ADDR16(0x500)
#define reg_dma0_size			REG_ADDR8 (0x502)
#define reg_dma0_mode			REG_ADDR8 (0x503)
enum{ ///use for "reg_dma0_mode","reg_dma1_mode","reg_dma2_mode","reg_dma3_mode","reg_dma5_mode"
	FLD_DMA_1WR_0RD_MEM =		BIT(0),
	FLD_DMA_PINGPONG_EN =		BIT(1),
	FLD_DMA_FIFO_EN =			BIT(2),
	FLD_DMA_AUTO_MODE =			BIT(3),
	FLD_DMA_RSVD   =			BIT(4),
	FLD_DMA_BYTE_MODE =			BIT(5)
};
//FLD_DMA_BYTE_MODE_EN =BIT(13),
//FLD_DMA_FIFO8 	     =(BIT(15) | BIT(14) | FLD_DMA_WR_MEM | FLD_DMA_PINGPONG_EN),

#define reg_dma1_addr			REG_ADDR16(0x504)
#define reg_dma1_size			REG_ADDR8(0x506)
#define reg_dma1_mode			REG_ADDR8(0x507)

#define reg_dma2_addr			REG_ADDR16(0x508)
#define reg_dma2_size			REG_ADDR8(0x50a)
#define reg_dma2_mode			REG_ADDR8(0x50b)

#define reg_dma2_ctrl			REG_ADDR16(0x50a) ///stack use this macro.
enum //Use for "reg_dma2_ctrl"
{
	FLD_DMA_BUF_SIZE     =BIT_RNG(0,7),
	FLD_DMA_WR_MEM       =BIT(8),     ///stack use this macro.
};

#define reg_dma3_addr			REG_ADDR16(0x50c)
#define reg_dma3_size			REG_ADDR8(0x50e)
#define reg_dma3_mode			REG_ADDR8(0x50f)

#define reg_dma5_addr			REG_ADDR16(0x514)
#define reg_dma5_size			REG_ADDR8(0x516)
#define reg_dma5_mode			REG_ADDR8(0x517)



#define reg_dma_size(v)			REG_ADDR8(0x502+4*v)

// The default channel assignment
#define reg_dma_uart_rx_addr	reg_dma0_addr
#define reg_dma_uart_rx_size	reg_dma0_size
#define reg_dma_uart_rx_mode	reg_dma0_mode

#define reg_dma_uart_tx_addr	reg_dma1_addr
#define reg_dma_uart_tx_size	reg_dma1_size
#define reg_dma_uart_tx_mode	reg_dma1_mode

#define reg_dma_rf_rx_addr		reg_dma2_addr
#define reg_dma_rf_rx_size		reg_dma2_size
#define reg_dma_rf_rx_mode		reg_dma2_mode

#define reg_dma_rf_tx_addr		reg_dma3_addr
#define reg_dma_rf_tx_size		reg_dma3_size
#define reg_dma_rf_tx_mode		reg_dma3_mode

#define reg_dma_pwm_addr		reg_dma5_addr
#define reg_dma_pwm_size		reg_dma5_size
#define reg_dma_pwm_mode		reg_dma5_mode


#define reg_dma_chn_en			REG_ADDR8(0x520)
#define reg_dma_chn_irq_msk		REG_ADDR8(0x521)
#define reg_dma_tx_rdy0			REG_ADDR8(0x524)
#define reg_dma_tx_rdy1			REG_ADDR8(0x525)
#define reg_dma_rx_rdy0			REG_ADDR8(0x526)
#define reg_dma_rx_rdy1			REG_ADDR8(0x527)

#define reg_dma_irq_status		reg_dma_rx_rdy0

enum{
    FLD_DMA_CHN0 =	BIT(0),		FLD_DMA_CHN_UART_RX = BIT(0),
    FLD_DMA_CHN1 =	BIT(1),		FLD_DMA_CHN_UART_TX = BIT(1),
	FLD_DMA_CHN2 =	BIT(2),		FLD_DMA_CHN_RF_RX 	= BIT(2),
	FLD_DMA_CHN3 =	BIT(3),		FLD_DMA_CHN_RF_TX 	= BIT(3),
	FLD_DMA_CHN4 =	BIT(4),
	FLD_DMA_CHN5 =	BIT(5),		FLD_DMA_CHN_PWM 	= BIT(5),
	FLD_DMA_CHN6 =	BIT(6),
	FLD_DMA_CHN7 =	BIT(7),
};

#define reg_dma_rx_rptr			REG_ADDR8(0x528)
#define reg_dma_rx_wptr			REG_ADDR8(0x529)

#define reg_dma_tx_rptr			REG_ADDR8(0x52a)
#define reg_dma_tx_wptr			REG_ADDR8(0x52b)
#define reg_dma_tx_fifo			REG_ADDR16(0x52c)

enum{
	FLD_DMA_RPTR_CLR =			BIT(4),
	FLD_DMA_RPTR_NEXT =			BIT(5),
	FLD_DMA_RPTR_SET =			BIT(6),
};


#define reg_rf_manual_irq_status REG_ADDR16(0x526)   //Rx buf 0 data received
#define FLD_RF_MANUAL_IRQ_RX     BIT(2)


/*----------------------------------------------------------------------------*/
/*------  AES register define                                           ------*/
/*----------------------------------------------------------------------------*/
#define reg_aes_ctrl            REG_ADDR8(0x540)
enum
{	FLD_AES_Decrypt   = BIT(0),
	FLD_AES_Feed_Data = BIT(1),
	FLD_AES_Finished  = BIT(2),
};
#define reg_aes_data            REG_ADDR32(0x548)
#define reg_aes_key(key_id) 	REG_ADDR8(0x550 + key_id)
#define reg_aes_key0            REG_ADDR8(0x550)
#define reg_aes_key1            REG_ADDR8(0x551)
#define reg_aes_key2            REG_ADDR8(0x552)
#define reg_aes_key3            REG_ADDR8(0x553)
#define reg_aes_key4            REG_ADDR8(0x554)
#define reg_aes_key5            REG_ADDR8(0x555)
#define reg_aes_key6            REG_ADDR8(0x556)
#define reg_aes_key7            REG_ADDR8(0x557)
#define reg_aes_key8            REG_ADDR8(0x558)
#define reg_aes_key9            REG_ADDR8(0x559)
#define reg_aes_key10           REG_ADDR8(0x55a)
#define reg_aes_key11           REG_ADDR8(0x55b)
#define reg_aes_key12           REG_ADDR8(0x55c)
#define reg_aes_key13           REG_ADDR8(0x55d)
#define reg_aes_key14           REG_ADDR8(0x55e)
#define reg_aes_key15           REG_ADDR8(0x55f)


/*----------------------------------------------------------------------------*/
/*-------   GPIO register define                                      --------*/
/*----------------------------------------------------------------------------*/
#define reg_gpio_pa_in			REG_ADDR8(0x580)
#define reg_gpio_pa_ie			REG_ADDR8(0x581)
#define areg_gpio_pa5_6_7_ie	0xb6
#define reg_gpio_pa_oen			REG_ADDR8(0x582)
#define reg_gpio_pa_out			REG_ADDR8(0x583)
#define reg_gpio_pa_pol			REG_ADDR8(0x584)
#define reg_gpio_pa_ds			REG_ADDR8(0x585)
#define reg_gpio_pa5_6_7_ds		0xb8
#define reg_gpio_pa_gpio		REG_ADDR8(0x586)
#define reg_gpio_pa_irq_en		REG_ADDR8(0x587)

#define reg_gpio_pb_in			REG_ADDR8(0x588)
#define areg_gpio_pb_ie			0xb9
#define reg_gpio_pb_ie			REG_ADDR8(0x589)
#define reg_gpio_pb_oen			REG_ADDR8(0x58a)
#define reg_gpio_pb_out			REG_ADDR8(0x58b)
#define reg_gpio_pb_pol			REG_ADDR8(0x58c)
#define areg_gpio_pb_ds			0xbb
#define reg_gpio_pb_ds			REG_ADDR8(0x58d)
#define reg_gpio_pb_gpio		REG_ADDR8(0x58e)
#define reg_gpio_pb_irq_en		REG_ADDR8(0x58f)

#define reg_gpio_pc_in			REG_ADDR8(0x590)
#define reg_gpio_pc_ie			REG_ADDR8(0x591)
#define reg_gpio_pc_oen			REG_ADDR8(0x592)
#define reg_gpio_pc_out			REG_ADDR8(0x593)
#define reg_gpio_pc_pol			REG_ADDR8(0x594)
#define reg_gpio_pc_ds			REG_ADDR8(0x595)
#define reg_gpio_pc_gpio		REG_ADDR8(0x596)
#define reg_gpio_pc_irq_en		REG_ADDR8(0x597)


#define reg_gpio_pe_in			REG_ADDR8(0x5a0)
#define reg_gpio_pe_ie			REG_ADDR8(0x5a1)
#define reg_gpio_pe_oen			REG_ADDR8(0x5a2)
#define reg_gpio_pe_out			REG_ADDR8(0x5a3)
#define reg_gpio_pe_pol			REG_ADDR8(0x5a4)
#define reg_gpio_pe_ds			REG_ADDR8(0x5a5)
#define reg_gpio_pe_gpio		REG_ADDR8(0x5a6)
#define reg_gpio_pe_irq_en		REG_ADDR8(0x5a7)

#define reg_gpio_pc_setting1	REG_ADDR32(0x590)
#define reg_gpio_pc_setting2	REG_ADDR32(0x594)

typedef union
{
    unsigned short RegAll;
    struct
	{
		unsigned short P0_AF :2;
		unsigned short P1_AF :2;
		unsigned short P2_AF :2;
		unsigned short P3_AF :2;
		unsigned short P4_AF :2;
		unsigned short P5_AF :2;
		unsigned short P6_AF :2;
		unsigned short P7_AF :2;
	}RegBits;
}GPIO_AFTypeDef;

#define GPIOA_AF  ((GPIO_AFTypeDef *)(REG_BASE_ADDR + 0x5a8))
#define GPIOB_AF  ((GPIO_AFTypeDef *)(REG_BASE_ADDR + 0x5aa))
#define GPIOC_AF  ((GPIO_AFTypeDef *)(REG_BASE_ADDR + 0x5ac))

//5316 must
#define reg_gpio_wakeup_irq		REG_ADDR8(0x5b5)
enum{
	FLD_GPIO_WAKEUP_EN	  =	BIT(2),
	FLD_GPIO_INTERRUPT_EN =	BIT(3),
};

//5316 must
#define reg_gpio_pb_multi_func_select  REG_ADDR8(0x5b6)
enum
{
	FLD_PB_MULTI_FUNC_SEL = BIT(3),//1:UART; 0:SPI(PB0-PB3)
};

/*----------------------------------------------------------------------------*/
/*-------   Timer and Watchdog register define                        --------*/
/*----------------------------------------------------------------------------*/
#define reg_tmr_ctrl			REG_ADDR32(0x620)
#define reg_tmr_ctrl16			REG_ADDR16(0x620)		// 因为0x622 不要写
#define reg_tmr_ctrl8			REG_ADDR8(0x620)
enum{
	FLD_TMR0_EN     = BIT(0),
	FLD_TMR0_MODE   = BIT_RNG(1,2),
	FLD_TMR1_EN     = BIT(3),
	FLD_TMR1_MODE   = BIT_RNG(4,5),
	FLD_TMR2_EN     = BIT(6),
	FLD_TMR2_MODE   = BIT_RNG(7,8),
	FLD_TMR_WD_CAPT = BIT_RNG(9,22),
	FLD_TMR_WD_EN   = BIT(23),
	FLD_TMR0_STA    = BIT(24),
	FLD_TMR1_STA    = BIT(25),
	FLD_TMR2_STA    = BIT(26),
	FLD_CLR_WD      = BIT(27),
};

#define reg_tmr_sta				REG_ADDR8(0x623)
enum{
	FLD_TMR_STA_TMR0 = BIT(0),//write 1 clear.
	FLD_TMR_STA_TMR1 = BIT(1),
	FLD_TMR_STA_TMR2 = BIT(2),
	FLD_TMR_STA_WD   = BIT(3),
};

#define reg_tmr0_capt			REG_ADDR32(0x624)
#define reg_tmr1_capt			REG_ADDR32(0x628)
#define reg_tmr2_capt			REG_ADDR32(0x62c)
#define reg_tmr_capt(i)			REG_ADDR32(0x624 + ((i) << 2))
#define reg_tmr0_tick			REG_ADDR32(0x630)
#define reg_tmr1_tick			REG_ADDR32(0x634)
#define reg_tmr2_tick			REG_ADDR32(0x638)
#define reg_tmr_tick(i)			REG_ADDR32(0x630 + ((i) << 2))

//Watchdog
#define reg_watchdog_reset_flg  REG_ADDR8(0x72)
#define WATCHDOG_TIMEOUT_COEFF	18		//  check register definiton, 0x622
#define WATCHDOG_DISABLE		(reg_tmr_ctrl &= ~FLD_TMR_WD_EN)
#define WATCHDOG_CLEAR			(reg_tmr_sta  |= FLD_TMR_STA_WD)


/*----------------------------------------------------------------------------*/
/*-------   Interrupt registers define                                 -------*/
/*----------------------------------------------------------------------------*/
#define reg_irq_mask			REG_ADDR32(0x640)
enum//Use for "reg_irq_mask"
{
	FLD_IRQ_TMR0_EN       =	BIT(0),
	FLD_IRQ_TMR1_EN       =	BIT(1),
	FLD_IRQ_TMR2_EN       =	BIT(2),
	FLD_IRQ_SYSTEM_TIMER  =	BIT(3),
	FLD_IRQ_DMA_EN        =	BIT(4),
	FLD_IRQ_QDEC_EN       =	BIT(5),
	FLD_IRQ_UART_EN 	  =	BIT(6),
	FLD_IRQ_HOST_CMD_EN   =	BIT(7),

	FLD_IRQ_RSVD_8        =	BIT(8),
	FLD_IRQ_RSVD_9        =	BIT(9),
	FLD_IRQ_RSVD_10       =	BIT(10),
	FLD_IRQ_RSVD_11       =	BIT(11),
	FLD_IRQ_SOFT_IRQ_EN   =	BIT(12),
	FLD_IRQ_ZB_RT_EN      =	BIT(13),
	FLD_IRQ_SW_PWM_EN     =	BIT(14),
	FLD_IRQ_AN_EN         =	BIT(15),

	FLD_IRQ_RSVD_16       =	BIT(16),
	FLD_IRQ_RSVD_17       =	BIT(17),
	FLD_IRQ_GPIO_EN       =	BIT(18),
	FLD_IRQ_PM_EN         =	BIT(19),
	FLD_IRQ_RSVD_20       =	BIT(20),
	FLD_IRQ_GPIO_RISC0_EN =	BIT(21),
	FLD_IRQ_GPIO_RISC1_EN =	BIT(22),
	FLD_IRQ_GPIO_RISC2_EN = BIT(23),

//	FLD_IRQ_EN            =	BIT(24),

//	//Can not be used
	FLD_IRQ_USB_PWDN_EN   = BIT(31),
	FLD_IRQ_USB_RST_EN    = BIT(31),
};
#define reg_irq_pri				REG_ADDR32(0x644)
#define reg_irq_src				REG_ADDR32(0x648)
enum//Use for "reg_irq_src"
{
	FLD_IRQ_SRC_TMR0      =	BIT(0),
	FLD_IRQ_SRC_TMR1      =	BIT(1),
	FLD_IRQ_SRC_TMR2      =	BIT(2),
	FLD_IRQ_SRC_SYSTEM_TIMER=BIT(3),
	FLD_IRQ_SRC_DMA       =	BIT(4),
	FLD_IRQ_SRC_QDEC      =	BIT(5),
	FLD_IRQ_SRC_UART	  =	BIT(6),
	FLD_IRQ_SRC_HOST_CMD  =	BIT(7),

	FLD_IRQ_SRC_RSVD_8    =	BIT(8),
	FLD_IRQ_SRC_RSVD_9    =	BIT(9),
	FLD_IRQ_SRC_RSVD_10   =	BIT(10),
	FLD_IRQ_SRC_RSVD_11   =	BIT(11),
	FLD_IRQ_SRC_SOFT_IRQ  =	BIT(12),
	FLD_IRQ_SRC_ZB_RT     =	BIT(13),
	FLD_IRQ_SRC_PWM       =	BIT(14),
	FLD_IRQ_SRC_AN        =	BIT(15),

	FLD_IRQ_SRC_RSVD_16   =	BIT(16),
	FLD_IRQ_SRC_RSVD_17   =	BIT(17),
	FLD_IRQ_SRC_GPIO      =	BIT(18),
	FLD_IRQ_SRC_PM        =	BIT(19),
	FLD_IRQ_SRC_RSVD_20   =	BIT(20),
	FLD_IRQ_SRC_GPIO_RISC0=	BIT(21),
	FLD_IRQ_SRC_GPIO_RISC1=	BIT(22),
	FLD_IRQ_SRC_GPIO_RISC2= BIT(23),
};
#define reg_irq_src3			REG_ADDR8(0x64a)
#define reg_irq_en				REG_ADDR8(0x643)



/*----------------------------------------------------------------------------*/
/*-------   System timer register define                              --------*/
/*----------------------------------------------------------------------------*/
#define reg_system_tick			          REG_ADDR32(0x740)
#define reg_32k_timer_counter_latch       REG_ADDR32(0x744)//Read only
#define reg_32k_timer_calibration_value   REG_ADDR16(0x748)//Read only
#define reg_sys_timer_ctrl                REG_ADDR8(0x74a)
enum//Use for "reg_sys_timer_ctrl"
{
	FLD_SYS_TIMER_EN                 = BIT(7),
	FLD_SYS_TIMER_IRQ_EN             = BIT(6),
	FLD_SYS_TIMER_CAL_MODE           = BIT_RNG(4,5),
	FLD_SYS_TIMER_CAL_EN             = BIT(3),
	FLD_SYS_TIMER_AUTO_MODE          = BIT(2),
	FLD_RSVD                         = BIT(1),
	FLD_SYS_TIMER_WRITE_32K_TICK_EN  = BIT(0),
};

#define reg_sys_timer_cmd_state   REG_ADDR8(0x74b)
#define reg_32k_timer_tick        REG_ADDR32(0x74c)



//8267
#define reg_system_tick_irq		REG_ADDR32(0x744)
#define reg_system_wakeup_tick	REG_ADDR32(0x748)
#define reg_system_tick_mode	REG_ADDR8(0x74c)
#define reg_system_tick_ctrl	REG_ADDR8(0x74f)
enum {
	FLD_SYSTEM_TICK_START	= BIT(0),
	FLD_SYSTEM_TICK_STOP	= BIT(1),
	FLD_SYSTEM_TICK_RUNNING = BIT(1),

	FLD_SYSTEM_TICK_IRQ_EN  = BIT(1),
};


/*----------------------------------------------------------------------------*/
/*-------   PWM register define                                       --------*/
/*----------------------------------------------------------------------------*/
#define reg_pwm_enable			REG_ADDR8(0x780)
#define reg_pwm_clk				REG_ADDR8(0x781)
#define reg_pwm0_mode			REG_ADDR8(0x782)
enum //Use for "reg_pwm_mode"
{
	FLD_PWM0_MODE = BIT_RNG(0,3),
};

#define reg_pwm_invert			REG_ADDR8(0x783)
#define reg_pwm_n_invert		REG_ADDR8(0x784)
#define reg_pwm_pol				REG_ADDR8(0x785)//first polarity


#define reg_pwm_phase(i)		REG_ADDR16(0x789 + (i << 1))
#define reg_pwm_cycle(i)		REG_ADDR32(0x794 + (i << 2))
#define reg_pwm_cmp(i)			REG_ADDR16(0x794 + (i << 2))
#define reg_pwm_max(i)			REG_ADDR16(0x796 + (i << 2))
enum{
	FLD_PWM_CMP  = BIT_RNG(0,15),
	FLD_PWM_MAX  = BIT_RNG(16,31),
};

//PWM PNum
#define reg_pwm_pulse_num		 REG_ADDR16(0x7ac)

//PWM interrupt manager
#define reg_pwm_irq_mask		 REG_ADDR8(0x7b0)
#define reg_pwm_irq_sta			 REG_ADDR8(0x7b1)
enum{
	FLD_IRQ_PWM0_PNUM =					BIT(0),
	FLD_IRQ_PWM0_IR_DMA_FIFO_DONE  =	BIT(1),
	FLD_IRQ_PWM0_FRAME =				BIT(2),
	FLD_IRQ_PWM1_FRAME =				BIT(3),
	FLD_IRQ_PWM2_FRAME =				BIT(4),
};

#define reg_pwm0_fifo_mode_irq_mask		REG_ADDR8(0x7b2)
enum{
	FLD_IRQ_PWM0_IR_FIFO_EN  	 = BIT(0),
};

#define reg_pwm0_fifo_mode_irq_sta		REG_ADDR8(0x7b3)
enum{
	FLD_IRQ_PWM0_IR_FIFO_CNT 	 = BIT(0),
};


#define reg_pwm_tcmp0_shadow            REG_ADDR16(0x7c4)
#define reg_pwm_tmax0_shadow	        REG_ADDR16(0x7c6)

#define reg_pwm_ir_fifo_dat(i)			REG_ADDR16(0x7c8+i*2)
#define reg_pwm_ir_fifo_irq_trig_level	REG_ADDR8(0x7cc)

#define reg_pwm_ir_fifo_data_status     REG_ADDR8(0x7cd)
enum{
	FLD_PWM0_IR_FIFO_DATA_NUM 	=		BIT_RNG(0,3),
	FLD_PWM0_IR_FIFO_EMPTY 		=		BIT(4),
	FLD_PWM0_IR_FIFO_FULL 		=		BIT(5),
};

#define reg_pwm_ir_clr_fifo_data		REG_ADDR8(0x7ce)
enum{
	FLD_PWM0_IR_FIFO_CLR_DATA 	=		BIT(0),
};

/////////////////////////////////////////////////////////////////////////////

///////////////////// PM register /////////////////////////

//#define		rega_deepsleep_flag		0x3f		//0x34 - 0x39 (watch dog reset)
////#define		rega_deepsleep_flag		0x34		//0x3a - 0x3b (power-on reset)
//#define		flag_deepsleep_wakeup	(analog_read(0x3f) & 0x40)
//
//#define		rega_wakeup_en_val0		0x41
//#define		rega_wakeup_en_val1		0x42
//#define		rega_wakeup_en_val2		0x43
//#define		raga_gpio_wkup_pol		0x44
//
//#define		raga_pga_gain0		0x86
//#define		raga_pga_gain1		0x87


static inline void config_timer_interrupt (unsigned int tick) {
	reg_tmr1_tick = 0;
	reg_tmr1_capt = tick;
	reg_tmr_ctrl8 |= FLD_TMR1_EN;
	reg_irq_mask |= FLD_IRQ_TMR1_EN;
}

#define  reg_audio_rd_ptr   REG_ADDR16(0xb14)//DFIFO0 read pointer
#define  reg_audio_wr_ptr   REG_ADDR16(0xb16)//DFIFO0 write pointer


#if defined(__cplusplus)
}
#endif

#endif
