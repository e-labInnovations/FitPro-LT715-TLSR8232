#ifndef UART_HELPER_H
#define UART_HELPER_H

#include <stdarg.h>
#include <stdint.h>

#include "drivers/5316/gpio.h"
#include "drivers/5316/uart.h"

#define UART_TX_LEN (128 - 4)

// Macro to format and send a string over UART (like printf).
#define uart_printf(send_buff, format, ...)                \
  sprintf((char *)send_buff.data, format, ##__VA_ARGS__);  \
  send_buff.len = strlen((char *)send_buff.data);          \
  uart_send(&send_buff);

typedef struct {
    unsigned int len;
    uint8_t data[UART_TX_LEN];
} uart_data_t;

// Initializes DMA UART at 115200 baud.
// TX => GPIO_PB4  (FPC debug pad)
// RX => GPIO_PB5  (FPC debug pad)
void uart_helper_init(uint8_t *recv_buff, uint16_t recv_buff_len);

void uart_send(const uart_data_t *data);

#endif // UART_HELPER_H
