//
// Created by DELL on 2025/8/11.
//

#include "Instrument_starts_canning.h"

static const char *TAG_SYSTEM = "SYSTEM--Notice";

static TaskHandle_t plateDetectTaskHandle = NULL;   //检测盘子任务句柄
static TaskHandle_t motorTaskHandle = NULL;         // 电机任务句柄
static int plate_result = 0;  // 0=未检测到, 1=检测到
/********************************   检查取盘  START  ***********************************/
// 电机任务
void MotorTask(void *pvParameters) {
    float gear = *(float *) pvParameters;
    SERVO_MOTOR_Move_Position_Speed((int) (1000 * gear), -310000, 0, sensor_Middle_get_state);
    SERVO_MOTOR_Move_Position_Speed((int) (1000 * gear), -297300, 0, NULL);
    vTaskDelete(NULL); // 任务完成后自删
}

// 培养皿检测任务
void PlateDetectTask(void *pvParameters) {
    const int total_time_ms = 4000; // 运动总时长约4秒
    const int check_interval_ms = 50;
    int elapsed_time = 0;

    while (elapsed_time < total_time_ms) {
        if (sensor_Entrance_get_state() == 0) { // 检测到培养皿
            ESP_LOGI(TAG_SYSTEM, "成功检测到培养皿");
            plate_result = 1;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(check_interval_ms));
        elapsed_time += check_interval_ms;
    }
    if (plate_result == 0) {
        ESP_LOGW(TAG_SYSTEM, "未检测到培养皿!");
    }
    vTaskDelete(NULL); // 自删
}
/********************************   检查取盘   END    ***********************************/

/*****************************   蠕动泵 & 旋转  Start    *********************************/
int volatile task_done_count = 0;                   //蠕动泵和旋转并行任务检测
typedef struct {   //蠕动泵的结构体定义
    int rpm;       // 转速
    int volume;  // 目标体积 (mL)
} PumpParam_t;

// 25# 管号专用流量表 (mL/min)
float get_flow_rate_25(int rpm) {
    switch (rpm) {
        case 30:
            return 27;
        case 60:
            return 55;
        case 100:
            return 92;
        case 200:
            return 184;
        case 400:
            return 370;
        case 600:
            return 550;
        default:
            return 0; // 不支持的转速
    }
}

float PPSS = 0.0f;

//蠕动泵任务
void PeristalticPumpTask(void *pvParameters) {
    uart_set_baudrate(SERVO_MOTOR_UART_NUM, 9600);
    PumpParam_t *param = (PumpParam_t *) pvParameters;
    if (!param) {   // 防止 NULL 崩溃
        ESP_LOGE(TAG_SYSTEM, "PumpTask 参数为空");
        vTaskDelete(NULL);
        return;
    }
    //可以安全使用
    ESP_LOGI("PumpTask", "收到参数: volume=%d, rpm=%d", param->volume, param->rpm);
    // 查表获取流量
    float flow_mL_min = get_flow_rate_25(param->rpm);
    if (flow_mL_min <= 0) {
        ESP_LOGE(TAG_SYSTEM, "不支持的转速: %d rpm", param->rpm);
        vPortFree(param);
        vTaskDelete(NULL);
        return;
    }
    float flow_mL_s = flow_mL_min / 60.0f;      // 转换成 mL/s
    float time_s = (float) param->volume / flow_mL_s;   // 计算运行时间
    PPSS = time_s;
    ESP_LOGI(TAG_SYSTEM,
             "目标体积=%.2d mL, 转速=%d rpm, 25#管, 流量=%.2f mL/min, 运行时间=%.2f s",
             param->volume, param->rpm, flow_mL_min, time_s);

    // 控制泵
    Peristaltic_pump_Run(param->rpm, time_s - 0.3f);

    task_done_count++;
    ESP_LOGI(TAG_SYSTEM, "蠕动泵输出营养液 完毕");

    vPortFree(param);
    vTaskDelete(NULL);
}

// 任务2：控制小旋转电机      两圈
void LittleRotateMotorTask(void *pvParameters) {
    float angle = 360.0f * ceilf(PPSS / 3);   // 电机要转的角度
    Little_stepper_rotate(angle, 20.0f, 1);
    task_done_count++;
    ESP_LOGI(TAG_SYSTEM, "旋转培养皿 完毕");
    vTaskDelete(NULL);
}

