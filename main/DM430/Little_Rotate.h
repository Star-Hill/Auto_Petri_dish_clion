//
// Created by DELL on 2025/8/7.
//

#ifndef AUTO_PETRI_DISH_CLION_LITTLE_Rotate_H
#define AUTO_PETRI_DISH_CLION_LITTLE_Rotate_H

#include "driver/ledc.h"

// 电机控制引脚
#define Little_Rotate_STEPPER_Step_GPIO    13
#define Little_Rotate_STEPPER_DIR_GPIO    12
#define Little_Rotate_STEPPER_EN_GPIO     14

// PWM 设置
#define Little_Rotate_STEPPER_PWM_CHANNEL     LEDC_CHANNEL_2
#define Little_Rotate_STEPPER_PWM_TIMER       LEDC_TIMER_2
#define Little_Rotate_STEPPER_PWM_MODE        LEDC_LOW_SPEED_MODE
#define Little_Rotate_STEPPER_PWM_FREQ_HZ     5000
#define Little_Rotate_STEPPER_PWM_RESOLUTION  LEDC_TIMER_10_BIT
#define Little_Rotate_STEPPER_PWM_DUTY        512         // 对于10位分辨率的最大值为1023

// 初始化电机相关GPIO和PWM
void Little_Rotate_motor_driver_init(void);

// 控制方向：0 = 正转，1 = 反转
void Little_Rotate_motor_set_direction(int dir);

// 控制使能：0 = 禁用，1 = 使能
void Little_Rotate_motor_enable(int enable);

// 设置PWM脉冲的占空比（等于步进速度），0~1023
void Little_Rotate_motor_set_speed(uint32_t duty);

// 启动脉冲输出（常用于持续转动）
void Little_Rotate_motor_start(void);

// 停止脉冲输出
void Little_Rotate_motor_stop(void);

//电机测试程序
void Little_Rotate_motor_test(void);

#endif //AUTO_PETRI_DISH_CLION_LITTLE_Rotate_H
