//
// Created by DELL on 2025/8/10.
//

#ifndef AUTO_PETRI_DISH_CLION_SOFTSERIAL_H
#define AUTO_PETRI_DISH_CLION_SOFTSERIAL_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

typedef struct
{
    uint8_t *buffer;//缓存区指针
    uint16_t head;//头
    uint16_t tail;//尾
    uint16_t rx_buff_size;//大小
} UART_Buffer;

typedef struct
{
    uint8_t tx_pin;//发送引脚
    uint8_t rx_pin;//接收引脚
    uint16_t rx_buff_size;//接收缓存区大小
    uint32_t baud_rate;//波特率-默认正常逻辑电平-八数据位
    uint64_t bit_cycles;// 每位时间（CPU周期）
    UART_Buffer rx_buffer;//不可改变
    portMUX_TYPE lock;//不可改变
} SoftwareUART;

void software_uart_init(SoftwareUART *uart);
void software_uart_tx_str(SoftwareUART *uart,const char* str);
int software_uart_rx_read(SoftwareUART *uart, uint8_t* data, size_t size);

void software_uart_test(void);               //干净的串口通讯
#endif //AUTO_PETRI_DISH_CLION_SOFTSERIAL_H
