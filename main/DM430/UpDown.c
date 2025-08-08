//
// Created by DELL on 2025/8/6.
//

#include "UpDown.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "UpDownMotor"

void UpDown_motor_driver_init(void) {
    // 配置 EN 和 DIR GPIO
    gpio_config_t io_conf = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << UpDown_STEPPER_EN_GPIO) | (1ULL << UpDown_STEPPER_DIR_GPIO),
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    // 初始化 LEDC（PWM）用于 PUL 脉冲输出
    ledc_timer_config_t ledc_timer = {
            .duty_resolution = UpDown_STEPPER_PWM_RESOLUTION,
            .freq_hz = UpDown_STEPPER_PWM_FREQ_HZ,
            .speed_mode = UpDown_STEPPER_PWM_MODE,
            .timer_num = UpDown_STEPPER_PWM_TIMER,
            .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
            .channel    = UpDown_STEPPER_PWM_CHANNEL,
            .duty       = 0,
            .gpio_num   = UpDown_STEPPER_PUL_GPIO,
            .speed_mode = UpDown_STEPPER_PWM_MODE,
            .hpoint     = 0,
            .timer_sel  = UpDown_STEPPER_PWM_TIMER,
            .flags.output_invert = 0
    };
    ledc_channel_config(&ledc_channel);

    //ESP_LOGI(TAG, "UpDown_MOTOR driver initialized.");
}

void UpDown_motor_set_direction(int dir) {
    gpio_set_level(UpDown_STEPPER_DIR_GPIO, dir);
}

void UpDown_motor_enable(int enable) {
    gpio_set_level(UpDown_STEPPER_EN_GPIO, enable);
}

void UpDown_motor_set_speed(uint32_t duty) {
    if (duty > (1 << UpDown_STEPPER_PWM_RESOLUTION)) {
        duty = (1 << UpDown_STEPPER_PWM_RESOLUTION);
    }
    ledc_set_duty(UpDown_STEPPER_PWM_MODE, UpDown_STEPPER_PWM_CHANNEL, duty);
    ledc_update_duty(UpDown_STEPPER_PWM_MODE, UpDown_STEPPER_PWM_CHANNEL);
}

void UpDown_motor_start(void) {
    UpDown_motor_set_speed(UpDown_STEPPER_PWM_DUTY); // 启动默认速度
}

void UpDown_motor_stop(void) {
    UpDown_motor_set_speed(0);  // 占空比为0，停止输出脉冲
}

void UpDown_motor_test(void) {
    /* ***********    上下电机测试   ************ */
    UpDown_motor_driver_init();
    UpDown_motor_enable(1);              // 1--使能   0--失能

    UpDown_motor_set_direction(0);          // 设置方向     0--下  1--上
    UpDown_motor_start();                       // 开始转动
    vTaskDelay(pdMS_TO_TICKS(2000));
    UpDown_motor_stop();                        // 停止

    vTaskDelay(pdMS_TO_TICKS(1000));        //等一秒

    UpDown_motor_set_direction(1);          // 设置方向     0--下  1--上
    UpDown_motor_start();                       // 开始转动
    vTaskDelay(pdMS_TO_TICKS(2000));
    UpDown_motor_stop();                        // 停止
}