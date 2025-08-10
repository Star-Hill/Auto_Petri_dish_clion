//
// Created by DELL on 2025/8/7.
//

#ifndef AUTO_PETRI_DISH_CLION_LEFTRIGHT_H
#define AUTO_PETRI_DISH_CLION_LEFTRIGHT_H

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LeftRight_UART_NUM 1
#define LeftRight485_TX   41         // 4852_RX     对应      IO41    (原理图上是IO42)
#define LeftRight485_RX   42         // 4852_TX     对应      IO42    (原理图上是IO41)

#define LeftRight_RS485_BAUDRATE           57600       //波特率

// 从站地址（根据伺服电机的的设定修改，默认假设是 1）
#define LeftRight_SLAVE_ADDR  0x01

// 功能码
#define LeftRight_MODBUS_READ_SINGLE_REGISTER 0x03
#define LeftRight_MODBUS_WRITE_SINGLE_REGISTER 0x06

/* ------------------- 函数声明 ------------------- */

/**
 * @brief 初始化左右伺服电机的 RS485 UART 配置
 */
void LeftRight_init(void);

/**
 * @brief 读取伺服电机电压寄存器值
 *
 * 调用后会在日志中打印电压值（单位 V）。
 */
void LeftRight_read_Voltage(void);

/**
 * @brief 读取伺服电机电流寄存器值
 *
 * 调用后会在日志中打印电流值（单位 A）。
 */
void LeftRight_read_Electric_current(void);

/**
 * @brief 设置伺服电机转速（速度模式下）
 *
 * @param rpm 目标转速（单位 RPM）
 */
void LeftRight_set_speed(int16_t rpm);

/**
 * @brief 设置伺服电机为速度模式
 *
 * @param Mode 模式值（例如 0xC4）
 */
void LeftRight_set_speed_Mode(uint16_t Mode);

/**
 * @brief 设置伺服电机为位置模式
 *
 * @param Mode 模式值（例如 0xD0）
 */
void LeftRight_set_Location_Mode(uint16_t Mode);

/**
 * @brief 启动伺服电机
 */
void LeftRight_Start(void);

/**
 * @brief 停止伺服电机
 */
void LeftRight_Stop(void);

/**
 * @brief 清除伺服电机故障
 */
void LeftRight_Clear_the_fault(void);

/**
 * @brief 测试电机运行流程
 *
 * 包含：初始化 → 读取电压/电流 → 启动 → 设置速度模式 → 设置转速 → 停止 → 清除故障
 */
void LeftRight_Test(void);

#endif //AUTO_PETRI_DISH_CLION_LEFTRIGHT_H
