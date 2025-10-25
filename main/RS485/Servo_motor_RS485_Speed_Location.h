//
// Created by DELL on 2025/9/10.
//

#ifndef AUTO_PETRI_DISH_CLION_SERVO_MOTOR_RS485_SPEED_LOCATION_H
#define AUTO_PETRI_DISH_CLION_SERVO_MOTOR_RS485_SPEED_LOCATION_H

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdint.h>

// UART 端口定义
#define SERVO_MOTOR_UART_NUM UART_NUM_1

// RS485 引脚定义
#define LeftRight485_TX    41   // IO41
#define LeftRight485_RX    42   // IO42

// 通讯波特率
#define SERVO_MOTOR_RS485_BAUDRATE   57600   // 波特率

// 从站地址（伺服电机设定）
#define SERVO_MOTOR_SLAVE_ADDR  0x01

// 功能码定义
#define SERVO_MOTOR_MODBUS_READ_SINGLE_REGISTER     0x03
#define SERVO_MOTOR_MODBUS_WRITE_SINGLE_REGISTER    0x06
#define SERVO_MOTOR_MODBUS_WRITE_MULTIPLE_REGISTER  0x10

// 是否启用调试日志（收发帧打印）
#define SERVO_MOTOR_DEBUG  // 注释掉则关闭日志
/* ------------------- 公共函数 ------------------- */

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 初始化伺服电机 UART 与 RS485 通讯
*/
void SERVO_MOTOR_init(void);

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 写单个寄存器
* @param reg_addr 寄存器地址
* @param value    写入的值
*/
void SERVO_MOTOR_modbus_write_single_register(uint16_t reg_addr, uint16_t value);

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 写多个寄存器（32位数据）
* @param reg_addr 起始寄存器地址
* @param value    写入的 32 位值
*/
void SERVO_MOTOR_modbus_write_multi_register(uint16_t reg_addr, uint32_t value);

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 读取单个寄存器
* @param reg_addr 寄存器地址
* @param value    读取到的数值指针
* @return true 成功，false 失败
*/
bool modbus_read_single_register(uint16_t reg_addr, uint16_t* value);

/* ------------------- 通用读写 ------------------- */
/**
* @CreateTime 2025/10/22
* @Author Star-Hill
* @brief 读取位置输出
*/
void SERVO_MOTOR_read_pos();

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 读取电压
*/
void SERVO_MOTOR_read_Voltage(void);

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 读取电流
*/
void SERVO_MOTOR_read_Electric(void);

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 清除伺服故障
*/
void SERVO_MOTOR_Clear_the_fault(void);

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 启动伺服电机
*/
void SERVO_MOTOR_Start(void);

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 停止伺服电机
*/
void SERVO_MOTOR_Stop(void);

/* ------------------- 速度模式 ------------------- */
/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 设置为速度模式
*/
void SERVO_MOTOR_Set_Speed_Mode(void);

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 设置电机速度
* @param speed_value 速度值
*/
void SERVO_MOTOR_Set_Speed(int32_t speed_value);

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 设置电机位置清零
*/
void SERVO_MOTOR_Clear_Position(void);

// 定义传感器函数指针类型（返回传感器状态）
typedef int (*SensorFunc)(void);

// /**
// * @CreateTime 2025/9/10
// * @Author Star-Hill
// * @brief 控制伺服电机移动到指定位置
// * @param sensor_get_state 传感器检测函数
// * @param speed 正数向右 || 负数向左
// * @param desc  日志输出说明
// */
// void SERVO_MOTOR_Move_To_Position(SensorFunc sensor_get_state, float speed, const char* desc);

/* ------------------- 位置模式 ------------------- */

/**
* @CreateTime 2025/9/10
* @Author Star-Hill
* @brief 设置为位置模式  并设置  0--绝对模式  1--相对模式
*/
void SERVO_MOTOR_set_Location_Mode(int relative);

/**
 * @brief 设置位置模式下最高速度
 * @param target_rpm 目标RPM，单位RPM
 */
void SERVO_MOTOR_Set_Position_Speed(uint16_t target_rpm);

/**
 * @brief 读取位置反馈
 */
int32_t SERVO_MOTOR_Read_Feedback_Position(void);

/**
 * @brief 读取位置给定
 */
int32_t SERVO_MOTOR_Read_Command_Position(void);
/**
 * @CreateTime 2025/9/10
 * @Author Star-Hill
 * @brief 伺服电机使用位置模式设置峰值速度和位置
 * @param speed_rpm 峰值速度，单位RPM
 * @param position 移动位置 , 原点为右 0 , 中间为 -297300 , 左边为 -561000
 * @param mode 0--绝对  1--相对
 * @param sensor_func 传感器函数调用
 */
void SERVO_MOTOR_Move_Position_Speed(int speed_rpm, int32_t position, int mode, SensorFunc sensor_func);
/**
 * @CreateTime 2025/10/23
 * @Author Star-Hill
 * @brief 伺服电机使用位置模式设置峰值速度和位置
 * @param speed_rpm 峰值速度，单位RPM
 * @param position 移动位置 , 原点为右 0 , 中间为 -300000 , 左边为 -555000
 * @param mode 0--绝对  1--相对
 * @param use_sensor ture--轮询右传感器  false--轮询寄存器
 */
void SERVO_MOTOR_POS_Reg(int speed_rpm, int32_t position, int mode, bool use_sensor);

#endif //AUTO_PETRI_DISH_CLION_SERVO_MOTOR_RS485_SPEED_LOCATION_H
