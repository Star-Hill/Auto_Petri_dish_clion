//
// Created by DELL on 2025/8/11.
//

#include "Instrument_starts_canning.h"

static const char *TAG_SYSTEM = "SYSTEM--Notice";

int volatile task_done_count = 0;                   //蠕动泵和旋转并行任务检测

typedef struct {   //蠕动泵的结构体定义
    int rpm;       // 转速
    int volume;  // 目标体积 (mL)
} PumpParam_t;

// 25# 管号专用流量表 (mL/min)
float get_flow_rate_25(int rpm) {
    switch (rpm) {
        case 30:  return 27;
        case 60:  return 55;
        case 100: return 92;
        case 200: return 184;
        case 400: return 370;
        case 600: return 550;
        default:  return 0; // 不支持的转速
    }
}

void PeristalticPumpTask(void *pvParameters) {
    PumpParam_t *param = (PumpParam_t *)pvParameters;
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

    // 转换成 mL/s
    float flow_mL_s = flow_mL_min / 60.0f;

    // 计算运行时间
    float time_s = param->volume / flow_mL_s;


    ESP_LOGI(TAG_SYSTEM,
             "目标体积=%.2d mL, 转速=%d rpm, 25#管, 流量=%.2f mL/min, 运行时间=%.2f s",
             param->volume, param->rpm, flow_mL_min, time_s);

    // 控制泵
    Peristaltic_pump_Control(1);
    Peristaltic_pump_set_speed(param->rpm);
    vTaskDelay(pdMS_TO_TICKS((int)(time_s * 1000)));
    Peristaltic_pump_Control(0);

    task_done_count++;

    ESP_LOGI(TAG_SYSTEM, "蠕动泵输出营养液 完毕");

    vPortFree(param);
    vTaskDelete(NULL);
}

// 任务2：控制小旋转电机      两圈
void LittleRotateMotorTask(void *pvParameters) {
    Little_stepper_rotate(720.0f, 40.0f, 1);
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
    Peristaltic_pump_init();                //  蠕动泵
    Pump_driver_init();                     //  泵初始化
    valve_driver_init();                    //  电磁阀初始化
}

