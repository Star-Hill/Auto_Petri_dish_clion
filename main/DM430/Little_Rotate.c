//
// Created by DELL on 2025/8/6.
//

#include "Little_Rotate.h"

#define TAG "Little_Rotate_MOTOR"

void Little_Rotate_motor_driver_init(void) {
    // 配置 小旋转电机的    EN 和 DIR GPIO
    gpio_config_t io_conf = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << Little_Rotate_STEPPER_EN_GPIO) | (1ULL << Little_Rotate_STEPPER_DIR_GPIO),
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    // 初始化 LEDC（PWM）用于 PUL 脉冲输出
    ledc_timer_config_t ledc_timer = {
            .duty_resolution = Little_Rotate_STEPPER_PWM_RESOLUTION,
            .freq_hz = Little_Rotate_STEPPER_PWM_FREQ_HZ,
            .speed_mode = Little_Rotate_STEPPER_PWM_MODE,
            .timer_num = Little_Rotate_STEPPER_PWM_TIMER,
            .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
            .channel    = Little_Rotate_STEPPER_PWM_CHANNEL,
            .duty       = 0,
            .gpio_num   = Little_Rotate_STEPPER_STEP_GPIO,
            .speed_mode = Little_Rotate_STEPPER_PWM_MODE,
            .hpoint     = 0,
            .timer_sel  = Little_Rotate_STEPPER_PWM_TIMER,
            .flags.output_invert = 0
    };
    ledc_channel_config(&ledc_channel);

    ESP_LOGI(TAG, "Little_Rotate_MOTOR driver initialized.");
}

void Little_Rotate_motor_set_direction(int dir) {
    gpio_set_level(Little_Rotate_STEPPER_DIR_GPIO, dir);
}

void Little_Rotate_motor_enable(int enable) {
    // 传入1表示使能，0表示禁用
    if (enable) {
        // 使能电机，对应低电平
        gpio_set_level(Little_Rotate_STEPPER_EN_GPIO, 0);
    } else {
        // 禁用电机，对应高电平
        gpio_set_level(Little_Rotate_STEPPER_EN_GPIO, 1);
    }
}

void Little_Rotate_motor_set_speed(uint32_t duty) {
    if (duty > (1 << Little_Rotate_STEPPER_PWM_RESOLUTION)) {
        duty = (1 << Little_Rotate_STEPPER_PWM_RESOLUTION);
    }
    ledc_set_duty(Little_Rotate_STEPPER_PWM_MODE, Little_Rotate_STEPPER_PWM_CHANNEL, duty);
    ledc_update_duty(Little_Rotate_STEPPER_PWM_MODE, Little_Rotate_STEPPER_PWM_CHANNEL);
}

void Little_Rotate_motor_start(void) {
    Little_Rotate_motor_set_speed(Little_Rotate_STEPPER_PWM_DUTY); // 启动默认速度
}

void Little_Rotate_motor_stop(void) {
    Little_Rotate_motor_set_speed(0);  // 占空比为0，停止输出脉冲
}

void Little_Rotate_motor_test(void) {
    /* ***********    小旋转电机测试   ************ */
    Little_Rotate_motor_driver_init();
    Little_Rotate_motor_enable(1);              // 1--使能   0--失能
    Little_Rotate_motor_set_direction(0);          // 设置为正转0--下  1--上

    Little_Rotate_motor_start();                       // 开始转动
    vTaskDelay(pdMS_TO_TICKS(2000));
    Little_Rotate_motor_stop();                        // 停止
}


/*
 * 小旋转旋转到任意角度
 * */
void Little_Rotate_motor_CALIBRATION(float angle_deg, float rpm) {
    const uint32_t motor_full_steps = 200; // 电机整步数
    const uint32_t microsteps = 1;         // 驱动器细分，根据SW5-8设置
    const uint32_t steps_per_rev = motor_full_steps * microsteps;

    // 计算总步数
    uint32_t total_steps = (uint32_t) ((fabs(angle_deg) / 360.0f) * steps_per_rev + 0.5f);
    if (total_steps == 0) return; // 没有步数就直接返回

    // 设置方向
    int dir = (angle_deg >= 0) ? 1 : 0;
    Little_Rotate_motor_set_direction(dir);
    Little_Rotate_motor_enable(1);          //使能

    // 计算PWM频率
    float steps_per_sec = (rpm * motor_full_steps) / 60.0f;
    uint32_t pwm_freq = (uint32_t) (steps_per_sec + 0.5f);

    // 更新LEDC频率
    ledc_set_freq(Little_Rotate_STEPPER_PWM_MODE, Little_Rotate_STEPPER_PWM_TIMER, pwm_freq);

    // 启动PWM
    Little_Rotate_motor_set_speed(Little_Rotate_STEPPER_PWM_DUTY);

    // 计算需要的时间并延时
    float time_sec = (float) total_steps / pwm_freq;
    vTaskDelay(pdMS_TO_TICKS(time_sec * 1000));

    ESP_LOGI(TAG, "已经转完一圈，停止电机");

    // 停止PWM
    Little_Rotate_motor_stop();
    Little_Rotate_motor_enable(0);
}