//调用    Peristaltic_Steeping  同步函数
void Peristaltic_Steeping(int volume, int rpm) {
    PumpParam_t *param = pvPortMalloc(sizeof(PumpParam_t));
    if (!param) {
        ESP_LOGE(TAG_SYSTEM, "内存分配失败");
        return;
    }
    param->volume = volume;
    param->rpm = rpm;

    xTaskCreate(PeristalticPumpTask, "PumpTask", 4096, param, 5, NULL);
    xTaskCreate(LittleRotateMotorTask, "MotorTask", 4096, NULL, 5, NULL);

    // 等待两个任务完成
    while (task_done_count < 2) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    task_done_count = 0;
}
/*****************************   蠕动泵 & 旋转  End    *********************************/

/*****************   柱体电机旋转 45 度 Start *******************/
void BigRotateMotorTask(void *pvParameters) {
    float gear = *(float *) pvParameters;
    Big_ROTATE_stepper_rotate(45.0f, 2.0f * gear, 0, false, 500);
    ESP_LOGI(TAG_SYSTEM, "柱体电机旋转 45 度 完毕  到达满柱体  当前挡位 %.2f", gear);
    vTaskDelete(NULL);  // 执行完毕自动删除
}

/*****************   柱体电机旋转 45 度 End *******************/
void All_init(void) {
    sensor_ALL_init();                      //  初始化所有传感器
    Big_ROTATE_stepper_init();              //  初始化并使能柱体旋转电机
    Little_stepper_init();                  //  培养皿旋转电机
    UpDown_stepper_init();                  //  初始化并使能升降电机
    SERVO_MOTOR_init();                     //  左右电机初始化
    //  蠕动泵  和  伺服电机服用
    Pump_driver_init();                     //  泵初始化
    valve_driver_init();                    //  电磁阀初始化
    uart_hmi_init();                        // 初始化硬件串口
    ESP_LOGI(TAG_SYSTEM, "所有设备初始化完成");
}

