//
// Created by DELL on 2025/8/11.
//
#include "Instrument_starts_canning.h"

static const char *TAG_SYSTEM = "SYSTEM--Notice";

/********************************   检查取盘  START  ***********************************/
// 问题2修复：用信号量替代 eTaskGetState，避免对已删除任务句柄的非法访问
static SemaphoreHandle_t pickplate_sem = NULL;
static int plate_result = 0; // 0=未检测到, 1=检测到

// 电机任务
void MotorTask(void *pvParameters)
{
    float gear = *(float *)pvParameters;
    SERVO_MOTOR_POS_Reg((int)(1500 * gear), middle, 0, false);
    xSemaphoreGive(pickplate_sem);
    vTaskDelete(NULL);
}

// 培养皿检测任务
void PlateDetectTask(void *pvParameters)
{
    const int total_time_ms = 4000;
    const int check_interval_ms = 50;
    int elapsed_time = 0;

    while (elapsed_time < total_time_ms)
    {
        if (sensor_Entrance_get_state() == 0)
        {
            ESP_LOGI(TAG_SYSTEM, "成功检测到培养皿");
            plate_result = 1;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(check_interval_ms));
        elapsed_time += check_interval_ms;
    }
    if (plate_result == 0)
    {
        ESP_LOGW(TAG_SYSTEM, "未检测到培养皿!");
    }
    xSemaphoreGive(pickplate_sem);
    vTaskDelete(NULL);
}

/********************************   检查取盘   END    ***********************************/

/*****************************   蠕动泵 & 旋转  Start    *********************************/
// 问题1修复：用计数信号量替代 volatile int task_done_count，消除多核竞态
static SemaphoreHandle_t pump_rotate_sem = NULL;

// 25# 管号专用流量表 (mL/min)
float get_flow_rate_25(int rpm)
{
    switch (rpm)
    {
    case 30:  return 27;
    case 60:  return 55;
    case 100: return 92;
    case 200: return 184;
    case 400: return 370;
    case 600: return 550;
    default:  return 0;
    }
}

typedef struct
{
    int rpm;
    float time_s; // 预计算好的运行时间，避免 PPSS 全局变量竞态（问题B修复）
} PumpRunParam_t;

