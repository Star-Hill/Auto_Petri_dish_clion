//
// Created by DELL on 2025/8/7.
//

#include "LeftRight_motor_control.h"

void LeftRight_motor_auto_loop(void ) {
    //初始化传感器
    sensor_ALL_init();
    // 初始化并使能左右电机
    LeftRight_init();                   //左右电机初始化
    while (1) {
        LeftRight_Clear_the_fault();         // 清除故障
        LeftRight_set_speed_Mode(0xC4);     // 设置伺服电机速度模式
        LeftRight_Start();                   // 启动伺服电机

        // 1. 左移，直到左一传感器触发
        LeftRight_set_speed(-200);           // 左移
        while (sensor_Left_get_state() != 0) {
            vTaskDelay(pdMS_TO_TICKS(10));   // 每10ms检查一次传感器
        }
        LeftRight_set_speed(0);              // 停止
        vTaskDelay(pdMS_TO_TICKS(2000));    // 停2秒

        // 2. 右移，直到左二传感器触发
        LeftRight_set_speed(200);            // 右移
        while (sensor_Middle_get_state() != 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        LeftRight_set_speed(0);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 3. 继续右移，直到左三传感器触发
        LeftRight_set_speed(200);            // 继续右移
        while (sensor_Right_get_state() != 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        LeftRight_set_speed(0);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 4. 左移，直到左二传感器触发
        LeftRight_set_speed(-200);            // 左移
        while (sensor_Middle_get_state() != 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        LeftRight_set_speed(0);
        vTaskDelay(pdMS_TO_TICKS(2000));

        LeftRight_Stop();                    // 停止伺服电机
        LeftRight_Clear_the_fault();        // 清除故障
    }

}