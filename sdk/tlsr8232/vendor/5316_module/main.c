/********************************************************************************************************
 * @file     main.c
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
 * @date     May. 12, 2018
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *           
 *			 The information contained herein is confidential and proprietary property of Telink 
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms 
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai) 
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in. 
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this 
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided. 
 *           
 *******************************************************************************************************/
#include "app.h"
#include <tl_common.h>
#include "drivers.h"
#include "stack/ble/ble.h"
#include "../common/user_config.h"


extern my_fifo_t spp_rx_fifo;

_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler();

#if (HCI_ACCESS==HCI_USE_UART)
	unsigned char irqS = dma_chn_irq_status_get();
    if(irqS & FLD_DMA_CHN_UART_RX)	//rx
    {
    	dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX);

    	/////
    	u8* w = spp_rx_fifo.p + (spp_rx_fifo.wptr & (spp_rx_fifo.num-1)) * spp_rx_fifo.size;
    	if(w[0]!=0)
    	{
    		my_fifo_next(&spp_rx_fifo);
    		u8* p = spp_rx_fifo.p + (spp_rx_fifo.wptr & (spp_rx_fifo.num-1)) * spp_rx_fifo.size;
    		reg_dma_uart_rx_addr = (u16)((u32)p);  //switch uart RX dma address
    	}
    }

    if(irqS & FLD_DMA_CHN_UART_TX)	//tx
    {
    	dma_chn_irq_status_clr(FLD_DMA_CHN_UART_TX);
    }
#endif
}

int main(void){

	blc_pm_select_internal_32k_crystal();
	//blc_pm_select_external_32k_crystal();

	#if(FLASH_SIZE_OPTION == FLASH_SIZE_OPTION_128K) ///FLASH_SIZE_OPTION_128K
		bls_ota_setFirmwareSizeAndOffset(48, 0x10000);///default : ota_firmware_size_k=128;ota_program_bootAddr=0x20000; it is for hawk 128K flash
		bls_smp_configParingSecurityInfoStorageAddr(0x1C000);
	#endif

	cpu_wakeup_init();

	#if (CLOCK_SYS_CLOCK_HZ == 16000000)
		clock_init(SYS_CLK_16M_Crystal);
	#elif (CLOCK_SYS_CLOCK_HZ == 32000000)
		clock_init(SYS_CLK_32M_Crystal);
	#elif (CLOCK_SYS_CLOCK_HZ == 48000000)
		clock_init(SYS_CLK_48M_Crystal);
	#endif

	gpio_init();

	/* load customized freq_offset CAP value and TP value.*/
	blc_app_loadCustomizedParameters();

	rf_drv_init(RF_MODE_BLE_1M);

	user_init();

    irq_enable();

	while (1) {
	#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
	#endif
		main_loop ();
	}
}

