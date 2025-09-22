//
// Created by DELL on 2025/8/6.
//

#ifndef AUTO_PETRI_DISH_CLION_UPDOWN_H
#define AUTO_PETRI_DISH_CLION_UPDOWN_H

#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor.h"

// ==================== 步进电机的GPIO引脚设置 ====================
#define UpDown_STEPPER_STEP_GPIO       38    // STEP 脉冲引脚
#define UpDown_STEPPER_DIR_GPIO        39    // 方向控制引脚
#define UpDown_STEPPER_EN_GPIO         40    // 使能控制引脚（低电平有效）

// ==================== 电机参数 ====================
#define UpDown_STEPS_PER_REV           1600   // 电机转一圈步数（含细分）

// ==================== RMT 配置 ====================
#define UpDown_STEPPER_RESOLUTION_HZ   1000000  // 1 MHz 分辨率
#define UpDown_STEPPER_QUEUE_DEPTH     4        // 队列深度

void UpDown_stepper_init(void);

/**
 * 上下电机调用函数
 * @param angle_deg    旋转角度（度）
 * @param rpm          转速（转/分钟）
 * @param dir          方向 (1=上, 0=下)
 * @param check_sensor 0=不检测, 1=检测上限, 2=检测下限
 * @Comment 1850--上  1220--取   1200--吐液
 */
void UpDown_stepper_rotate(float angle_deg, float rpm, int dir, int check_sensor);

#endif //AUTO_PETRI_DISH_CLION_UPDOWN_H
