//
// Created by DELL on 2025/8/7.
//

#include "Little_Rotate.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "Little_Rotate_MOTOR"

void Little_Rotate_motor_driver_init(void) {
    // 配置 EN 和 DIR GPIO
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
            .gpio_num   = Little_Rotate_STEPPER_Step_GPIO,
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
    gpio_set_level(Little_Rotate_STEPPER_EN_GPIO, enable);  // 共阳极：低电平使能
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
    /* ***********    旋转电机测试   ************ */
    Little_Rotate_motor_driver_init();
    Little_Rotate_motor_enable(0);              // 1--使能   0--失能
    Little_Rotate_motor_set_direction(0);          // 设置为正转0--下  1--上

    Little_Rotate_motor_start();                       // 开始转动
    vTaskDelay(pdMS_TO_TICKS(3000));
    Little_Rotate_motor_stop();                        // 停止
}