// 蠕动泵任务
void PeristalticPumpTask(void *pvParameters)
{
    PumpRunParam_t *param = (PumpRunParam_t *)pvParameters;
    if (!param)
    {
        ESP_LOGE(TAG_SYSTEM, "PumpTask 参数为空");
        xSemaphoreGive(pump_rotate_sem);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI("PumpTask", "收到参数: rpm=%d, time_s=%.2f", param->rpm, param->time_s);
    Peristaltic_pump_Run(param->rpm, param->time_s - 0.3f);
    ESP_LOGI(TAG_SYSTEM, "蠕动泵输出营养液 完毕");
    vPortFree(param);
    xSemaphoreGive(pump_rotate_sem);
    vTaskDelete(NULL);
}

// 任务2：控制小旋转电机，接收预计算好的 time_s
void LittleRotateMotorTask(void *pvParameters)
{
    // 问题B修复：time_s 由 Peristaltic_Steeping 预计算后堆分配传入，消除对 PPSS 全局变量的依赖
    float time_s = *(float *)pvParameters;
    vPortFree(pvParameters);
    float angle = 360.0f * ceilf(time_s / 3);
    Little_stepper_rotate_US(angle, 40.0f, 1); // 原来是20现在修改为40  2026-4-13 修改旋转速度
    ESP_LOGI(TAG_SYSTEM, "旋转培养皿 完毕");
    xSemaphoreGive(pump_rotate_sem);
    vTaskDelete(NULL);
}

// 调用    Peristaltic_Steeping  同步函数
void Peristaltic_Steeping(int volume, int rpm)
{
    // 问题B修复：在此预计算 time_s，再分别传给两个任务，不再依赖全局 PPSS + 100ms 延时
    float flow_mL_min = get_flow_rate_25(rpm);
    if (flow_mL_min <= 0)
    {
        ESP_LOGE(TAG_SYSTEM, "不支持的转速: %d rpm，跳过灌装", rpm);
        return;
    }
    float flow_mL_s = flow_mL_min / 60.0f;
    float time_s = (float)volume / flow_mL_s;
    ESP_LOGI(TAG_SYSTEM, "体积=%d mL, 转速=%d rpm, 流量=%.2f mL/min, 运行时间=%.2f s",
             volume, rpm, flow_mL_min, time_s);

    PumpRunParam_t *pump_param = pvPortMalloc(sizeof(PumpRunParam_t));
    float *rotate_time = pvPortMalloc(sizeof(float));
    if (!pump_param || !rotate_time)
    {
        ESP_LOGE(TAG_SYSTEM, "内存分配失败");
        vPortFree(pump_param);
        vPortFree(rotate_time);
        return;
    }
    pump_param->rpm = rpm;
    pump_param->time_s = time_s;
    *rotate_time = time_s;

    // 每次重建信号量，保证初始计数为0
    if (pump_rotate_sem != NULL)
    {
        vSemaphoreDelete(pump_rotate_sem);
    }
    pump_rotate_sem = xSemaphoreCreateCounting(2, 0);

    xTaskCreate(PeristalticPumpTask, "PumpTask", 4096, pump_param, 5, NULL);
    xTaskCreate(LittleRotateMotorTask, "RotateTask", 4096, rotate_time, 5, NULL);

    xSemaphoreTake(pump_rotate_sem, portMAX_DELAY);
    xSemaphoreTake(pump_rotate_sem, portMAX_DELAY);
}

/*****************************   蠕动泵 & 旋转  End    *********************************/

/*****************   柱体电机旋转 45 度 Start *******************/
// 问题3修复：BigRotateMotorTask 接收堆上 gear 副本，避免 Success() 返回后访问已释放栈
void BigRotateMotorTask(void *pvParameters)
{
    float gear = *(float *)pvParameters;
    vPortFree(pvParameters);
    Big_ROTATE_stepper_rotate_US(45.0f, 4.0f, 0, false);
    ESP_LOGI(TAG_SYSTEM, "柱体电机旋转 45 度 完毕  到达满柱体  当前挡位 %.2f", gear);
    vTaskDelete(NULL);
}
/*****************   柱体电机旋转 45 度 End *******************/

void All_init(void)
{
    sensor_ALL_init();
    buzzer_init();
    Big_ROTATE_stepper_init();
    Little_stepper_init();
    UpDown_stepper_init();
    // Peristaltic_Pump_init();
    SERVO_MOTOR_init();

    Pump_driver_init();
    valve_driver_init();

    uart_hmi_init();
    ESP_LOGI(TAG_SYSTEM, "所有设备初始化完成");
}

// 函数返回值：1 表示成功取到培养皿，0 表示失败
int check_and_pick_plate(float gear)
{
    Pump_Valve_run_combo(0, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    check_pause();

    UpDown_stepper_rotate(1220.0f, 200.0f * gear, 1, 0);
    check_pause();

    plate_result = 0;

    // 每次重建信号量，保证初始计数为0
    if (pickplate_sem != NULL)
    {
        vSemaphoreDelete(pickplate_sem);
    }
    pickplate_sem = xSemaphoreCreateCounting(2, 0);

    xTaskCreate(MotorTask, "MotorTask", 4096, &gear, 5, NULL);
    xTaskCreate(PlateDetectTask, "PlateDetectTask", 4096, NULL, 5, NULL);

    xSemaphoreTake(pickplate_sem, portMAX_DELAY);
    xSemaphoreTake(pickplate_sem, portMAX_DELAY);

    return plate_result;
}

/*
 * 成功后的操作
 * */
void Success(float gear, int volume, int rpm)
{
    const float Compensation = 100.0f; // 调整这里的补偿测试出开盖的合适距离，没碰到盖子就正数，挤压盖子就是负数

    vTaskDelay(pdMS_TO_TICKS(100));
    /***************   打开上磁阀和上气泵电机    ******************/
    Pump_Valve_run_combo(1, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    check_pause();

    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(680.0f + Compensation, 150.0f * gear, 1, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /************************   升降电机--吐液位置    ***************************/
    UpDown_stepper_rotate(700.0f + Compensation, 200.0f * gear, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    check_pause();

    /****************   左右电机--左位置    *******************/
    SERVO_MOTOR_POS_Reg((int)(1500 * gear), left, 0, false);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /****************   蠕动泵输出营养液  电机旋转  *******************/
    Peristaltic_Steeping(volume, rpm);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /****************   小旋转电机--左-->中位置   顺时针   *******************/
    Little_stepper_rotate_US(720.0f, 40.0f, 0); // 两圈     2026-4-13 修改旋转速度

    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /****************   左右电机--中位置    *******************/
    SERVO_MOTOR_POS_Reg((int)(1500 * gear), middle, 0, false);
    check_pause();

    /****************   柱体电机--转45度  罐装完毕的柱体    *******************/
    // 问题3修复：堆分配 gear 副本，防止 Success() 继续运行后任务访问已回收的栈内存
    float *gear_heap = pvPortMalloc(sizeof(float));
    *gear_heap = gear;
    xTaskCreate(BigRotateMotorTask, "BigRotateTask", 4096, gear_heap, 5, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG_SYSTEM, "移动到罐装完毕的柱体");

    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(700.0f + Compensation, 200.0f * gear, 1, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /***************   关闭上磁阀和上气泵电机    ******************/
    Pump_Valve_run_combo(1, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(1900.0f + Compensation, 200.0f * gear, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /****************   左右电机--右位置    *******************/
    SERVO_MOTOR_POS_Reg((int)(1500 * gear), right, 0, false);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /************************   升降电机--上限位置    ***************************/
    UpDown_stepper_rotate(1850.0f, 200.0f, 1, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /***************   关闭下磁阀和下气泵电机    ******************/
    Pump_Valve_run_combo(0, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /************************   升降电机--下限位置    ***************************/
    UpDown_stepper_rotate(1300.0f, 200.0f, 0, 0);
    UpDown_stepper_rotate(1000.0f, 40.0f, 0, 2);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();

    /****************   柱体电机--转-45度  空盒子    *******************/
    Big_ROTATE_stepper_rotate_US(45.0f, 4.0f, 1, false);
    vTaskDelay(pdMS_TO_TICKS(100));
    check_pause();
}

/*
 * 失败后的操作
 * */
static SemaphoreHandle_t failureSem = NULL;

void BigRotateTask(void *pvParameters)
{
    const float gear = *(float *)pvParameters;
    (void)gear;
    Big_ROTATE_stepper_rotate_US(90.0f, 4.0f, 1, false);
    xSemaphoreGive(failureSem);
    vTaskDelete(NULL);
}

void LeftRightMotorTask(void *pvParameters)
{
    float gear = *(float *)pvParameters;
    SERVO_MOTOR_POS_Reg((int)(1000 * gear), right, 0, false);
    xSemaphoreGive(failureSem);
    vTaskDelete(NULL);
}

void Failure(float gear)
{
    ESP_LOGW(TAG_SYSTEM, "当前柱体无空培养皿了");
    Pump_Valve_run_combo(0, 0);
    check_pause();

    UpDown_stepper_rotate(1220.0f, 200.0f * gear, 0, 0);
    check_pause();

    // 问题5修复：每次重建信号量，彻底消除上次任务残留计数导致的提前返回
    if (failureSem != NULL)
    {
        vSemaphoreDelete(failureSem);
    }
    failureSem = xSemaphoreCreateCounting(2, 0);

    xTaskCreate(BigRotateTask, "BigRotateTask", 4096, &gear, 5, NULL);
    xTaskCreate(LeftRightMotorTask, "LeftRightMotorTask", 4096, &gear, 5, NULL);

    xSemaphoreTake(failureSem, portMAX_DELAY);
    xSemaphoreTake(failureSem, portMAX_DELAY);

    ESP_LOGI(TAG_SYSTEM, "两个子任务完成，继续后续操作");
}

/*
 * 数量,体积,挡位
 * */
void Instrument_starts_canning(int num, int volume, float gear, int rpm)
{
    int produced_count = 0;
    char buf[64];

    for (int column = 0; column < 4; column++)
    {
        ESP_LOGI(TAG_SYSTEM, "开始检测第 %d 根柱子", column + 1);

        int result = check_and_pick_plate(gear);
        if (result == 1)
        {
            Success(gear, volume, rpm);
            produced_count++;
            ESP_LOGI(TAG_SYSTEM, "第 %d 根柱子检测成功", column + 1);
            ESP_LOGI(TAG_SYSTEM, "累计完成数量: %d / %d", produced_count, num);

            if (produced_count == num)
            {
                ESP_LOGI(TAG_SYSTEM, "已完成目标数量 %d，停止装配", num);
                sprintf(buf, "finish.t3.txt=\"%d\"", produced_count);
                uart_hmi_send(buf);
                vTaskDelay(pdMS_TO_TICKS(100));
                uart_hmi_send("main.t4.txt=\"0\"");
                uart_hmi_send("page finish");
                buzzer_beep_music();
                return;
            }
            column = -1; // 因为for循环结束会 column++，所以这里设 -1
            continue;
        }
        else
        {
            Failure(gear);
            ESP_LOGW(TAG_SYSTEM, "第 %d 根柱子检测失败", column + 1);
        }
        ESP_LOGI(TAG_SYSTEM, "第 %d 根柱子检测结束", column + 1);
    }
    ESP_LOGI(TAG_SYSTEM, "全部柱子检测完毕 (总完成数量: %d / %d)", produced_count, num);

    buzzer_beep_music();

    sprintf(buf, "finish.t3.txt=\"%d\"", produced_count);
    uart_hmi_send(buf);
    vTaskDelay(pdMS_TO_TICKS(100));
    uart_hmi_send("main.t4.txt=\"0\"");
    uart_hmi_send("page finish");
}

void check_pause(void)
{
    ESP_LOGI("CANNING", "check_pause: 当前 g_pause_flag=%d", g_pause_flag);
    while (g_pause_flag)
    {
        ESP_LOGI("CANNING", "暂停中，等待继续... g_pause_flag=%d", g_pause_flag);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    ESP_LOGI("CANNING", "继续运行, g_pause_flag=%d", g_pause_flag);
}
