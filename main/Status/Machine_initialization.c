//
// Created by DELL on 2025/8/11.
//
#include "Machine_initialization.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG_UPDOWN = "UPDOWN_MOTOR_CTRL";
static const char *TAG_LEFTRIGHT = "LEFTRIGHT_MOTOR_CTRL";

int Machine_initialization(void){
    bool upDown_done = false;
    bool leftRight_done = false;
    /************************   升降电机--下限位置    ***************************/
    UpDown_motor_enable(1); // 使能
    ESP_LOGI(TAG_UPDOWN, "升降电机开始向下运动");
    UpDown_motor_set_direction(0); // 向下
    UpDown_motor_start();

    while (sensor_Down_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGI(TAG_UPDOWN, "升降电机到达下限位，停止升降电机");
    UpDown_motor_stop();
    upDown_done = true;

    /************************   柱体电机--回零位置    ***************************/
    Rotate_motor_CALIBRATION(360.0f, 6.0f, true); //柱体回零

    /****************   左右电机--左移两秒位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机右移到最右位置");
    LeftRight_set_speed(-400);
    vTaskDelay(pdMS_TO_TICKS(1500));
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机刷新右限位");

    /****************   左右电机--右位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机右移到最右位置");
    LeftRight_set_speed(400);

    while (sensor_Right_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机到达最右限位");
    leftRight_done = true;

    if (upDown_done && leftRight_done) {
        return 1; // 成功初始化
    } else {
        return 0; // 初始化失败
    }
}





