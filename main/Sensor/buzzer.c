//
// Created by StarHill on 2025/11/12.
//

#include "buzzer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUZZER";

// 初始化蜂鸣器
void buzzer_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(BUZZER_GPIO, 0);  // 默认关闭
    ESP_LOGI(TAG, "蜂鸣器初始化完成 (GPIO %d)", BUZZER_GPIO);
}

// 打开蜂鸣器（高电平发声）
void buzzer_on(void) {
    gpio_set_level(BUZZER_GPIO, 1);
}

// 关闭蜂鸣器（低电平静音）
void buzzer_off(void) {
    gpio_set_level(BUZZER_GPIO, 0);
}

// 发声一段时间（单位：毫秒）
void buzzer_beep_music(void) {
    for (int i = 0; i < 3; ++i) {
        buzzer_on();
        vTaskDelay(pdMS_TO_TICKS(1000));
        buzzer_off();
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_LOGI(TAG, "蜂鸣器开始唱歌");
    }

}
