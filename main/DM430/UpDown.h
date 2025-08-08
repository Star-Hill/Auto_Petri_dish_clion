//
// Created by DELL on 2025/8/6.
//

#ifndef AUTO_PETRI_DISH_CLION_UpDown_H
#define AUTO_PETRI_DISH_CLION_UpDown_H

#include "driver/ledc.h"

// 电机控制引脚
#define UpDown_STEPPER_PUL_GPIO    48
#define UpDown_STEPPER_DIR_GPIO    47
#define UpDown_STEPPER_EN_GPIO     21

// PWM 设置
#define UpDown_STEPPER_PWM_CHANNEL     LEDC_CHANNEL_1
#define UpDown_STEPPER_PWM_TIMER       LEDC_TIMER_1
#define UpDown_STEPPER_PWM_MODE        LEDC_LOW_SPEED_MODE
#define UpDown_STEPPER_PWM_FREQ_HZ     10000
#define UpDown_STEPPER_PWM_RESOLUTION  LEDC_TIMER_10_BIT
#define UpDown_STEPPER_PWM_DUTY        512         // 对于10位分辨率的最大值为1023

// 初始化电机相关GPIO和PWM
void UpDown_motor_driver_init(void);

// 控制方向：0 = 正转，1 = 反转
void UpDown_motor_set_direction(int dir);

// 控制使能：0 = 禁用，1 = 使能
void UpDown_motor_enable(int enable);

// 设置PWM脉冲的占空比（等于步进速度），0~1023
void UpDown_motor_set_speed(uint32_t duty);

// 启动脉冲输出（常用于持续转动）
void UpDown_motor_start(void);

// 停止脉冲输出
void UpDown_motor_stop(void);

//电机测试程序
void UpDown_motor_test(void);

#endif //AUTO_PETRI_DISH_CLION_UpDown_H
