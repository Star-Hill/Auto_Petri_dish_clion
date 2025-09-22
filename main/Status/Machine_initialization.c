//
// Created by DELL on 2025/8/11.
//
#include "Machine_initialization.h"
#include "HMI_control_driver.h"
#include "freertos/event_groups.h"

// ================= 同步事件组 =================
static EventGroupHandle_t motor_event_group;
#define MOTOR1_DONE BIT0
#define MOTOR2_DONE BIT1

/**
* @CreateTime 2025/9/12
* @Author Star-Hill
* @brief 柱体电机回零
*/
void task_column_motor(void *pvParameters) {
    Big_ROTATE_stepper_rotate(360.0f, 2.0f, 1, true, 500);
    //before      I (469) Big_Stepping: angle=360.0 deg, rpm=2.0, steps=160000, freq=5333 Hz

    // 通知完成
    xEventGroupSetBits(motor_event_group, MOTOR1_DONE);

    vTaskDelete(NULL);
}

/**
* @CreateTime 2025/9/12
* @Author Star-Hill
* @brief 伺服电机右位置
*/
void task_left_right_motor(void *pvParameters) {
    SERVO_MOTOR_Move_Position_Speed((int)(1500), -15000, 0, NULL);
    SERVO_MOTOR_Move_To_Position(sensor_Right_get_state, 200, "右位置");

    // 位置强制清零
    SERVO_MOTOR_Clear_Position();

    // 回一点校准
    SERVO_MOTOR_Move_Position_Speed((int)(100), -5850, 0, NULL);

    // 通知完成
    xEventGroupSetBits(motor_event_group, MOTOR2_DONE);

    vTaskDelete(NULL);
}

/**
* @CreateTime 2025/9/12
* @Author Star-Hill
* @brief 伺服电机右位置 && 柱体电机回零 同时进行，并等待两者完成
*/
void start_both_motors(void *pvParameters) {
    // 清空事件组标志位
    xEventGroupClearBits(motor_event_group, MOTOR1_DONE | MOTOR2_DONE);

    // 创建两个并行任务
    xTaskCreate(task_column_motor, "task_column_motor", 4096, NULL, 5, NULL);
    xTaskCreate(task_left_right_motor, "task_left_right_motor", 4096, NULL, 5, NULL);

    // 等待两个任务都完成
    xEventGroupWaitBits(
            motor_event_group,
            MOTOR1_DONE | MOTOR2_DONE,
            pdTRUE,   // 退出时清除标志位
            pdTRUE,   // 必须两个任务都完成
            portMAX_DELAY
    );

    // 两个任务完成后，再切换页面
    uart_hmi_send("page 12");

    vTaskDelete(NULL); // 自删
}

/**
* @CreateTime 2025/9/12
* @Author Star-Hill
* @brief 设备初始化流程
*/
void Machine_initialization(void) {
    // 升降电机到下位置
    UpDown_stepper_rotate(3600.0f, 20.0f, 0, 2);

    // 创建事件组
    if (motor_event_group == NULL) {
        motor_event_group = xEventGroupCreate();
    }

    // 启动并行电机任务
    xTaskCreate(start_both_motors, "start_both_motors", 4096, NULL, 5, NULL);
}
