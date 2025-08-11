//
// Created by DELL on 2025/8/11.
//
#include "Sensor/sensor.h"
#include "Rotate.h"
#include "esp_log.h"
#include <math.h>
#include "freertos/FreeRTOS.h"

void Rotate_motor_CALIBRATION(float angle_deg, float rpm, bool check_sensor) {
    #define TAG "Rotate_MOTOR"
    ESP_LOGI(TAG, "开始初始化柱体旋转电机驱动");
    Rotate_motor_driver_init();

    const uint32_t motor_full_steps = 200; // 电机整步数
    const uint32_t microsteps = 1;         // 驱动器细分，根据SW5-8设置
    const uint32_t steps_per_rev = motor_full_steps * microsteps;

    // 计算总步数
    uint32_t total_steps = (uint32_t)((fabs(angle_deg) / 360.0f) * steps_per_rev + 0.5f);
    if (total_steps == 0) return; // 没有步数就直接返回

    // 设置方向
    int dir = (angle_deg >= 0) ? 1 : 0;
    Rotate_motor_set_direction(dir);
    Rotate_motor_enable(1);

    // 计算PWM频率
    float steps_per_sec = (rpm * steps_per_rev) / 60.0f;
    uint32_t pwm_freq = (uint32_t)(steps_per_sec + 0.5f);

    // 更新LEDC频率
    ledc_set_freq(Rotate_STEPPER_PWM_MODE, Rotate_STEPPER_PWM_TIMER, pwm_freq);

    // 启动PWM
    Rotate_motor_set_speed(Rotate_STEPPER_PWM_DUTY);

    // 计算需要的时间并延时
    float time_sec = (fabs(angle_deg) / 360.0f) * (60.0f / rpm);

    if (check_sensor) {
        // 循环检测传感器状态，期间延时小段时间，模拟非阻塞等待
        const TickType_t delay_ticks = pdMS_TO_TICKS(10); // 10ms检测一次
        TickType_t elapsed_ticks = 0;
        TickType_t total_ticks = pdMS_TO_TICKS((uint32_t) (time_sec * 1000));

        while (elapsed_ticks < total_ticks) {
            if (sensor_Calibration_get_state() == 0) {
                ESP_LOGI(TAG, "传感器检测到状态为0，停止电机");
                break;
            }
            vTaskDelay(delay_ticks);
            elapsed_ticks += delay_ticks;
        }
    }
    ESP_LOGI(TAG, "到达校准位，停止电机");
    // 停止PWM
    Rotate_motor_stop();
    Rotate_motor_enable(0);
}




