//
// Created by DELL on 2025/8/7.
//

#include "Valve_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define TAG "Valve_TEST"

// 初始化电磁阀相关 GPIO
void valve_driver_init(void) {
    gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << Valve_Down_GPIO) | (1ULL << Valve_Up_GPIO),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // 初始状态：关闭
    gpio_set_level(Valve_Down_GPIO, 0);
    gpio_set_level(Valve_Up_GPIO, 0);
}

// 控制电磁阀1
void Valve_Down_on(void) { gpio_set_level(Valve_Down_GPIO, 1); }

void Valve_Down_off(void) { gpio_set_level(Valve_Down_GPIO, 0); }

// 控制电磁阀2
void Valve_Up_on(void) { gpio_set_level(Valve_Up_GPIO, 1); }

void Valve_Up_off(void) { gpio_set_level(Valve_Up_GPIO, 0); }

//测试电磁阀程序
void Valve_test(void) {
    valve_driver_init();  // 初始化 GPIO

    while (1) {
        ESP_LOGI(TAG, "打开电磁阀1（下）");
        Valve_Down_on();
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "关闭电磁阀1（下）");
        Valve_Down_off();
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_LOGI(TAG, "打开电磁阀2（上）");
        Valve_Up_on();
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_LOGI(TAG, "关闭电磁阀2（上）");
        Valve_Up_off();
        vTaskDelay(pdMS_TO_TICKS(1000));  // 一个完整循环的间隔
    }
}

