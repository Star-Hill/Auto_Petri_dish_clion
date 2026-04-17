//
// Created by DELL on 2025/8/6.
//

#ifndef AUTO_PETRI_DISH_CLION_UPDOWN_H
#define AUTO_PETRI_DISH_CLION_UPDOWN_H

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "sensor.h"
#include "stepper_motor_encoder.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_types.h"

// ==================== 步进电机的GPIO引脚设置 ====================
#define UpDown_STEPPER_STEP_GPIO   48        // STEP 脉冲引脚
#define UpDown_STEPPER_DIR_GPIO    47        // 方向控制引脚
#define UpDown_STEPPER_EN_GPIO     21        // 使能控制引脚

// ==================== 电机参数 ====================
#define UpDown_STEPS_PER_REV           1600   // 电机转一圈步数（含细分）1600
#define UpDown_ROTATE_GEAR_RATIO       1      // 传动比

// ==================== RMT 配置 ====================
#define UpDown_STEPPER_RESOLUTION_HZ   1000000  // 1 MHz 分辨率
#define UpDown_STEPPER_QUEUE_DEPTH     10        // 队列深度

void UpDown_stepper_init(void);

/**
 * 上下电机匀速运动
 * @param angle_deg    旋转角度（度）
 * @param rpm          转速（转/分钟）200基值可行  300基值测试
 * @param dir          方向 (1=上, 0=下)
 * @param check_sensor 0=不检测, 2=检测下限传感器
 * @Comment 1850--上  1220--取   1200--吐液
 */
void UpDown_stepper_rotate(float angle_deg, float rpm, int dir, int check_sensor);

/**
 * 上下电机 S 曲线加减速运动（起步/停止平滑，减少机械冲击）
 * @param angle_deg    旋转角度（度）
 * @param rpm          峰值转速（转/分钟），内部限幅 600
 * @param dir          方向 (1=上, 0=下)
 * @param check_sensor 0=不检测, 2=检测下限传感器（触发后立即停止）
 */
void UpDown_stepper_rotate_ADS(float angle_deg, float rpm, int dir, int check_sensor);

#endif //AUTO_PETRI_DISH_CLION_UPDOWN_H
