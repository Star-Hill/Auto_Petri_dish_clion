//
// Created by DELL on 2025/8/11.
//

#include "Instrument_starts_canning.h"



// 全局变量计数，或者用信号量
int task_done_count = 0;

static const char *TAG_SYSTEM = "SYSTEM--Notice";

// 任务1：控制蠕动泵
void PeristalticPumpTask(void *pvParameters) {
    Peristaltic_pump_Control(1);
    Peristaltic_pump_set_speed(200);
    vTaskDelay(pdMS_TO_TICKS(4000));
    Peristaltic_pump_Control(0);
    ESP_LOGI(TAG_SYSTEM, "蠕动泵输出营养液 完毕");
    task_done_count++;
    vTaskDelete(NULL);  // 任务结束自杀
}

// 任务2：控制小旋转电机
void LittleRotateMotorTask(void *pvParameters) {
    Little_stepper_rotate(360.0f, 60.0f, 1);
    task_done_count++;
    ESP_LOGI(TAG_SYSTEM, "旋转培养皿 完毕");
    vTaskDelete(NULL);
}



void All_init(void) {
    sensor_ALL_init();                      //  初始化所有传感器
    Big_ROTATE_stepper_init();              //  初始化并使能柱体旋转电机
    Little_stepper_init();                  //  培养皿旋转电机
    UpDown_stepper_init();                  //  初始化并使能升降电机
    LeftRight_init();                       //  左右电机初始化

    Peristaltic_pump_init();               //  蠕动泵

    Pump_driver_init();                     //  泵初始化
    valve_UpDown_driver_init();             //  电磁阀初始化
}

// 函数返回值：1 表示成功取到培养皿，0 表示失败
int check_and_pick_plate(void) {
    /****************   升降电机--吸取位置    *******************/
    UpDown_stepper_rotate(800.0f, 50.0f, 1, 0);


    /***************   打开下磁阀和气泵电机    ******************/
    Valve_Down_on();
    vTaskDelay(pdMS_TO_TICKS(10));
    Pump_Down_on();
    vTaskDelay(pdMS_TO_TICKS(10));


    /****************   左右电机--取出后移动中位置    *******************/
    LeftRight_Clear_the_fault();         // 清除故障
    LeftRight_set_speed_Mode(0xC4);     // 设置伺服电机速度模式
    LeftRight_Start();                   // 启动左右伺服电机
    // 取出后左移
    LeftRight_set_speed(-400);            // 左移
    /*
     * 培养皿检测
     * */
    // 检测是否取到培养皿
    TickType_t startTick = xTaskGetTickCount();  // 在这里定义一次
    const TickType_t timeoutTicks = pdMS_TO_TICKS(4000); // 4秒

    while (sensor_Entrance_get_state() != 0) {
        if ((xTaskGetTickCount() - startTick) > timeoutTicks) {
            ESP_LOGW(TAG_SYSTEM, "4秒内未检测到培养皿");
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
        ESP_LOGI(TAG_SYSTEM, "左右电机吸取到培养皿并到达到中间位置，停止左右电机");
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
    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(3600.0f, 50.0f, 1, 1);      //上移动检测上传感器

    /***************   打开上磁阀和上气泵电机    ******************/
    ESP_LOGI(TAG_SYSTEM, "打开上电磁阀");
    Valve_Up_on();
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG_SYSTEM, "打开上气泵");
    Pump_Up_on();
    vTaskDelay(pdMS_TO_TICKS(10));

    /************************   升降电机--中限位置    ***************************/
    UpDown_stepper_rotate(400.0f, 50.0f, 0, 0);      //下移动

    /****************   左右电机--左位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_SYSTEM, "左右电机右移到最左位置");
    LeftRight_set_speed(-600);

    while (sensor_Left_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_SYSTEM, "左右电机到达最左限位");

    /****************   蠕动泵输出营养液  电机旋转  *******************/
    task_done_count = 0;
    xTaskCreate(PeristalticPumpTask, "PumpTask", 4096, NULL, 5, NULL);
    xTaskCreate(LittleRotateMotorTask, "MotorTask", 4096, NULL, 5, NULL);

    // 等待两个任务完成
    while (task_done_count < 2) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /****************   左右电机--中位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_SYSTEM, "左右电机右移到中间位置");
    LeftRight_set_speed(400);

    while (sensor_Middle_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_SYSTEM, "左右电机到达中间限位");

    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(3600.0f, 50.0f, 1, 1);      //上移动检测上传感器

    /***************   关闭上磁阀和上气泵电机    ******************/
    ESP_LOGI(TAG_SYSTEM, "关闭上气泵");
    Pump_Up_off();
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG_SYSTEM, "关闭上电磁阀");
    Valve_Up_off();
    vTaskDelay(pdMS_TO_TICKS(10));

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(1290.0f, 50.0f, 0, 0);

    /****************   柱体电机--转45度  罐装完毕的柱体    *******************/
    Big_ROTATE_stepper_rotate(45.0f, 120.0f, 1, 0);
    ESP_LOGI(TAG_SYSTEM, "移动到罐装完毕的柱体");

    /****************   左右电机--右位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_SYSTEM, "左右电机右移到最右位置");
    LeftRight_set_speed(400);

    while (sensor_Right_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_SYSTEM, "左右电机到达最右边限位");

/************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(3600.0f, 50.0f, 1, 1);      //上移动检测上传感器

    /***************   关闭下磁阀和下气泵电机    ******************/
    ESP_LOGI(TAG_SYSTEM, "关闭下气泵");
    Pump_Down_off();
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG_SYSTEM, "关闭下电磁阀");
    Valve_Down_off();
    vTaskDelay(pdMS_TO_TICKS(10));

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(1290.0f, 50.0f, 0, 0);

    /****************   柱体电机--转45度  空盒子    *******************/
    Big_ROTATE_stepper_rotate(45.0f, 120.0f, 1, 0);

}