// 函数返回值：1 表示成功取到培养皿，0 表示失败
int check_and_pick_plate(float gear) {
    UpDown_stepper_rotate(1220.0f, 50.0f * gear, 1, 0);
    check_pause();

    Pump_Valve_run_combo(0, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    check_pause();

    plate_result = 0; // 清零结果

    // 创建电机任务 & 检测任务
    xTaskCreate(MotorTask, "MotorTask", 4096, &gear, 5, &motorTaskHandle);
    xTaskCreate(PlateDetectTask, "PlateDetectTask", 4096, NULL, 5, &plateDetectTaskHandle);

    // 等待两个任务都结束
    while (eTaskGetState(motorTaskHandle) != eDeleted ||
           eTaskGetState(plateDetectTaskHandle) != eDeleted) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return plate_result;
}

/*
 * 成功后的操作
 * */
void Success(float gear, int volume, int rpm) {
    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(630.0f, 50.0f * gear, 1, 0);      //上盖位置
    check_pause();

    /***************   打开上磁阀和上气泵电机    ******************/
    Pump_Valve_run_combo(1, 1);
    check_pause();

    /************************   升降电机--吐液位置    ***************************/
    UpDown_stepper_rotate(650.0f, 50.0f * gear, 0, 0);      //开盖
    check_pause();

    /****************   柱体电机--转45度  罐装完毕的柱体    *******************/
    xTaskCreate(BigRotateMotorTask, "BigRotateTask", 4096, &gear, 5, NULL);
    ESP_LOGI(TAG_SYSTEM, "移动到罐装完毕的柱体");

    /****************   左右电机--左位置    *******************/
    SERVO_MOTOR_Move_Position_Speed((int) (1000 * gear), -561000, 0, sensor_Left_get_state);
    check_pause();

    /****************   蠕动泵输出营养液  电机旋转  *******************/
    Peristaltic_Steeping(volume, rpm);
    uart_set_baudrate(SERVO_MOTOR_UART_NUM, 57600);
    check_pause();

    /****************   小旋转电机--左-->中位置   逆时针   *******************/
    Little_stepper_rotate(720.0f, 30.0f, 0);              //两圈
    check_pause();

    /****************   左右电机--中位置    *******************/
    SERVO_MOTOR_Move_Position_Speed((int) (1000 * gear), -297300, 0, sensor_Middle_get_state);
    check_pause();

    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(650.0f, 50.0f * gear, 1, 0);
    check_pause();

    /***************   关闭上磁阀和上气泵电机    ******************/
    Pump_Valve_run_combo(1, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    check_pause();

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(1850.0f, 50.0f * gear, 0, 0);
    check_pause();

    /****************   左右电机--右位置    *******************/
    SERVO_MOTOR_Move_Position_Speed((int) (1000 * gear), 0, 0, sensor_Right_get_state);
    SERVO_MOTOR_Move_Position_Speed((int) (1000), -4500, 0, NULL);
    check_pause();

    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(1850.0f, 50.0f * gear, 1, 1);
    check_pause();

    /***************   关闭下磁阀和下气泵电机    ******************/
    Pump_Valve_run_combo(0, 0);
    check_pause();

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(1850.0f, 50.0f * gear, 0, 0);
    check_pause();

    /****************   柱体电机--转-45度  空盒子    *******************/
    Big_ROTATE_stepper_rotate(45.0f, 2.0f * gear, 1, false, 500);
    check_pause();

}

/*
 * 失败后的操作
 * */
static SemaphoreHandle_t failureSem = NULL;

void BigRotateTask(void *pvParameters) {
    float gear = *(float *) pvParameters;
    Big_ROTATE_stepper_rotate(90.0f, 2.0f * gear, 1, false, 500);

    xSemaphoreGive(failureSem);
    vTaskDelete(NULL);
}

void LeftRightMotorTask(void *pvParameters) {
    float gear = *(float *) pvParameters;
    SERVO_MOTOR_Move_Position_Speed((int)(1000 * gear), 0, 0, sensor_Right_get_state);
    SERVO_MOTOR_Move_Position_Speed((int)(1000 * gear), -5850, 0, NULL);

    xSemaphoreGive(failureSem);
    vTaskDelete(NULL);
}

void Failure(float gear) {
    ESP_LOGW(TAG_SYSTEM, "当前柱体无空培养皿了");
    Pump_Valve_run_combo(0, 0);
    check_pause();

    UpDown_stepper_rotate(1220.0f, 50.0f * gear, 0, 0);
    check_pause();

    // 创建信号量（最大计数2）
    if (failureSem == NULL) {
        failureSem = xSemaphoreCreateCounting(2, 0);
    } else {
        xSemaphoreTake(failureSem, 0); // 清空
        xSemaphoreTake(failureSem, 0);
    }

    xTaskCreate(BigRotateTask, "BigRotateTask", 4096, &gear, 5, NULL);
    xTaskCreate(LeftRightMotorTask, "LeftRightMotorTask", 4096, &gear, 5, NULL);

    // 等待两个任务都完成
    xSemaphoreTake(failureSem, portMAX_DELAY);
    xSemaphoreTake(failureSem, portMAX_DELAY);

    ESP_LOGI(TAG_SYSTEM, "两个子任务完成，继续后续操作");
}



/*
 * 数量,体积,挡位
 * */
void Instrument_starts_canning(int num, int volume, float gear, int rpm) {
    int produced_count = 0;   // 已经制作的数量
    char buf[64];             // 发送缓冲区

    for (int column = 0; column < 4; column++) {  // 4个柱子
        ESP_LOGI(TAG_SYSTEM, "开始检测第 %d 根柱子", column + 1);

        int result = check_and_pick_plate(gear);
        if (result == 1) { // 成功
            Success(gear, volume, rpm);
            produced_count++;   // 成功制作一个培养皿
            ESP_LOGI(TAG_SYSTEM, "第 %d 根柱子检测成功", column + 1);
            ESP_LOGI(TAG_SYSTEM, "累计完成数量: %d / %d", produced_count, num);

            if (produced_count == num) {
                ESP_LOGI(TAG_SYSTEM, "已完成目标数量 %d，停止装配", num);
                sprintf(buf, "finish.t3.txt=\"%d\"", produced_count);
                uart_hmi_send(buf);
                vTaskDelay(pdMS_TO_TICKS(100));
                uart_hmi_send("page 15");
                return; // 直接退出函数
            }
            // 成功后重新从第0根柱子开始
            column = -1;            // 因为for循环结束会 column++，所以这里设 -1
            continue;
        } else { // 失败
            Failure(gear);
            ESP_LOGW(TAG_SYSTEM, "第 %d 根柱子检测失败", column + 1);
        }
        ESP_LOGI(TAG_SYSTEM, "第 %d 根柱子检测结束", column + 1);
    }
    ESP_LOGI(TAG_SYSTEM, "全部柱子检测完毕 (总完成数量: %d / %d)", produced_count, num);

    sprintf(buf, "finish.t3.txt=\"%d\"", produced_count);
    uart_hmi_send(buf);
    vTaskDelay(pdMS_TO_TICKS(100));
    uart_hmi_send("page 15");
}

void check_pause(void) {
    ESP_LOGI("CANNING", "check_pause: 当前 g_pause_flag=%d", g_pause_flag);
    while (g_pause_flag) {
        ESP_LOGI("CANNING", "暂停中，等待继续... g_pause_flag=%d", g_pause_flag);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    ESP_LOGI("CANNING", "继续运行, g_pause_flag=%d", g_pause_flag);
}
