//
// Created by DELL on 2025/8/7.
//

#include "Pump_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define TAG_Pump "Pump"

void Pump_driver_init(void) {
    gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << Pump_Down_GPIO) | (1ULL << Pump_Up_GPIO),
            .mode = GPIO_MODE_OUTPUT,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // 默认关闭气泵
    gpio_set_level(Pump_Down_GPIO, 0);
    gpio_set_level(Pump_Up_GPIO, 0);
    ESP_LOGI(TAG_Pump, "气泵初始化完成");
}

// 控制电磁阀1
void Pump_Down_on(void)  { gpio_set_level(Pump_Down_GPIO, 1); }
void Pump_Down_off(void) { gpio_set_level(Pump_Down_GPIO, 0); }

// 控制电磁阀2
void Pump_Up_on(void)  { gpio_set_level(Pump_Up_GPIO, 1); }
void Pump_Up_off(void) { gpio_set_level(Pump_Up_GPIO, 0); }

// 气泵测试程序
void Pump_test(void) {
    Pump_driver_init();

    while (1) {
        ESP_LOGI(TAG_Pump, "打开气泵1（下）");
        Pump_Down_on();
        vTaskDelay(pdMS_TO_TICKS(2000));
        Pump_Down_off();
        ESP_LOGI(TAG_Pump, "关闭气泵1（下）");

        vTaskDelay(pdMS_TO_TICKS(500));  // 间隔

        ESP_LOGI(TAG_Pump, "打开气泵2（上）");
        Pump_Up_on();
        vTaskDelay(pdMS_TO_TICKS(2000));
        Pump_Up_off();
        ESP_LOGI(TAG_Pump, "关闭气泵2（上）");

        vTaskDelay(pdMS_TO_TICKS(1000));  // 一个完整循环后等待
    }
}
