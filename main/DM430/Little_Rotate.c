//
// Created by DELL on 2025/8/6.
//

#include "Little_Rotate.h"

static const char *TAG = "Little_ROTATE_STEPPER_CTRL";

// 步进电机初始化
void Little_stepper_init(void) {
    // STEP 引脚配置为 PWM
    ledc_timer_config_t timer_conf = {
            .speed_mode       = Little_ROTATE_STEPPER_PWM_MODE,
            .timer_num        = Little_ROTATE_STEPPER_PWM_TIMER,
            .duty_resolution  = Little_ROTATE_STEPPER_PWM_RESOLUTION,
            .freq_hz          = Little_ROTATE_STEPPER_PWM_FREQ_HZ,      // 占位初始值，运行时会更新
            .clk_cfg          = LEDC_USE_APB_CLK                    // 直接指定 80 MHz APB
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf = {
            .gpio_num       = Little_ROTATE_STEPPER_STEP_GPIO,
            .speed_mode     = Little_ROTATE_STEPPER_PWM_MODE,
            .channel        = Little_ROTATE_STEPPER_PWM_CHANNEL,
            .timer_sel      = Little_ROTATE_STEPPER_PWM_TIMER,
            .duty           = 0,
            .hpoint         = 0
    };
    ledc_channel_config(&channel_conf);

    // DIR 和 EN 引脚
    gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << Little_ROTATE_STEPPER_DIR_GPIO) | (1ULL << Little_ROTATE_STEPPER_EN_GPIO),
    };
    gpio_config(&io_conf);

    // 默认失能
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 0);
}

// 角度 + 转速控制函数
void Little_stepper_rotate(float angle_deg, float rpm, int dir) {
    // 计算需要的步数
    float steps_needed = (angle_deg / 360.0f) * Little_STEPS_PER_REV;

    // 计算 PWM 频率
    float pwm_freq = (rpm / 60.0f) * Little_STEPS_PER_REV;

    // 运行时间（秒）
    float run_time_sec = steps_needed / pwm_freq * 4;

    ESP_LOGI(TAG, "Angle=%.1f deg, RPM=%.1f, Freq=%.2f Hz, Time=%.3f s",
             angle_deg, rpm, pwm_freq, run_time_sec);

    // 设置方向
    gpio_set_level(Little_ROTATE_STEPPER_DIR_GPIO, dir ? 1 : 0);
    // 使能
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 0);
    // 设置 PWM 频率
    ledc_set_freq(Little_ROTATE_STEPPER_PWM_MODE, Little_ROTATE_STEPPER_PWM_TIMER, (uint32_t) pwm_freq);
    // 启动 PWM
    ledc_set_duty(Little_ROTATE_STEPPER_PWM_MODE, Little_ROTATE_STEPPER_PWM_CHANNEL, Little_ROTATE_STEPPER_PWM_DUTY);
    ledc_update_duty(Little_ROTATE_STEPPER_PWM_MODE, Little_ROTATE_STEPPER_PWM_CHANNEL);
    // 延时运行
    vTaskDelay(pdMS_TO_TICKS(run_time_sec * 1000));
    // 停止 PWM
    ledc_set_duty(Little_ROTATE_STEPPER_PWM_MODE, Little_ROTATE_STEPPER_PWM_CHANNEL, 0);
    ledc_update_duty(Little_ROTATE_STEPPER_PWM_MODE, Little_ROTATE_STEPPER_PWM_CHANNEL);

    // 失能
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 1);
}