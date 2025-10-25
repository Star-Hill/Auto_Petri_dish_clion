//
// Created by DELL on 2025/10/24
//
#ifndef AUTO_PETRI_DISH_CLION_LITTLE_ROTATE_H
#define AUTO_PETRI_DISH_CLION_LITTLE_ROTATE_H

#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdint.h>
#include "stepper_motor_encoder.h"

// ==================== 步进电机GPIO引脚 ====================
#define Little_ROTATE_STEPPER_STEP_GPIO   13
#define Little_ROTATE_STEPPER_DIR_GPIO    12
#define Little_ROTATE_STEPPER_EN_GPIO     14

// ==================== 电机参数 ====================
#define Little_STEPS_PER_REV              800
#define LITTLE_ROTATE_GEAR_RATIO          1

// ==================== RMT 配置 ====================
#define Little_ROTATE_STEPPER_RESOLUTION_HZ 1000000
#define Little_ROTATE_STEPPER_QUEUE_DEPTH    4

// ==================== 函数接口 ====================
void Little_stepper_init(void);

/**
 * 匀速旋转
 */
void Little_stepper_rotate_US(float angle_deg, float rpm, int dir);

#endif // AUTO_PETRI_DISH_CLION_LITTLE_ROTATE_H
