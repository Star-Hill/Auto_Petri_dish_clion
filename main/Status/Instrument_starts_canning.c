//
// Created by DELL on 2025/8/11.
//

#include "Instrument_starts_canning.h"

// 全局变量计数，或者用信号量
int task_done_count = 0;

static const char *TAG_UPDOWN = "UPDOWN_MOTOR_CTRL";
static const char *TAG_LEFTRIGHT = "LEFTRIGHT_MOTOR_CTRL";
static const char *TAG_PV = "Pump_Valve";
static const char *TAG_SYSTEM = "SYSTEM--Notice";
static const char *TAG_Peristaltic_pump = "Peristaltic_pump";


void task_rotate(void *pvParameters) {
    Rotate_motor_CALIBRATION(90.0f, 6.0f, false);
    vTaskDelete(NULL); // 任务完成后删除
}

void task_pick(void *pvParameters) {
    check_and_pick_plate();
    vTaskDelete(NULL);
}

// 任务1：控制蠕动泵
void PeristalticPumpTask(void *pvParameters) {
    Peristaltic_pump_Control(1);
    Peristaltic_pump_set_speed(200);
    vTaskDelay(pdMS_TO_TICKS(5000));
    Peristaltic_pump_Control(0);
    ESP_LOGI(TAG_Peristaltic_pump, "蠕动泵输出营养液 完毕");
    task_done_count++;
    vTaskDelete(NULL);  // 任务结束自杀
}

// 任务2：控制小旋转电机
void LittleRotateMotorTask(void *pvParameters) {
    Little_Rotate_motor_CALIBRATION(220,6);
    task_done_count++;
    vTaskDelete(NULL);
}


// 函数返回值：1 表示成功取到培养皿，0 表示失败
int check_and_pick_plate(void) {
    /****************   升降电机--吸取位置    *******************/
    UpDown_motor_enable(1);
    ESP_LOGI(TAG_UPDOWN, "升降电机开始向上吸取运动");
    UpDown_motor_set_direction(1);
    UpDown_motor_start();

    /*
     * 如果太高太低培养皿被吸住取不出来，则需要调整时间
     * */
    vTaskDelay(pdMS_TO_TICKS(2900)); // 向上运动时间
    UpDown_motor_stop();
    ESP_LOGI(TAG_UPDOWN, "升降电机到达取出位");

    /***************   打开下磁阀和气泵电机    ******************/
    ESP_LOGI(TAG_PV, "打开下电磁阀");
    Valve_Down_on();
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG_PV, "打开下气泵");
    Pump_Down_on();
    vTaskDelay(pdMS_TO_TICKS(10));



    /****************   左右电机--取出后移动中位置    *******************/
    LeftRight_Clear_the_fault();         // 清除故障
    LeftRight_set_speed_Mode(0xC4);     // 设置伺服电机速度模式
    LeftRight_Start();                   // 启动左右伺服电机
    // 取出后左移
    LeftRight_set_speed(-400);            // 左移


    // 三秒检测
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
    /************************   升降电机--上限位置    ***************************/
    UpDown_motor_enable(1); // 使能
    ESP_LOGI(TAG_UPDOWN, "升降电机开始向上运动");
    UpDown_motor_set_direction(1); // 向上
    UpDown_motor_start();

    while (sensor_Up_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGI(TAG_UPDOWN, "升降电机到达上限位，停止升降电机");
    UpDown_motor_stop();

    /***************   打开上磁阀和上气泵电机    ******************/
    ESP_LOGI(TAG_PV, "打开上电磁阀");
    Valve_Up_on();
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG_PV, "打开上气泵");
    Pump_Up_on();
    vTaskDelay(pdMS_TO_TICKS(10));

    /************************   升降电机--中限位置    ***************************/
    UpDown_motor_enable(1);                 // 使能
    ESP_LOGI(TAG_UPDOWN, "升降电机开始向下运动");
    UpDown_motor_set_direction(0);              // 0下    1上
    UpDown_motor_start();
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG_UPDOWN, "开盖子");
    UpDown_motor_stop();

    /****************   左右电机--左位置    *******************/
    LeftRight_Clear_the_fault();
    LeftRight_set_speed_Mode(0xC4);
    LeftRight_Start();
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机右移到最左位置");
    LeftRight_set_speed(-600);

    while (sensor_Left_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机到达最左限位");

    /****************   蠕动泵输出营养液  电机旋转  *******************/
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
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机右移到中间位置");
    LeftRight_set_speed(400);

    while (sensor_Middle_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    LeftRight_set_speed(0);
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机到达中间限位");

    /************************   升降电机--上限位置    ***************************/
    UpDown_motor_enable(1); // 使能
    ESP_LOGI(TAG_UPDOWN, "升降电机开始向上运动");
    UpDown_motor_set_direction(1); // 向上
    UpDown_motor_start();

    while (sensor_Up_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGI(TAG_UPDOWN, "升降电机到达上限位，停止升降电机");
    UpDown_motor_stop();

    /***************   关闭上磁阀和上气泵电机    ******************/
    ESP_LOGI(TAG_PV, "关闭上气泵");
    Pump_Up_off();
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG_PV, "关闭上电磁阀");
    Valve_Up_off();
    vTaskDelay(pdMS_TO_TICKS(10));

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

    /****************   柱体电机--转45度    *******************/
    Rotate_motor_CALIBRATION(45.0f, 6.0f, false);

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
    ESP_LOGI(TAG_LEFTRIGHT, "左右电机到达最右边限位");

    /************************   升降电机--上限位置    ***************************/
    UpDown_motor_enable(1); // 使能
    ESP_LOGI(TAG_UPDOWN, "升降电机开始向上运动");
    UpDown_motor_set_direction(1); // 向上
    UpDown_motor_start();

    while (sensor_Up_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGI(TAG_UPDOWN, "升降电机到达上限位，停止升降电机");
    UpDown_motor_stop();

    /***************   关闭下磁阀和下气泵电机    ******************/
    ESP_LOGI(TAG_PV, "关闭下气泵");
    Pump_Down_off();
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG_PV, "关闭下电磁阀");
    Valve_Down_off();
    vTaskDelay(pdMS_TO_TICKS(10));

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
    UpDown_motor_enable(0); // 失能

    /************************   升降电机--再次下降位置    ***************************/
    UpDown_motor_enable(1); // 使能
    ESP_LOGI(TAG_UPDOWN, "升降电机开始向下运动");
    UpDown_motor_set_direction(0); // 向下
    UpDown_motor_start();
    vTaskDelay(pdMS_TO_TICKS(1000));
    UpDown_motor_stop();
    UpDown_motor_enable(0); // 失能


    /****************   柱体电机--转45度  空盒子    *******************/
    Rotate_motor_CALIBRATION(45.0f, 6.0f, false);
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
        while (check_and_pick_plate()) {
            // 可以在这里调用 Success()，或者执行别的逻辑
            Success();
        }
    }
        /*
         * 失败则重新来
         */
    else {
        ESP_LOGW(TAG_SYSTEM, "当前柱体无空培养皿了");
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

        // 创建旋转任务
        xTaskCreate(task_rotate, "RotateTask", 4096, NULL, 5, NULL);

        // 创建取盘任务
        xTaskCreate(task_pick, "PickTask", 4096, NULL, 5, NULL);
    }
}