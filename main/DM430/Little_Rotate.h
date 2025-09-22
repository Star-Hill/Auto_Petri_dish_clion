//
// Created by DELL on 2025/8/7.
//

#ifndef AUTO_PETRI_DISH_CLION_LITTLE_ROTATE_H
#define AUTO_PETRI_DISH_CLION_LITTLE_ROTATE_H

#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

// ==================== 步进电机的GPIO引脚设置 ====================
#define Little_ROTATE_STEPPER_STEP_GPIO   13   // STEP 脉冲引脚
#define Little_ROTATE_STEPPER_DIR_GPIO    12   // 方向控制引脚
#define Little_ROTATE_STEPPER_EN_GPIO     14   // 使能控制引脚（低电平有效）

// ==================== 电机参数 ====================
#define Little_STEPS_PER_REV              800  // 电机转一圈需要的步数（含细分）

// ==================== RMT 配置 ====================
#define Little_ROTATE_STEPPER_RESOLUTION_HZ   1000000  // 1MHz 分辨率（1 tick = 1us）
#define Little_ROTATE_STEPPER_QUEUE_DEPTH     4        // 事务队列深度

// ==================== 函数接口 ====================
void Little_stepper_init(void);

/**
 * 控制电机旋转
 * @param angle_deg 旋转角度（度）
 * @param rpm       转速（转/分钟）
 * @param dir       方向（0=顺时针, 1=逆时针）
 */
void Little_stepper_rotate(float angle_deg, float rpm, int dir);

#endif //AUTO_PETRI_DISH_CLION_LITTLE_ROTATE_H
