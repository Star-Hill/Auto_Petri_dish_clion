#include "HMI.h"
#include "Instrument_starts_canning.h"
#include "HMI_control_driver.h"   // 硬件串口驱动
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
volatile bool g_pause_flag = false;   // 是否暂停

// 硬件串口接收任务
static void uart_receive_task(void *pvParameters) {
    ESP_LOGI(TAG, "串口初始化成功");
    char recv_str[128];

    ESP_LOGI(TAG, "UART Receive Task Started...");

    while (1) {
        int len = uart_hmi_read((uint8_t *) recv_str, sizeof(recv_str) - 1);
        if (len > 0) {
            recv_str[len] = '\0';
            ESP_LOGI(TAG, "收到字符串: %s", recv_str);

            // 检测命令并放入队列
            if (strstr(recv_str, "Return_to_zero") != NULL ||
                strstr(recv_str, "Start_canning") != NULL ||
                strstr(recv_str, "Liquid_line_purging") != NULL ||
                strstr(recv_str, "Pipeline_Cleaning") != NULL ||
                strstr(recv_str, "Software_reset") != NULL
                    ) {
                xQueueSend(command_queue, recv_str, portMAX_DELAY);
            }

            /***************   复位    ******************/
            if (strstr(recv_str, "Software_reset") != NULL) {
                Peristaltic_pump_Control(false);
                vTaskDelay(pdMS_TO_TICKS(100));
                SERVO_MOTOR_Stop();
                vTaskDelay(pdMS_TO_TICKS(100));
                esp_restart();  // 立即复位芯片
                ESP_LOGI(TAG, "机器强行复位");
            }
            if (strstr(recv_str, "stop") != NULL) {
                g_pause_flag = true;
                uart_hmi_send("page stop");
                ESP_LOGI(TAG, "立即暂停！");
                continue;   // 不进队列
            }
            if (strstr(recv_str, "go_on") != NULL) {
                g_pause_flag = false;
                uart_hmi_send("page go_on");
                ESP_LOGI(TAG, "立即继续！");
                continue;   // 不进队列
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// 命令执行任务 (保持不变)
static void command_execute_task(void *pvParameters) {
    char cmd_buf[CMD_MAX_LEN];

    while (1) {
        if (xQueueReceive(command_queue, cmd_buf, portMAX_DELAY)) {
            ESP_LOGI(TAG, "执行命令: %s", cmd_buf);

            /***************   回零    ******************/
            if (strstr(cmd_buf, "Return_to_zero") != NULL) {
                Machine_initialization();
                // 两个任务完成后，再切换页面
                uart_hmi_send("page dialog5");
                ESP_LOGI(TAG, "初始化成功，可以开始装配了！");
            }
            /***************   开始装配    ******************/
            if (strstr(cmd_buf, "Start_canning") != NULL) {
                ESP_LOGI(TAG, "开始装配！");
                int num = 0, volume = 0, rpm = 0;
                float gear = 0.0f;
                char *token;
                char *saveptr;

                token = strtok_r(cmd_buf, ",", &saveptr);

                // 数量
                token = strtok_r(NULL, ",", &saveptr);
                if (token) num = atoi(token);

                // 体积
                token = strtok_r(NULL, ",", &saveptr);
                if (token) volume = atoi(token);

                // 档位
                token = strtok_r(NULL, ",", &saveptr);
                if (token) gear = atof(token);

                // 蠕动泵速度
                token = strtok_r(NULL, ",", &saveptr);
                if (token) rpm = atoi(token);

                ESP_LOGI(TAG, "解析结果: 数量=%d, 体积=%d, 档位=%.1f, 蠕动泵速度=%d", num, volume, gear, rpm);
                Instrument_starts_canning(num, volume, gear, rpm);
            }
            /***************   液路排空    ******************/
            if (strstr(cmd_buf, "Liquid_line_purging") != NULL) {
                uart_set_baudrate(SERVO_MOTOR_UART_NUM, 9600);
                Peristaltic_pump_Run(200, 10);
                uart_set_baudrate(SERVO_MOTOR_UART_NUM, 57600);
                uart_hmi_send("page dialog6");
                ESP_LOGI(TAG, "液路排空完毕！");
            }
            /***************   管路清洗    ******************/
            if (strstr(cmd_buf, "Pipeline_Cleaning") != NULL) {
                uart_set_baudrate(SERVO_MOTOR_UART_NUM, 9600);
                Peristaltic_pump_Cleaning(1);
                uart_set_baudrate(SERVO_MOTOR_UART_NUM, 57600);
                uart_hmi_send("page dialog7");
                ESP_LOGI(TAG, "管路清洗完毕！");
            }
        }
    }
}

// 初始化函数
void command_handler_init(void) {
    command_queue = xQueueCreate(5, CMD_MAX_LEN);
    if (command_queue == NULL) {
        ESP_LOGE(TAG, "创建HMI收发队列失败！");
        return;
    }

    xTaskCreate(uart_receive_task, "UART Receive Task", 4096, NULL, 5, NULL);
    xTaskCreate(command_execute_task, "Command Execute Task", 4096, NULL, 5, NULL);
}
