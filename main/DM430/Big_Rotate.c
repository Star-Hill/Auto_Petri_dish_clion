//
// Created by DELL on 2025/8/6.
//

#include "Big_Rotate.h"

static const char *TAG = "Big_ROTATE_STEPPER_CTRL";

// 步进电机初始化
void Big_ROTATE_stepper_init(void) {
    // STEP 引脚配置为 PWM
    ledc_timer_config_t timer_conf = {
            .speed_mode       = Big_ROTATE_STEPPER_PWM_MODE,
            .timer_num        = Big_ROTATE_STEPPER_PWM_TIMER,
            .duty_resolution  = Big_ROTATE_STEPPER_PWM_RESOLUTION,
            .freq_hz          = Big_ROTATE_STEPPER_PWM_FREQ_HZ,      // 占位初始值，运行时会更新
            .clk_cfg          = LEDC_USE_APB_CLK                    // 直接指定 80 MHz APB
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf = {
            .gpio_num       = Big_ROTATE_STEPPER_STEP_GPIO,
            .speed_mode     = Big_ROTATE_STEPPER_PWM_MODE,
            .channel        = Big_ROTATE_STEPPER_PWM_CHANNEL,
            .timer_sel      = Big_ROTATE_STEPPER_PWM_TIMER,
            .duty           = 0,
            .hpoint         = 0
    };
    ledc_channel_config(&channel_conf);

    // DIR 和 EN 引脚
    gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << Big_ROTATE_STEPPER_DIR_GPIO) | (1ULL << Big_ROTATE_STEPPER_EN_GPIO),
    };
    gpio_config(&io_conf);

    // 默认失能
    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 0);
}

// 角度 + 转速 + 是否检测零点传感器控制函数
void Big_ROTATE_stepper_rotate(float angle_deg, float rpm, int dir, bool check_sensor) {
    // 计算需要的步数
    float steps_needed = (angle_deg / 360.0f) * Big_ROTATE_STEPS_PER_REV;

    // 计算 PWM 频率
    float pwm_freq = (rpm / 60.0f) * Big_ROTATE_STEPS_PER_REV;

    // 运行时间（秒）
    float run_time_sec = steps_needed / pwm_freq * 50;

    ESP_LOGI(TAG, "Angle=%.1f deg, RPM=%.1f, Freq=%.2f Hz, Time=%.3f s",
             angle_deg, rpm, pwm_freq, run_time_sec);

    // 设置方向
    gpio_set_level(Big_ROTATE_STEPPER_DIR_GPIO, dir ? 1 : 0);
    // 使能
    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 1);
    // 设置 PWM 频率
    ledc_set_freq(Big_ROTATE_STEPPER_PWM_MODE, Big_ROTATE_STEPPER_PWM_TIMER, (uint32_t) pwm_freq);
    // 启动 PWM
    ledc_set_duty(Big_ROTATE_STEPPER_PWM_MODE, Big_ROTATE_STEPPER_PWM_CHANNEL, Big_ROTATE_STEPPER_PWM_DUTY);
    ledc_update_duty(Big_ROTATE_STEPPER_PWM_MODE, Big_ROTATE_STEPPER_PWM_CHANNEL);

    if (check_sensor == 1) {
        // 循环检测传感器状态，期间延时小段时间，模拟非阻塞等待
        const TickType_t delay_ticks = pdMS_TO_TICKS(10); // 10ms检测一次
        TickType_t elapsed_ticks = 0;
        TickType_t total_ticks = pdMS_TO_TICKS((uint32_t) (run_time_sec * 1000));

        while (elapsed_ticks < total_ticks) {
            if (sensor_Calibration_get_state() == 0) {
                ESP_LOGI(TAG, "传感器检测到状态为0，停止电机");
                break;
            }
            vTaskDelay(delay_ticks);
            elapsed_ticks += delay_ticks;
        }
    } else if (check_sensor == 0) {
        vTaskDelay(pdMS_TO_TICKS((uint32_t)(run_time_sec * 1000)));
    }
    ESP_LOGI(TAG, "到达校准位，停止电机");

    // 停止 PWM
    ledc_set_duty(Big_ROTATE_STEPPER_PWM_MODE, Big_ROTATE_STEPPER_PWM_CHANNEL, 0);
    ledc_update_duty(Big_ROTATE_STEPPER_PWM_MODE, Big_ROTATE_STEPPER_PWM_CHANNEL);

    // 失能
    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 0);
}