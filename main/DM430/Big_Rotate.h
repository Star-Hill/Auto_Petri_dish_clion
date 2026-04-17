//
// Created by DELL on 2025/9/15
//

#ifndef AUTO_PETRI_DISH_CLION_BIG_ROTATE_H
#define AUTO_PETRI_DISH_CLION_BIG_ROTATE_H

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "sensor.h"
#include "stepper_motor_encoder.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_types.h"

// ==================== 步进电机的GPIO引脚设置 ====================
#define Big_ROTATE_STEPPER_EN_GPIO    38     // 使能控制引脚(低电平有效)
#define Big_ROTATE_STEPPER_DIR_GPIO   39     // 方向控制引脚
#define Big_ROTATE_STEPPER_STEP_GPIO  40     // STEP 脉冲引脚

// ==================== 电机参数 ====================
#define Big_ROTATE_STEPS_PER_REV       1600    // 电机转一圈需要的步数（含细分）
#define BIG_ROTATE_GEAR_RATIO          50      // 传动比

// ==================== RMT 配置 ====================
#define Big_ROTATE_STEPPER_RESOLUTION_HZ   1000000  // 1 MHz 分辨率（1 tick = 1us）
#define Big_ROTATE_STEPPER_QUEUE_DEPTH     10        // 事务队列深度

// ==================== 函数接口 ====================

/**
 * 初始化大电机 GPIO 引脚
 */
void Big_ROTATE_stepper_init(void);

/**
 * 控制大电机旋转（匀速）
 * @param angle_deg     旋转角度（度）
 * @param rpm           转速（转/分钟）
 * @param dir           方向（0=顺时针, 1=逆时针）
 */
void Big_ROTATE_stepper_rotate_US(float angle_deg, float rpm, int dir, bool check_sensor);

/**
 * 控制大电机旋转（加减速）
 * @param angle_deg     旋转角度（度）
 * @param rpm           转速（转/分钟）
 * @param dir           方向（0=顺时针, 1=逆时针）
 */
void Big_ROTATE_stepper_rotate_ADS(float angle_deg, float rpm, int dir);

#endif // AUTO_PETRI_DISH_CLION_BIG_ROTATE_H
