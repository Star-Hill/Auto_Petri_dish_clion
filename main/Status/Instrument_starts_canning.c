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
    Peristaltic_pump_set_speed(100);
    vTaskDelay(pdMS_TO_TICKS(4000));
    Peristaltic_pump_Control(0);
    ESP_LOGI(TAG_SYSTEM, "蠕动泵输出营养液 完毕");
    task_done_count++;
    vTaskDelete(NULL);  // 任务结束自杀
}

// 任务2：控制小旋转电机      两圈
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
    valve_driver_init();             //  电磁阀初始化
}

// 函数返回值：1 表示成功取到培养皿，0 表示失败
int check_and_pick_plate(void) {
    /****************   升降电机--吸取位置    *******************/
    UpDown_stepper_rotate(800.0f, 50.0f, 1, 0);

    /***************   打开下磁阀和气泵电机    ******************/
    Pump_Valve_run_combo(0, 1);

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
    UpDown_stepper_rotate(600.0f, 50.0f, 1, 1);      //上移动检测上传感器

    /***************   打开上磁阀和上气泵电机    ******************/
    Pump_Valve_run_combo(1, 1);

    /************************   升降电机--中限位置    ***************************/
    UpDown_stepper_rotate(400.0f, 50.0f, 0, 0);      //上下电机开盖位置

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

    /****************   左右电机--左-->中位置   逆时针   *******************/
    Little_stepper_rotate(360.0f, 60.0f, 0);
    ESP_LOGI(TAG_SYSTEM, "旋转培养皿开始逆时针旋转");

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
    UpDown_stepper_rotate(600.0f, 50.0f, 1, 1);      //上移动检测上传感器

    /***************   关闭上磁阀和上气泵电机    ******************/
    Pump_Valve_run_combo(1, 0);

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
    UpDown_stepper_rotate(2000.0f, 50.0f, 1, 1);      //上移动检测上传感器

    /***************   关闭下磁阀和下气泵电机    ******************/
    Pump_Valve_run_combo(0, 0);

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(1290.0f, 50.0f, 0, 0);        //到达最低

    /****************   柱体电机--转-45度  空盒子    *******************/
    Big_ROTATE_stepper_rotate(45.0f, 120.0f, 0, 0);

}

/*
 * 失败后的操作
 * */
int column_Failure_Num = 1;

void Failure(void) {
    ESP_LOGW(TAG_SYSTEM, "当前柱体无空培养皿了");

    /***************   关闭下磁阀和气泵电机    ******************/
    Pump_Valve_run_combo(0, 0);

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(800.0f, 50.0f, 0, 0);     //dir = 0 向下移动

    /****************   柱体电机--转90度位置    *******************/
    if (column_Failure_Num == 2) {
        Big_ROTATE_stepper_rotate(90.0f, 120.0f, 1, 0);
        column_Failure_Num = 1;
    } else {
        column_Failure_Num++;
    }

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
    for (int column = 0; column < 4; column++) {  // 4个柱子
        int failCount_single_column = 0;  // 单个柱子的失败计数

        ESP_LOGI(TAG_SYSTEM, "开始检测第 %d 根柱子", column + 1);

        for (int attempt = 0; attempt < 2; attempt++) { // 每根柱子检测2次
            int result = check_and_pick_plate();
            if (result == 1) { // 成功
                Success();
                failCount_single_column = 0; // 成功就清零
                ESP_LOGI(TAG_SYSTEM, "第 %d 根柱子第 %d 次检测成功", column + 1, attempt + 1);
                break; // 成功一次就不再检测该柱子，直接下一根
            }
            if (result == 0) { // 失败
                Failure();
                failCount_single_column++;
                ESP_LOGW(TAG_SYSTEM, "第 %d 根柱子第 %d 次检测失败", column + 1, attempt + 1);
            }
        }

        if (failCount_single_column == 2) { // 两次都失败
            ESP_LOGW(TAG_SYSTEM, "第 %d 根柱子连续两次检测失败", column + 1);
        }
        ESP_LOGI(TAG_SYSTEM, "第 %d 根柱子检测结束", column + 1);
    }
    ESP_LOGI(TAG_SYSTEM, "全部柱子检测完毕");
}