// 函数返回值：1 表示成功取到培养皿，0 表示失败
int check_and_pick_plate(float gear) {
    /****************   升降电机--吸取位置    *******************/
    UpDown_stepper_rotate(800.0f, 50.0f * gear, 1, 0);
    check_pause();

    /***************   打开下磁阀和气泵电机    ******************/
    Pump_Valve_run_combo(0, 1);
    check_pause();

    /****************   左右电机--取出后移动中位置    *******************/
    LeftRight_Clear_the_fault();             // 清除故障
    LeftRight_set_speed_Mode(0xC4);    // 设置伺服电机速度模式
    LeftRight_Start();                       // 启动左右伺服电机
    // 取出后左移
    LeftRight_set_speed(-400);           // 左移
    /*
     * 培养皿检测
     * */
    // 检测是否取到培养皿
    TickType_t startTick = xTaskGetTickCount();  // 在这里定义一次
    /*
     * 如果入口传感器精准的话      这个检测时间是可以实现动态的
     * */
    const TickType_t timeoutTicks = pdMS_TO_TICKS(4 * 1000); // 4秒

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
void Success(float gear,int volume,int rpm) {
    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(600.0f, 50.0f * gear, 1, 1);      //上移动检测上传感器
    check_pause();

    /***************   打开上磁阀和上气泵电机    ******************/
    Pump_Valve_run_combo(1, 1);
    check_pause();

    /************************   升降电机--中限位置    ***************************/
    UpDown_stepper_rotate(500.0f, 50.0f * gear, 0, 0);      //上下电机开盖位置
    check_pause();      //400是刚好

    /****************   左右电机--左位置    *******************/
    LeftRight_Move_To_Position(sensor_Left_get_state, -400 * gear, "最左位置");
    check_pause();

    /****************   蠕动泵输出营养液  电机旋转  *******************/
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
    check_pause();
    task_done_count = 0;

    /****************   小旋转电机--左-->中位置   逆时针   *******************/
    Little_stepper_rotate(1080.0f, 120.0f, 0);              //三圈
    check_pause();
    ESP_LOGI(TAG_SYSTEM, "旋转培养皿开始逆时针旋转");

    /****************   左右电机--中位置    *******************/
    LeftRight_Move_To_Position(sensor_Middle_get_state, 400 * gear, "中间位置");
    check_pause();

    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(600.0f, 50.0f * gear, 1, 1);      //上移动检测上传感器
    check_pause();

    /***************   关闭上磁阀和上气泵电机    ******************/
    Pump_Valve_run_combo(1, 0);
    check_pause();

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(1290.0f, 50.0f * gear, 0, 0);
    check_pause();

    /****************   柱体电机--转45度  罐装完毕的柱体    *******************/
    Big_ROTATE_stepper_rotate(45.0f, 120.0f * gear, 1, 0);
    check_pause();
    ESP_LOGI(TAG_SYSTEM, "移动到罐装完毕的柱体");

    /****************   左右电机--右位置    *******************/
    LeftRight_Move_To_Position(sensor_Right_get_state, 400 * gear, "最右位置");
    check_pause();

    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(2000.0f, 50.0f * gear, 1, 1);      //上移动检测上传感器
    check_pause();

    /***************   关闭下磁阀和下气泵电机    ******************/
    Pump_Valve_run_combo(0, 0);
    check_pause();

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(1290.0f, 50.0f * gear, 0, 0);        //到达最低
    check_pause();

    /****************   柱体电机--转-45度  空盒子    *******************/
    Big_ROTATE_stepper_rotate(45.0f, 120.0f * gear, 0, 0);
    check_pause();

}

/*
 * 失败后的操作
 * */
int column_Failure_Num = 1;

void Failure(float gear) {
    ESP_LOGW(TAG_SYSTEM, "当前柱体无空培养皿了");

    /***************   关闭下磁阀和气泵电机    ******************/
    Pump_Valve_run_combo(0, 0);
    check_pause();

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(800.0f, 50.0f * gear, 0, 0);     //dir = 0 向下移动
    check_pause();

    /****************   柱体电机--转90度位置    *******************/
    if (column_Failure_Num == 2) {
        Big_ROTATE_stepper_rotate(90.0f, 120.0f * gear, 1, 0);
        column_Failure_Num = 1;
    } else {
        column_Failure_Num++;
    }
    check_pause();

    /****************   左右电机--右位置    *******************/
    LeftRight_Move_To_Position(sensor_Right_get_state, 400 * gear, "最右位置");
    check_pause();

}

/*
 * 数量,体积,挡位
 * */
void Instrument_starts_canning(int num, int volume, float gear, int rpm) {
    int produced_count = 0;   // 已经制作的数量

    for (int column = 0; column < 4; column++) {  // 4个柱子
        int failCount_single_column = 0;  // 单个柱子的失败计数

        ESP_LOGI(TAG_SYSTEM, "开始检测第 %d 根柱子", column + 1);

        for (int attempt = 0; attempt < 2; attempt++) { // 每根柱子检测2次
            int result = check_and_pick_plate(gear);
            if (result == 1) { // 成功
                Success(gear,volume,rpm);
                failCount_single_column = 0; // 成功就清零
                produced_count++;   // 成功制作一个培养皿
                ESP_LOGI(TAG_SYSTEM, "第 %d 根柱子第 %d 次检测成功", column + 1, attempt + 1);
                ESP_LOGI(TAG_SYSTEM, "累计完成数量: %d / %d", produced_count, num);

                if (produced_count >= num) {
                    ESP_LOGI(TAG_SYSTEM, "已完成目标数量 %d，停止装配", num);
                    return; // 直接退出函数
                }
                break; // 成功一次就不再检测该柱子，直接下一根
            }
            if (result == 0) { // 失败
                Failure(gear);
                failCount_single_column++;
                ESP_LOGW(TAG_SYSTEM, "第 %d 根柱子第 %d 次检测失败", column + 1, attempt + 1);
            }
        }

        if (failCount_single_column == 2) { // 两次都失败
            ESP_LOGW(TAG_SYSTEM, "第 %d 根柱子连续两次检测失败", column + 1);
        }
        ESP_LOGI(TAG_SYSTEM, "第 %d 根柱子检测结束", column + 1);
    }
    ESP_LOGI(TAG_SYSTEM, "全部柱子检测完毕 (总完成数量: %d / %d)", produced_count, num);
}

void check_pause(void) {
    ESP_LOGI("CANNING", "check_pause: 当前 g_pause_flag=%d", g_pause_flag);
    while (g_pause_flag) {
        ESP_LOGI("CANNING", "暂停中，等待继续... g_pause_flag=%d", g_pause_flag);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    ESP_LOGI("CANNING", "继续运行, g_pause_flag=%d", g_pause_flag);
}
