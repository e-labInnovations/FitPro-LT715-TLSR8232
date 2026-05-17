#include "uart_helper.h"

#include <stdint.h>
#include "common/string.h"
#include "drivers/5316/dma.h"
#include "drivers/5316/irq.h"
#include "drivers/5316/uart.h"

// Required by printf.c — not used here.
int putchar(int c) { return 0; }

void uart_helper_init(uint8_t *recv_buff, uint16_t recv_buff_len) {
    uart_set_recbuff((unsigned short *)&recv_buff, recv_buff_len);

    // TX = GPIO_PB4, RX = GPIO_PB5 (debug pads on FitPro LT715 FPC)
    uart_set_pin(GPIO_PB4, GPIO_PB5);

    uart_reset();

    // 16 MHz system clock: div=9, bwc=13 => 115200 baud
    uart_init_baudrate(9, 13, PARITY_NONE, STOP_BIT_ONE);

    // Enable DMA for TX & RX
    uart_dma_en(1, 1);
    irq_set_mask(FLD_IRQ_DMA_EN);
    dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);

    // Using DMA — disable UART pin interrupts
    uart_irq_en(0, 0);
}

void uart_send(const uart_data_t *data) {
    uart_dma_send((uint16_t *)data);
}
