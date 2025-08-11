//
// Created by DELL on 2025/8/11.
//

#include "Instrument_starts_canning.h"


static const char *TAG_UPDOWN = "UPDOWN_MOTOR_CTRL";
static const char *TAG_LEFTRIGHT = "LEFTRIGHT_MOTOR_CTRL";
static const char *TAG_PV = "Pump_Valve";
static const char *TAG_SYSTEM = "SYSTEM--Notice";

void task_rotate(void *pvParameters) {
    Rotate_motor_CALIBRATION(90.0f, 6.0f, false);
    vTaskDelete(NULL); // 任务完成后删除
}

void task_pick(void *pvParameters) {
    check_and_pick_plate();
    vTaskDelete(NULL);
}

// 函数返回值：1 表示成功取到培养皿，0 表示失败
int check_and_pick_plate(void) {
    /****************   升降电机--下限位置    *******************/
    UpDown_motor_enable(1); // 使能
    ESP_LOGI(TAG_UPDOWN, "升降电机开始向下运动");
    UpDown_motor_set_direction(0); // 向下
    UpDown_motor_start();

    TickType_t startTick = xTaskGetTickCount();
    while (sensor_Down_get_state() != 0) {
        if ((xTaskGetTickCount() - startTick) > pdMS_TO_TICKS(5000)) { // 5秒超时保护
            ESP_LOGE(TAG_UPDOWN, "升降电机下移超时！");
            UpDown_motor_stop();
            return 0;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGI(TAG_UPDOWN, "升降电机到达下限位，停止升降电机");
    UpDown_motor_stop();

    /****************   左右电机--中位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机左移到中间位置");
    LeftRight_set_speed(-400);

    startTick = xTaskGetTickCount();
    while (sensor_Middle_get_state() != 0) {
        if ((xTaskGetTickCount() - startTick) > pdMS_TO_TICKS(5000)) {
            ESP_LOGE(TAG_LEFTRIGHT, "左右电机到中间位置超时！");
            LeftRight_set_speed(0);
            return 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机到达中间限位");

    /****************   左右电机--右位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机右移到最右位置");
    LeftRight_set_speed(400);

    startTick = xTaskGetTickCount();
    while (sensor_Right_get_state() != 0) {
        if ((xTaskGetTickCount() - startTick) > pdMS_TO_TICKS(5000)) {
            ESP_LOGE(TAG_LEFTRIGHT, "左右电机到右限位超时！");
            LeftRight_set_speed(0);
            return 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机到达最右限位");

    /***************   打开下磁阀和气泵电机    ******************/
    ESP_LOGI(TAG_PV, "打开下电磁阀");
    Valve_Down_on();
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG_PV, "打开下气泵");
    Pump_Down_on();
    vTaskDelay(pdMS_TO_TICKS(10));

    /****************   升降电机--吸取位置    *******************/
    UpDown_motor_enable(1);
    ESP_LOGI(TAG_UPDOWN, "升降电机开始向上吸取运动");
    UpDown_motor_set_direction(1);
    UpDown_motor_start();

    vTaskDelay(pdMS_TO_TICKS(3300)); // 向上运动时间
    UpDown_motor_stop();
    ESP_LOGI(TAG_UPDOWN, "升降电机到达取出位");

    /****************   左右电机--取出后移动中位置    *******************/
    LeftRight_Clear_the_fault();         // 清除故障
    LeftRight_set_speed_Mode(0xC4);     // 设置伺服电机速度模式
    LeftRight_Start();                   // 启动左右伺服电机
    // 取出后左移
    LeftRight_set_speed(-400);            // 左移


    // 三秒检测
    startTick = xTaskGetTickCount();
    const TickType_t timeoutTicks = pdMS_TO_TICKS(3000); // 3秒

    while (sensor_Entrance_get_state() != 0) {
        if ((xTaskGetTickCount() - startTick) > timeoutTicks) {
            ESP_LOGW(TAG_SYSTEM, "3秒内未检测到培养皿");
            LeftRight_set_speed(0);
            LeftRight_Stop();
            return 0; // 超时直接返回
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

/*
 * 这里判断，检测入口传感器是否有盘
 * */
    if (sensor_Entrance_get_state() == 0) {
        ESP_LOGI(TAG_SYSTEM, "成功取到培养皿");
        // 左右电机继续移动到中间指定位置
        while (sensor_Middle_get_state() != 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        LeftRight_set_speed(0);
        LeftRight_Stop();
        ESP_LOGI(TAG_LEFTRIGHT, "左右电机到达中间限位，停止左右电机");
        ESP_LOGI(TAG_LEFTRIGHT, "左右电机吸取到培养皿并到达到中间位置，停止左右电机");
        return 1;
    } else {
        ESP_LOGW(TAG_SYSTEM, "未检测到培养皿");
        LeftRight_set_speed(0);
        LeftRight_Stop();
        return 0;
    }
}


/*
 * 成功后的操作
 * */
void Success(void) {

    ESP_LOGW(TAG_SYSTEM, "准备启动升降电机到上位置吸住上盖，开盖操作");
}


void Instrument_starts_canning(void) {
    /****************   初始化    *******************/

    /*
     * 检查是否取到培养皿
     */
    if (check_and_pick_plate()) {
        /*
         * 成功
         */
        Success();
    }
        /*
         * 失败则重新来
         */
    else {
        ESP_LOGW(TAG_SYSTEM, "当前柱体无空培养皿了");

        // 创建旋转任务
        xTaskCreate(task_rotate, "RotateTask", 4096, NULL, 5, NULL);

        // 创建取盘任务
        xTaskCreate(task_pick, "PickTask", 4096, NULL, 5, NULL);
    }
}