/*
 * 失败后的操作
 * */
void Failure(void) {
    ESP_LOGW(TAG_SYSTEM, "当前柱体无空培养皿了");

    /***************   关闭下磁阀和气泵电机    ******************/
    Valve_Down_off();
    vTaskDelay(pdMS_TO_TICKS(10));
    Pump_Down_off();
    vTaskDelay(pdMS_TO_TICKS(10));

    /****************   柱体电机--转90度位置    *******************/
    Big_ROTATE_stepper_rotate(90.0f, 120.0f, 1, 0);

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(800.0f, 50.0f, 0, 0);     //dir = 0 向下移动

    /****************   左右电机--右位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_SYSTEM, "左右电机右移到最右位置");
    LeftRight_set_speed(400);

    while (sensor_Right_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_SYSTEM, "左右电机到达最右边限位");
}


void Instrument_starts_canning(void) {
    int failCount = 0;  // 连续失败计数器
    int result = check_and_pick_plate();

    while (failCount < 4) {  // 连续失败达到 4 次就退出
        if (result == 1) {
            Success();
            failCount = 0; // 成功则清零失败计数
            ESP_LOGI(TAG_SYSTEM, "成功检测到培养皿，failCount 已重置为 %d", failCount);
        }
        if (result == 0) {
            Failure();
            failCount++;   // 失败计数 +1
            ESP_LOGW(TAG_SYSTEM, "未检测到培养皿，当前 failCount=%d", failCount);
        }
        // 再次检测
        result = check_and_pick_plate();
    }

    ESP_LOGW(TAG_SYSTEM, "连续4次失败，停止操作，failCount=%d", failCount);
    /***************   关闭下磁阀和气泵电机    ******************/
    Valve_Down_off();
    vTaskDelay(pdMS_TO_TICKS(10));
    Pump_Down_off();
    vTaskDelay(pdMS_TO_TICKS(10));
    Machine_initialization();
}
