/********************************************************************************************************
 * @file     putchar.c
 *
 * @brief    This is the source file for TLSR8233
 *
 * @author	 junwei.lu
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

#ifndef WIN32
#include "putchar.h"
#include "register.h"
#include "uart.h"
#include "irq.h"

#define USB_PRINT_TIMEOUT	 10		//  about 10us at 30MHz
#define USB_SWIRE_BUFF_SIZE  248	// 256 - 8

#define USB_EP_IN  		(USB_EDP_PRINTER_IN  & 0X07)	//  from the point of HOST 's view,  IN is the printf out
#define USB_EP_OUT  	(USB_EDP_PRINTER_OUT & 0X07)

int usb_putc(int c) {
  #if 0
	int i = 0;
	while(i ++ < USB_PRINT_TIMEOUT){
		if(!(reg_usb_ep8_fifo_mode & FLD_USB_ENP8_FULL_FLAG)){
			reg_usb_ep_dat(USB_EP_IN) = (unsigned char)c;
			return c;
		}
	}
  #endif
	return -1;
}

static inline void swire_set_clock(unsigned char div){
  #if 0
	reg_swire_clk_div = div;
  #endif
}

//static int swire_is_init = 0;
void swire_init()
{
#if(USB_SOMATIC_ENABLE)
    //  when USB_SOMATIC_ENABLE, USB_EDP_PRINTER_OUT disable
#else
//	r_usb.ep_adr[USB_EP_IN] = r_usb.ep_adr[USB_EP_OUT] = 0;
   #if 0
	reg_usb_ep_ptr(USB_EP_IN) = reg_usb_ep_ptr(USB_EP_OUT) = 0;
	reg_usb_ep8_send_max = 64;				// 32 * 8 == 256
   #endif

	//swire_set_clock(2);

#endif
}

int swire_putc(int c)
{
#if(USB_SOMATIC_ENABLE)
    //  when USB_SOMATIC_ENABLE, USB_EDP_PRINTER_OUT disable
#else
  #if 0
	if(!swire_is_init){
		swire_init();
		swire_is_init = 1;
	}
	int i = 0;
	while(i ++ < USB_PRINT_TIMEOUT){
		if(reg_usb_ep_ptr(USB_EP_IN) - reg_usb_ep_ptr(USB_EP_OUT) <= USB_SWIRE_BUFF_SIZE){	//  not full
			reg_usb_ep_dat(USB_EP_IN) = (unsigned char)c;
			return c;
		}
	}
  #endif
#endif
	return -1;
}

#if(SIMULATE_UART_EN)
#define	BIT_INTERVAL	  (16000000/DEBUG_BAUDRATE)
void uart_put_char(u8 byte)
{
	u8 j = 0;
	u32 t1 = 0,t2 = 0;

	REG_ADDR8(0x582+((DEBUG_TX_PIN>>8)<<3)) &= ~(DEBUG_TX_PIN & 0xff) ;//Enable output

	u32 pcTxReg = (0x583+((DEBUG_TX_PIN>>8)<<3));//register GPIO output level
	u8 tmp_bit0 = read_reg8(pcTxReg) & (~(DEBUG_TX_PIN & 0xff));
	u8 tmp_bit1 = read_reg8(pcTxReg) | (DEBUG_TX_PIN & 0xff);

	u8 bit[10] = {0};
	bit[0] = tmp_bit0;
	bit[1] = (byte & 0x01)? tmp_bit1 : tmp_bit0;
	bit[2] = ((byte>>1) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[3] = ((byte>>2) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[4] = ((byte>>3) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[5] = ((byte>>4) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[6] = ((byte>>5) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[7] = ((byte>>6) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[8] = ((byte>>7) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[9] = tmp_bit1;

	//注意：此处的关闭了中断，可能影响应用层。但是不关中断，一但产生了中断就会影响数据发送.需要权衡
	//u8 r = irq_disable();
	t1 = reg_system_tick;
	for(j = 0;j<10;j++)
	{
		t2 = t1;
		while(t1 - t2 < BIT_INTERVAL){
			t1  = reg_system_tick;
		}
		write_reg8(pcTxReg,bit[j]);        //send bit0
	}
	//irq_restore(r);
}
#endif

//the chip of Hawk serial Chip have no USB module.
int putchar(int c)
{
  #if(SIMULATE_UART_EN)
	uart_put_char(c);
  #else
	//uart_ndma_send_byte(c);
  #endif
	return c;
}

#endif

