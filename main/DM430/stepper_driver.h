#ifndef STEPPER_DRIVER_H
#define STEPPER_DRIVER_H

#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "stepper_motor_encoder.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================
//  用户配置区 —— 移植到新工程时只需修改此处
// ============================================================

// ---------- GPIO 引脚 ----------
#define STEPPER_STEP_GPIO           48
#define STEPPER_DIR_GPIO            47
#define STEPPER_EN_GPIO             21

// ---------- 使能电平 ----------
// DM 系列驱动器：ENA- 接 GND 时低电平使能，填 0
// 若硬件加了反相电路或驱动器高电平使能，填 1
#define STEPPER_EN_ACTIVE_LEVEL     1

// ---------- 方向反转 ----------
// 若实际转向与期望相反，将此宏改为 1 即可翻转所有调用的方向
#define STEPPER_DIR_INVERT          0

// ---------- 电机参数 ----------
#define STEPPER_STEPS_PER_REV       1600    // 每圈脉冲数（驱动器细分 × 200）
#define STEPPER_GEAR_RATIO          1       // 机械传动比（输出轴转速 = 电机转速 / 此值）

// ---------- 速度上限 ----------
#define STEPPER_RPM_MAX             600.0f  // 超过此值时自动限幅

// ---------- RMT 配置 ----------
#define STEPPER_RMT_RESOLUTION_HZ   1000000U  // 1 MHz，1 tick = 1 µs
#define STEPPER_RMT_MEM_SYMBOLS     64
#define STEPPER_RMT_QUEUE_DEPTH     10

// ---------- S 曲线 / 梯形曲线 公共参数 ----------
#define STEPPER_ADS_FREQ_START      300U    // 起步 / 收尾频率（Hz）
#define STEPPER_ADS_FREQ_MIN        300U    // freq_max 的保护下限（Hz），防止低速传参导致异常
#define STEPPER_ADS_ACCEL_DIV       7       // 加减速段占总步数比例的分母（steps_total / N）

// ---------- S 曲线专用参数 ----------
#define STEPPER_ADS_SAMPLE_CAP      500U    // sample_points 上限（须满足 ≤ |freq_max - freq_start|）

// ---------- 梯形曲线专用参数 ----------
// 加减速各自被均匀切成 N 段，每段以线性插值的固定频率匀速运行
// 值越大曲线越平滑，但 RMT 传输次数也越多；推荐 8~32
#define STEPPER_TRAP_RAMP_SEGS      16

// ---------- 日志标签 ----------
#define STEPPER_LOG_TAG             "STEPPER"

// ============================================================
//  类型定义
// ============================================================

// 传感器回调函数指针
// 返回 0：传感器触发（到位 / 遮挡）
// 返回 1：传感器未触发
// 传入 NULL：不使用传感器，纯步数控制
typedef int (*stepper_sensor_fn_t)(void);

// ============================================================
//  API
// ============================================================

/**
 * @brief 初始化步进电机 GPIO（DIR + EN），默认失能
 */
void stepper_init(void);

/**
 * @brief 匀速旋转
 *
 * @param angle_deg  旋转角度（度）
 * @param rpm        转速（rpm），超过 STEPPER_RPM_MAX 时自动限幅
 * @param dir        旋转方向：1 = 正转，0 = 反转
 * @param sensor_fn  到位检测回调，NULL = 纯步数控制
 *                   传感器触发（返回0）后立即停止
 */
void stepper_rotate_US(float angle_deg, float rpm, int dir, stepper_sensor_fn_t sensor_fn);

/**
 * @brief S 曲线加减速旋转
 *
 * 加减速段使用三阶 smoothstep 插值（curve_encoder），起步和停止时加速度变化平滑。
 * 速度曲线形状：freq_start →(平滑加速)→ freq_max →(匀速)→ freq_max →(平滑减速)→ freq_start
 * 三段步数之和严格等于 steps_total。
 *
 * @param angle_deg  旋转角度（度）
 * @param rpm        峰值转速（rpm），超过 STEPPER_RPM_MAX 时自动限幅
 * @param dir        旋转方向：1 = 正转，0 = 反转
 * @param sensor_fn  到位检测回调，NULL = 纯步数控制；触发后跳过剩余段立即停止
 */
void stepper_rotate_ADS(float angle_deg, float rpm, int dir, stepper_sensor_fn_t sensor_fn);

/**
 * @brief 梯形加减速旋转
 *
 * 加减速段为线性升/降频（等间距分 STEPPER_TRAP_RAMP_SEGS 段，每段 uniform_encoder 匀速）。
 * 速度曲线形状（速度-时间图）：
 *
 *          freq_max ________
 *                  /        \
 *       freq_start            freq_start
 *          |加速  |  匀速  |减速|
 *
 * 与 S 曲线对比：梯形的加速度恒定（线性斜坡），冲击略大但过渡更可预测。
 * 三段步数之和严格等于 steps_total。
 *
 * @param angle_deg  旋转角度（度）
 * @param rpm        峰值转速（rpm），超过 STEPPER_RPM_MAX 时自动限幅
 * @param dir        旋转方向：1 = 正转，0 = 反转
 * @param sensor_fn  到位检测回调，NULL = 纯步数控制；触发后立即停止
 */
void stepper_rotate_TRAP(float angle_deg, float rpm, int dir, stepper_sensor_fn_t sensor_fn);

#endif // STEPPER_DRIVER_H
