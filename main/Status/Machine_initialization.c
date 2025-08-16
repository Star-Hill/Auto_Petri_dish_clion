//
// Created by DELL on 2025/8/11.
//
#include "Machine_initialization.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG_LEFTRIGHT = "LEFTRIGHT_MOTOR_CTRL";
/*
 * 先向上找到传感器位置，再向下1300转，先左走两秒。在找到最右的位置，在向上走1300，
 * */
void Machine_initialization(float gear){
    /************************   升降电机--找下位置    ***************************/
    UpDown_stepper_rotate(3600.0f,50.0f * gear,0,2);

    /************************   柱体电机--回零位置    ***************************/
    Big_ROTATE_stepper_rotate(360.0f, 60.0f * gear, 1,1); //柱体回零

    /****************   左右电机--左移中位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机左移到中或者左位置");
    LeftRight_set_speed(-400 * gear);
    while (sensor_Middle_get_state() != 0 && sensor_Left_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机开始刷新右限位");

    /************************   升降电机--找上原点位置    ***************************/
    UpDown_stepper_rotate(3600.0f,50.0f * gear,1,1);      //六秒

    /************************   升降电机--到最低，但不撞击      解释：刷新原点位置    ***************************/
    UpDown_stepper_rotate(1290.0f,50.0f * gear,0,0);

    /****************   左右电机--右位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机右移到最右位置");
    LeftRight_set_speed(400 * gear);

    while (sensor_Right_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机到达最右限位");

}





