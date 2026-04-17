/*
 * @Author: Stathill星丘 && cishaxiatian@gmail.com
 * @Date: 2026-02-10 16:00:32
 * @LastEditors: Stathill星丘 && cishaxiatian@gmail.com
 * @LastEditTime: 2026-03-18 22:42:20
 * @FilePath: \Auto_Petri_dish_clion\main\Status\Machine_initialization.c
 * @Description: 
 */
//
// Created by DELL on 2025/8/11.
//
#include "Machine_initialization.h"
#define INIT_EVENT_LEFT_RIGHT_DONE (1 << 0)
#define INIT_EVENT_COLUMN_DONE (1 << 1)

static EventGroupHandle_t init_event_group = NULL;

/**
 * @brief 柱体电机回零任务
 */
static void column_motor_task(void *arg)
{
    ESP_LOGI("INIT", "柱体电机开始回零...");
    Big_ROTATE_stepper_rotate_US(360.0f, 4.0f, 1, true);
    ESP_LOGI("INIT", "柱体电机回零完成。");
    // 设置事件标志位
    xEventGroupSetBits(init_event_group, INIT_EVENT_COLUMN_DONE);
    vTaskDelete(NULL);
}

/**
 * @brief 左右伺服电机任务
 */
static void left_right_motor_task(void *arg)
{
    ESP_LOGI("INIT", "左右伺服电机开始回零...");

    SERVO_MOTOR_POS_Reg((int)(1000), -15000, 0, NULL);
    SERVO_MOTOR_POS_Reg((int)(100), 600000, 0, true);
    SERVO_MOTOR_Clear_Position();                     // 位置强制清零
    SERVO_MOTOR_POS_Reg((int)(200), -10000, 0, NULL); // 回一点校准
    ESP_LOGI("INIT", "左右伺服电机回零完成。");

    // 设置事件标志位
    xEventGroupSetBits(init_event_group, INIT_EVENT_LEFT_RIGHT_DONE);
    vTaskDelete(NULL);
}

/**
 * @brief 设备初始化流程（顺序+并发逻辑）
 */
void Machine_initialization(void)
{
    ESP_LOGI("INIT", "=== 开始设备回零 ===");

    if (init_event_group == NULL)
    {
        init_event_group = xEventGroupCreate();
    }

    // Step 1: 升降电机到下位置（阻塞）
    // 先上移后下移
    UpDown_stepper_rotate(300.0f, 100.0f, 1, 0);
    ESP_LOGI("INIT", "升降电机下移开始...");
    UpDown_stepper_rotate(3600.0f, 20.0f, 0, 2);
    ESP_LOGI("INIT", "升降电机下移完成。");

    // Step 2: 并发执行两个回零任务
    xTaskCreate(column_motor_task, "column_motor_task", 4096, NULL, 5, NULL);
    xTaskCreate(left_right_motor_task, "left_right_motor_task", 4096, NULL, 5, NULL);

    // Step 3: 等待两个任务都完成
    ESP_LOGI("INIT", "等待两个电机回零完成...");
    xEventGroupWaitBits(init_event_group,
                        INIT_EVENT_LEFT_RIGHT_DONE | INIT_EVENT_COLUMN_DONE,
                        pdTRUE, // 清除标志位
                        pdTRUE, // 等待两个事件都到达
                        portMAX_DELAY);
    ESP_LOGI("INIT", "两个电机回零已完成！");
}
