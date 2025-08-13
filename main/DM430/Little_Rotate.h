//
// Created by DELL on 2025/8/7.
//

#ifndef AUTO_PETRI_DISH_CLION_LITTLE_Rotate_H
#define AUTO_PETRI_DISH_CLION_LITTLE_Rotate_H

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sensor.h"
#include <math.h>


// ==================== 步进电机的GPIO引脚设置 ====================
#define Little_ROTATE_STEPPER_STEP_GPIO       13    // 电机 STEP 脉冲引脚
#define Little_ROTATE_STEPPER_DIR_GPIO        12    // 电机方向控制引脚
#define Little_ROTATE_STEPPER_EN_GPIO         14    // 电机使能控制引脚

// ==================== PWM设置 ====================
#define Little_ROTATE_STEPPER_PWM_CHANNEL     LEDC_CHANNEL_2      // PWM 通道号
#define Little_ROTATE_STEPPER_PWM_TIMER       LEDC_TIMER_2        // PWM 定时器号
#define Little_ROTATE_STEPPER_PWM_MODE        LEDC_LOW_SPEED_MODE // PWM 工作模式（低速）
#define Little_ROTATE_STEPPER_PWM_FREQ_HZ     2000                 // PWM 频率（Hz）
#define Little_ROTATE_STEPPER_PWM_RESOLUTION  LEDC_TIMER_10_BIT   // 占空比分辨率（10 位，最大值 1023）
#define Little_ROTATE_STEPPER_PWM_DUTY        512                  // PWM 占空比（≈50%，1023 最大值）

// ==================== 电机参数 ====================
#define Little_STEPS_PER_REV              1600                  // 电机转一圈需要的步数（含细分）

void Little_stepper_init(void);
void Little_stepper_rotate(float angle_deg, float rpm, int dir);


#endif //AUTO_PETRI_DISH_CLION_LITTLE_Rotate_H
