//
// Created by DELL on 2025/9/15.
//
#ifndef AUTO_PETRI_DISH_CLION_BIG_ROTATE_H
#define AUTO_PETRI_DISH_CLION_BIG_ROTATE_H

#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor.h"
#include <math.h>

// ==================== 步进电机的GPIO引脚设置 ====================
#define Big_ROTATE_STEPPER_STEP_GPIO    40          // STEP 脉冲引脚              21
#define Big_ROTATE_STEPPER_DIR_GPIO     39          // 方向控制引脚                47
#define Big_ROTATE_STEPPER_EN_GPIO      38          // 使能控制引脚（低电平有效）    48

// ==================== 电机参数 ====================
#define Big_ROTATE_STEPS_PER_REV       3200         // 电机转一圈需要的步数（含细分）1600
#define BIG_ROTATE_GEAR_RATIO          50           // 传动比

// ==================== RMT 配置 ====================
#define Big_ROTATE_STEPPER_RESOLUTION_HZ   1000000  // 1 MHz 分辨率（1 tick = 1us）
#define Big_ROTATE_STEPPER_QUEUE_DEPTH     10        // 事务队列深度

// ==================== 函数接口 ====================

/**
 * 初始化大电机 RMT 通道与引脚
 */
void Big_ROTATE_stepper_init(void);

/**
 * 控制大电机旋转（S 型曲线加减速）
 * @param angle_deg     旋转角度（度）
 * @param rpm           转速（转/分钟）
 * @param dir           方向（0=顺时针, 1=逆时针）
 * @param check_sensor  是否检测零点传感器（1=检测, 0=不检测）
 * @param sample_points 曲线采样点（越大越平滑，越小越陡）
 */
void Big_ROTATE_stepper_rotate(float angle_deg, float rpm, int dir, bool check_sensor, uint32_t sample_points);

#endif //AUTO_PETRI_DISH_CLION_BIG_ROTATE_H
