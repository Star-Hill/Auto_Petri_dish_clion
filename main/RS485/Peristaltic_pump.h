//
// Created by DELL on 2025/8/8.
//

#ifndef AUTO_PETRI_DISH_CLION_PERISTALTIC_PUMP_H
#define AUTO_PETRI_DISH_CLION_PERISTALTIC_PUMP_H

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

/**
 * @brief UART 端口定义
 */
#define Peristaltic_pump_UART_NUM UART_NUM_1

/**
 * @brief RS485 引脚定义
 */
#define Peristaltic_pump_TX   2     // IO2
#define Peristaltic_pump_RX   1     // IO1

/**
 * @brief 蠕动泵从站 波特率
 */
#define Peristaltic_pump_BAUDRATE   9600   // 波特率

/**
 * @brief 蠕动泵从站 Modbus 地址
 */
#define Peristaltic_pump_SLAVE_ADDR  0x02

// 功能码
#define Peristaltic_pump_MODBUS_READ_SINGLE_REGISTER  0x03
#define Peristaltic_pump_MODBUS_WRITE_SINGLE_REGISTER 0x06

//通讯连接定义
#define MODBUS_MAX_RETRY 10     // 最大重试次数
#define MODBUS_DELAY_MS_P  100   // 每次通信等待延迟
#define Peristaltic_pump_TAG "Peristaltic_pump_RS485"

#define Peristaltic_pump_DEBUG  // 注释掉则关闭日志

/* ------------------- 公共函数 ------------------- */
/* ===================== 安装蠕动泵 UART ===================== */
void Peristaltic_Pump_init(void);
/* ===================== 卸载蠕动泵 UART ===================== */
void Peristaltic_Pump_deinit(void);
/* ===================== API 函数声明 ===================== */

/**
 * @brief 设置蠕动泵转速
 * @param rpm 转速 (单位：转/分钟)，内部会乘以 10 发送给设备
 */
void Peristaltic_pump_set_speed(int32_t rpm);

/**
 * @brief 设置蠕动泵方向模式
 * @param Mode 方向：1=逆时针，0=顺时针
 * @note 对应寄存器地址为十进制 3101
 */
void Peristaltic_pump_set_Reverse_Mode(uint16_t Mode);

/**
 * @brief 启动/停止蠕动泵
 * @note 对应寄存器地址为十进制 3102，写入 0x01 启动，写入 0x00 停止
 */
void Peristaltic_pump_Control(bool enable);

/**
 * @brief 是否全速清洗蠕动泵
 * @note 对应寄存器地址为十进制 3103，写入 0x01 全速，写入 0x00 正常速度
 */
void Peristaltic_pump_Cleaning(bool enable);

// 运行蠕动泵一段时间
void Peristaltic_pump_Run(int32_t speed, float run_time_s);

#endif //AUTO_PETRI_DISH_CLION_PERISTALTIC_PUMP_H
