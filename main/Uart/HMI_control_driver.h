//
// Created by DELL on 2025/8/7.
//

#ifndef AUTO_PETRI_DISH_CLION_HMI_CONTROL_DRIVER_H
#define AUTO_PETRI_DISH_CLION_HMI_CONTROL_DRIVER_H

#include "driver/uart.h"
#include "esp_log.h"


#define UART_HMI UART_NUM_2
#define UART_TX_PIN 5       // ESP32 --> 串口屏 RX
#define UART_RX_PIN 6       // 串口屏 TX --> ESP32

#define UART_BUF_SIZE 1024

void uart_hmi_init(void);
void uart_hmi_send(const char *data);
int uart_hmi_read(uint8_t *data, size_t length);

//串口屏测试函数
void HMI_control_driver_test(void);

#endif //AUTO_PETRI_DISH_CLION_HMI_CONTROL_DRIVER_H
