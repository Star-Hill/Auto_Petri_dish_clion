#include "HMI.h"
#include "Instrument_starts_canning.h"
#include "HMI_control_driver.h" // 硬件串口驱动
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

static const char *TAG = "SYSTEM";

// 队列句柄定义
QueueHandle_t command_queue = NULL;

/*
 * 建立 volatile 变量
 */
volatile bool g_pause_flag = false; // 是否暂停

// 硬件串口接收任务
static void uart_receive_task(void *pvParameters)
{
    ESP_LOGI(TAG, "串口初始化成功");
    char recv_str[128];

    ESP_LOGI(TAG, "UART Receive Task Started...");

    while (1)
    {
        int len = uart_hmi_read((uint8_t *)recv_str, sizeof(recv_str) - 1);
        if (len > 0)
        {
            recv_str[len] = '\0';
            ESP_LOGI(TAG, "收到字符串: %s", recv_str);

            // 检测命令并放入队列
            if (strstr(recv_str, "Return_to_zero") != NULL ||
                strstr(recv_str, "Start_canning") != NULL ||
                strstr(recv_str, "Liquid_line_purging") != NULL ||
                strstr(recv_str, "Pipeline_Cleaning") != NULL ||
                strstr(recv_str, "Software_reset") != NULL)
            {
                xQueueSend(command_queue, recv_str, portMAX_DELAY);
            }

            /***************   复位    ******************/
            if (strstr(recv_str, "Software_reset") != NULL)
            {
                Peristaltic_pump_Control(true);
                Peristaltic_pump_Control(false);
                SERVO_MOTOR_Stop();
                esp_restart(); // 立即复位芯片
                ESP_LOGI(TAG, "机器强行复位");
            }
            if (strstr(recv_str, "stop") != NULL)
            {
                g_pause_flag = true;
                uart_hmi_send("page stop");
                ESP_LOGI(TAG, "立即暂停！");
                continue; // 不进队列
            }
            if (strstr(recv_str, "go_on") != NULL)
            {
                g_pause_flag = false;
                uart_hmi_send("page go_on");
                ESP_LOGI(TAG, "立即继续！");
                continue; // 不进队列
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// 命令执行任务 (保持不变)
static void command_execute_task(void *pvParameters)
{
    char cmd_buf[CMD_MAX_LEN];

    while (1)
    {
        if (xQueueReceive(command_queue, cmd_buf, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "执行命令: %s", cmd_buf);

            /***************   回零    ******************/
            if (strstr(cmd_buf, "Return_to_zero") != NULL)
            {
                uart_hmi_send("main.p1.aph=127");

                Machine_initialization(); // 回零函数

                uart_hmi_send("main.p1.aph=0");
                vTaskDelay(pdMS_TO_TICKS(100));
                uart_hmi_send("main.t5.txt=\"1\""); // 回零完成
                vTaskDelay(pdMS_TO_TICKS(100));
                uart_hmi_send("page dialog5"); // 两个任务完成后，再切换页面
                ESP_LOGI(TAG, "初始化成功，可以开始装配了！");
            }
            /***************   开始装配    ******************/
            if (strstr(cmd_buf, "Start_canning") != NULL)
            {
                ESP_LOGI(TAG, "开始装配！");
                int num = 0, volume = 0, rpm = 0;
                float gear = 0.0f;
                char *token;
                char *saveptr;

                // 第一次调用，用cmd_buf初始化strtok_r
                token = strtok_r(cmd_buf, ",", &saveptr); // 取出 "Start_canning"

                // 数量
                token = strtok_r(NULL, ",", &saveptr);
                if (token)
                    num = atoi(token);
                // 体积
                token = strtok_r(NULL, ",", &saveptr);
                if (token)
                    volume = atoi(token);
                // 档位
                token = strtok_r(NULL, ",", &saveptr);
                if (token)
                    gear = atof(token);

                // 蠕动泵速度
                token = strtok_r(NULL, ",", &saveptr);
                if (token)
                    rpm = atoi(token);

                ESP_LOGI(TAG, "解析结果: 数量=%d, 体积=%d, 档位=%.1f, 蠕动泵速度=%d", num, volume, gear, rpm);
                int volume2 = volume + 5; // 5就是补偿，比如输入20其实传入的参数是25，保证能装满20ml
                Instrument_starts_canning(num, volume2, gear, rpm);
            }
            /***************   液路排空    ******************/
            if (strstr(cmd_buf, "Liquid_line_purging") != NULL)
            {
                Peristaltic_pump_Run(200, 10);
                uart_hmi_send("page dialog6");
                ESP_LOGI(TAG, "液路排空完毕！");
            }
            /***************   管路清洗    ******************/
            if (strstr(cmd_buf, "Pipeline_Cleaning") != NULL)
            {
                Peristaltic_pump_Cleaning(1);
                uart_hmi_send("page dialog7");
                ESP_LOGI(TAG, "管路清洗完毕！");
            }
        }
    }
}

// 初始化函数
void command_handler_init(void)
{
    command_queue = xQueueCreate(5, CMD_MAX_LEN);
    if (command_queue == NULL)
    {
        ESP_LOGE(TAG, "创建HMI收发队列失败！");
        return;
    }

    xTaskCreate(uart_receive_task, "UART Receive Task", 4096, NULL, 5, NULL);
    xTaskCreate(command_execute_task, "Command Execute Task", 4096, NULL, 5, NULL);

    uart_hmi_send("main.t5.txt=\"0\""); // 回零重置
    uart_hmi_send("page dialog9");
}
