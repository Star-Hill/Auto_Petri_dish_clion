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

/*  通用运行函数
 * 1,1开上     1，0关上      0,1开上    0,0关上
 * */
void Pump_Valve_run_combo(bool up_down, bool on_off) {
    if (up_down == 1) { // 上
        if (on_off == 1) { // 开
            ESP_LOGI(TAG_PV, "打开 上电磁阀");
            Valve_Up_on();
            vTaskDelay(pdMS_TO_TICKS(10));
            ESP_LOGI(TAG_PV, "打开 上气泵");
            Pump_Up_on();
            vTaskDelay(pdMS_TO_TICKS(10));
        } else if (on_off == 0) {
            ESP_LOGI(TAG_PV, "关闭 上气泵");
            Pump_Up_off();
            vTaskDelay(pdMS_TO_TICKS(10));
            ESP_LOGI(TAG_PV, "关闭 上电磁阀");
            Valve_Up_off();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    } else if (up_down == 0) {
        if (on_off) {
            ESP_LOGI(TAG_PV, "打开 下电磁阀");
            Valve_Down_on();
            vTaskDelay(pdMS_TO_TICKS(10));
            ESP_LOGI(TAG_PV, "打开 下气泵");
            Pump_Down_on();
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            ESP_LOGI(TAG_PV, "关闭 下气泵");
            Pump_Down_off();
            vTaskDelay(pdMS_TO_TICKS(10));
            ESP_LOGI(TAG_PV, "关闭 下电磁阀");
            Valve_Down_off();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}


// 循环测试函数
void Pump_Valve_test(void) {
    Pump_driver_init();
    valve_driver_init();

    while (1) {
        Pump_Valve_run_combo(1, 1);         //上开
        vTaskDelay(pdMS_TO_TICKS(1000));      // 间隔
        Pump_Valve_run_combo(1, 0);         //下开

        Pump_Valve_run_combo(0, 1);         //下开
        vTaskDelay(pdMS_TO_TICKS(1000));      // 间隔
        Pump_Valve_run_combo(0, 0);         //下关
    }
}