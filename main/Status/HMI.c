//
// Created by DELL on 2025/8/12.
//

#include "HMI.h"
#include "Instrument_starts_canning.h"


/*
 *  该程序是工程的初始化程序
 * */

static const char *TAG = "SYSTEM";

// 队列句柄定义
QueueHandle_t command_queue = NULL;

/*
 * 建立 volatile 变量
 * 与volatile变量有关的运算，不要进行编译优化，以免出错
 * */
volatile bool g_pause_flag = false;   // 是否暂停

SoftwareUART g_uart;   // 全局串口变量
// 串口接收任务
static void uart_receive_task(void *pvParameters) {
    SoftwareUART uart = {
            .baud_rate = 115200,
            .rx_buff_size = 1024,
            .tx_pin = 5,
            .rx_pin = 6,
    };
    software_uart_init(&uart);

    char recv_str[128];

    ESP_LOGI(TAG, "UART Receive Task Started...");

    while (1) {
        int len = software_uart_rx_read(&uart, (uint8_t *) recv_str, sizeof(recv_str) - 1);
        if (len > 0) {
            recv_str[len] = '\0';
            ESP_LOGI(TAG, "收到字符串: %s", recv_str);
            // 检测命令并放入队列
            if (strstr(recv_str, "Return_to_zero") != NULL ||
                strstr(recv_str, "Start_canning") != NULL ||
                strstr(recv_str, "Liquid_line_purging") != NULL ||
                strstr(recv_str, "Pipeline_Cleaning") != NULL
                    ) {
                xQueueSend(command_queue, recv_str, portMAX_DELAY);
            }
            if (strstr(recv_str, "stop") != NULL) {
                g_pause_flag = true;
                ESP_LOGI(TAG, "立即暂停！");
                continue;   // 不进队列
            }
            if (strstr(recv_str, "go_on") != NULL) {
                g_pause_flag = false;
                ESP_LOGI(TAG, "立即继续！");
                continue;   // 不进队列
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// 命令执行任务
static void command_execute_task(void *pvParameters) {
    char cmd_buf[CMD_MAX_LEN];

    while (1) {
        if (xQueueReceive(command_queue, cmd_buf, portMAX_DELAY)) {
            ESP_LOGI(TAG, "执行命令: %s", cmd_buf);
            /***************   回零    ******************/
            if (strstr(cmd_buf, "Return_to_zero") != NULL) {
                /*
                 * 解析参数：Start_canning,数量,速度,档位
                * */
                float gear = 0.0f;
                char *token;
                char *saveptr;
                token = strtok_r(cmd_buf, ",", &saveptr);  // "Start_canning"

                // 解析档位
                token = strtok_r(NULL, ",", &saveptr);
                if (token) {
                    char *endptr;
                    errno = 0;
                    float fval = strtof(token, &endptr);
                    if (errno == 0 && *endptr == '\0') {
                        gear = fval;
                    } else {
                        ESP_LOGW(TAG, "档位参数非法: %s", token);
                    }
                }

                ESP_LOGI(TAG, "解析结果: 档位=%.1f", gear);


                Machine_initialization(gear);           //回零点
                ESP_LOGI(TAG, "初始化成功，可以开始装配了！");
            }
            /***************   开始装配    ******************/
            if (strstr(cmd_buf, "Start_canning") != NULL) {
                ESP_LOGI(TAG, "开始装配！");
                /*
                 * 解析参数：Start_canning,数量,体积,档位,蠕动泵的速度
                 * */
                int num = 0, volume = 0, rpm = 0;
                float gear = 0.0f;

                char *token;
                char *saveptr;

                token = strtok_r(cmd_buf, ",", &saveptr);  // "Start_canning"

                // 解析数量
                token = strtok_r(NULL, ",", &saveptr);
                if (token) {
                    char *endptr;
                    errno = 0;
                    long val = strtol(token, &endptr, 10);
                    if (errno == 0 && *endptr == '\0' && val >= 0 && val <= INT_MAX) {
                        num = (int) val;
                    } else {
                        ESP_LOGW(TAG, "数量参数非法: %s", token);
                    }
                }

                // 解析体积
                token = strtok_r(NULL, ",", &saveptr);
                if (token) {
                    char *endptr;
                    errno = 0;
                    long val = strtol(token, &endptr, 10);
                    if (errno == 0 && *endptr == '\0' && val >= 0 && val <= INT_MAX) {
                        volume = (int) val;
                    } else {
                        ESP_LOGW(TAG, "体积参数非法: %s", token);
                    }
                }

                // 解析档位
                token = strtok_r(NULL, ",", &saveptr);
                if (token) {
                    char *endptr;
                    errno = 0;
                    float fval = strtof(token, &endptr);
                    if (errno == 0 && *endptr == '\0') {
                        gear = fval;
                    } else {
                        ESP_LOGW(TAG, "档位参数非法: %s", token);
                    }
                }

                // 解析蠕动泵速度
                token = strtok_r(NULL, ",", &saveptr);
                if (token) {
                    char *endptr;
                    errno = 0;
                    long val = strtol(token, &endptr, 10);
                    if (errno == 0 && *endptr == '\0' && val >= 0 && val <= INT_MAX) {
                        rpm = (int) val;
                    } else {
                        ESP_LOGW(TAG, "蠕动泵速度参数非法: %s", token);
                    }
                }

                ESP_LOGI(TAG, "解析结果: 数量=%d, 体积=%d, 档位=%.1f, 蠕动泵速度=%d", num, volume, gear, rpm);

                Instrument_starts_canning(num, volume, gear, rpm);            //  开始装配

            }
            /***************   液路排空    ******************/
            if (strstr(cmd_buf, "Liquid_line_purging") != NULL) {
                Peristaltic_pump_Run(200, 10);      //速度 200rpm   时间10秒
                ESP_LOGI(TAG, "液路排空完毕！");
            }
            /***************   管路清洗    ******************/
            if (strstr(cmd_buf, "Pipeline_Cleaning") != NULL) {
                Peristaltic_pump_Cleaning(1);       //参数    1--全速   0--正常速度
                ESP_LOGI(TAG, "管路清洗完毕！");
            }
        }
    }
}


// 初始化函数
void command_handler_init(void) {
    // 创建队列
    command_queue = xQueueCreate(5, CMD_MAX_LEN);
    if (command_queue == NULL) {
        ESP_LOGE(TAG, "创建HMI收发队列失败！");
        return;
    }

    // 创建两个任务
    xTaskCreate(uart_receive_task, "UART Receive Task", 4096, NULL, 5, NULL);
    xTaskCreate(command_execute_task, "Command Execute Task", 4096, NULL, 5, NULL);
}
