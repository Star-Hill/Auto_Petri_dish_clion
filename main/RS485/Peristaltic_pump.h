//
// Created by DELL on 2025/8/8.
//

#ifndef AUTO_PETRI_DISH_CLION_PERISTALTIC_PUMP_H
#define AUTO_PETRI_DISH_CLION_PERISTALTIC_PUMP_H

#include "driver/gpio.h"
#include "esp_log.h"

#define Peristaltic_pump_UART_NUM 1
#define Peristaltic_pump_485_TX   41         // 4852_RX     对应      IO41    (原理图上是IO42)
#define Peristaltic_pump_485_RX   42         // 4852_TX     对应      IO42    (原理图上是IO41)

#define Peristaltic_pump_RS485_BAUDRATE           9600       //波特率

// 从站地址（根据伺服电机的的设定修改，默认假设是 1）
#define Peristaltic_pump_SLAVE_ADDR  0x01

// 功能码
#define Peristaltic_pump_MODBUS_READ_SINGLE_REGISTER 0x03
#define Peristaltic_pump_MODBUS_WRITE_SINGLE_REGISTER 0x06

#endif //AUTO_PETRI_DISH_CLION_PERISTALTIC_PUMP_H
