//
// Created by DELL on 2025/8/8.
//

#ifndef AUTO_PETRI_DISH_CLION_PERISTALTIC_PUMP_H
#define AUTO_PETRI_DISH_CLION_PERISTALTIC_PUMP_H

#include "driver/gpio.h"
#include "esp_log.h"

/**
 * @brief RS485 引脚定义    使用UART端口号
 */
#define Peristaltic_pump_UART_NUM 2
#define Peristaltic_pump_485_TX   2
#define Peristaltic_pump_485_RX   1

/**
 * @brief 蠕动泵 RS485 波特率
 */
#define Peristaltic_pump_RS485_BAUDRATE           9600       //波特率

/**
 * @brief 蠕动泵从站 Modbus 地址
 */
#define Peristaltic_pump_SLAVE_ADDR  0x01

// 功能码
#define Peristaltic_pump_MODBUS_READ_SINGLE_REGISTER 0x03           //功能码：读寄存器
#define Peristaltic_pump_MODBUS_WRITE_SINGLE_REGISTER 0x06          //功能码：写单寄存器

/* ===================== API 函数声明 ===================== */

/**
 * @brief 初始化蠕动泵 RS485 通信接口
 * @note 必须在调用其他函数之前调用
 */
void Peristaltic_pump_init(void);

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
 * @brief 启动停止蠕动泵
 * @note 对应寄存器地址为十进制 3102，写入 0x01 使能    写入 0x00 使能
 */
void Peristaltic_pump_Control(bool enable);

/**
 * @brief 是否全速清洗蠕动泵
 * @note 对应寄存器地址为十进制 3103，写入 0x01 全速    写入 0x00 正常速度
 */
void Peristaltic_pump_Cleaning(bool enable);

/**
 * @brief 测试蠕动泵循环启停
 * @note 启动、运行 2 秒、停止，循环执行
 */
void Peristaltic_pump_Test(void);


#endif //AUTO_PETRI_DISH_CLION_PERISTALTIC_PUMP_H
