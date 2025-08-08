//
// Created by DELL on 2025/8/6.
//

#ifndef AUTO_PETRI_DISH_CLION_ROTATE_H
#define AUTO_PETRI_DISH_CLION_ROTATE_H

#include "driver/ledc.h"

#define TAG1 "Rotate_MOTOR"

// 电机控制引脚
#define Rotate_STEPPER_PUL_GPIO    40
#define Rotate_STEPPER_DIR_GPIO    39
#define Rotate_STEPPER_EN_GPIO     38

// PWM 设置
#define Rotate_STEPPER_PWM_CHANNEL     LEDC_CHANNEL_0
#define Rotate_STEPPER_PWM_TIMER       LEDC_TIMER_0
#define Rotate_STEPPER_PWM_MODE        LEDC_LOW_SPEED_MODE
#define Rotate_STEPPER_PWM_FREQ_HZ     50000
#define Rotate_STEPPER_PWM_RESOLUTION  LEDC_TIMER_10_BIT
#define Rotate_STEPPER_PWM_DUTY        512         // 对于10位分辨率的最大值为1023

// 初始化电机相关GPIO和PWM
void Rotate_motor_driver_init(void);

// 控制方向：0 = 正转，1 = 反转
void Rotate_motor_set_direction(int dir);

// 控制使能：0 = 禁用，1 = 使能
void Rotate_motor_enable(int enable);

// 设置PWM脉冲的占空比（等于步进速度），0~1023
void Rotate_motor_set_speed(uint32_t duty);

// 启动脉冲输出（常用于持续转动）
void Rotate_motor_start(void);

// 停止脉冲输出
void Rotate_motor_stop(void);

//电机测试程序
void Rotate_motor_test(void);

#endif //AUTO_PETRI_DISH_CLION_ROTATE_H
