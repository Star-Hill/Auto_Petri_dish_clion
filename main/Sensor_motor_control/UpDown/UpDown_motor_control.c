//
// Created by DELL on 2025/8/7.
//

#include "UpDown_motor_control.h"

static const char *TAG = "MOTOR_CTRL";

void UpDown_motor_auto_loop(void ) {
    //初始化传感器
    sensor_upDown_init();
    // 初始化并使能电机
    UpDown_motor_driver_init();
    UpDown_motor_enable(1);             //1--使能

    while (1) {
        // 向上运动
        ESP_LOGI(TAG, "开始向上运动");
        UpDown_motor_set_direction(1);
        UpDown_motor_start();

        ESP_LOGI(TAG, "上传感器状态: %d", sensor_Up_get_state());
        ESP_LOGI(TAG, "下传感器状态: %d", sensor_Down_get_state());
        // 等待直到上传感器被遮挡
        while (sensor_Up_get_state() != 0) {
            vTaskDelay(pdMS_TO_TICKS(50)); // 查询间隔
        }

        ESP_LOGI(TAG, "到达上限位，停止电机");
        UpDown_motor_stop();

        vTaskDelay(pdMS_TO_TICKS(2000));  // 停两秒

        // 向下运动
        ESP_LOGI(TAG, "开始向下运动");
        UpDown_motor_set_direction(0);
        UpDown_motor_start();

        // 等待直到下传感器被遮挡
        while (sensor_Down_get_state() != 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        ESP_LOGI(TAG, "到达下限位，停止电机");
        UpDown_motor_stop();
        vTaskDelay(pdMS_TO_TICKS(2000));  // 停两秒

    }
}