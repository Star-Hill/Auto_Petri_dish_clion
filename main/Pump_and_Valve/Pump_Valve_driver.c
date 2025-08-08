//
// Created by DELL on 2025/8/7.
//

#include "Pump_Valve_driver.h"
#include "Pump_driver.h"
#include "Valve_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG_PV "Pump_Valve"

// 通用运行函数
void Pump_Valve_run_combo(bool is_up, uint32_t duration_ms) {
    if (is_up) {
        ESP_LOGI(TAG_PV, "打开 上电磁阀");
        Valve_Up_on();
        vTaskDelay(pdMS_TO_TICKS(200));

        ESP_LOGI(TAG_PV, "打开 上气泵");
        Pump_Up_on();

        vTaskDelay(pdMS_TO_TICKS(duration_ms));

        ESP_LOGI(TAG_PV, "关闭 上气泵");
        Pump_Up_off();
        ESP_LOGI(TAG_PV, "关闭 上电磁阀");
        Valve_Up_off();
    } else {
        ESP_LOGI(TAG_PV, "打开 下电磁阀");
        Valve_Down_on();
        vTaskDelay(pdMS_TO_TICKS(200));

        ESP_LOGI(TAG_PV, "打开 下气泵");
        Pump_Down_on();

        vTaskDelay(pdMS_TO_TICKS(duration_ms));

        ESP_LOGI(TAG_PV, "关闭 下气泵");
        Pump_Down_off();
        ESP_LOGI(TAG_PV, "关闭 下电磁阀");
        Valve_Down_off();
    }
}

// 循环测试函数
void Pump_Valve_test(void) {
    Pump_driver_init();
    valve_UpDown_driver_init();

    while (1) {
        Pump_Valve_run_combo(false, 5000);  // 下泵组合，运行2秒
        vTaskDelay(pdMS_TO_TICKS(500));     // 间隔

        Pump_Valve_run_combo(true, 2000);   // 上泵组合，运行2秒
        vTaskDelay(pdMS_TO_TICKS(1000));    // 间隔
    }
}