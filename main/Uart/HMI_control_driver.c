//
// Created by DELL on 2025/8/7.
//

#include "HMI_control_driver.h"
#include "Pump_driver.h"
#include "Valve_driver.h"
#include "UpDown_motor_control.h"

#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "UART_HMI";

void uart_hmi_init(void) {
    const uart_config_t uart_config = {
            .baud_rate = 115200,  // 串口屏波特率
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_HMI, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_HMI, &uart_config);
    uart_set_pin(UART_HMI, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "串口屏初始化完成");
}

void uart_hmi_send(const char *data) {
    uart_write_bytes(UART_HMI, data, strlen(data));
    // 串口屏大多需要 \xff\xff\xff 作为结束标志
    const char end[3] = {0xff, 0xff, 0xff};
    uart_write_bytes(UART_HMI, end, 3);
}

int uart_hmi_read(uint8_t *data, size_t length) {
    return uart_read_bytes(UART_HMI, data, length, pdMS_TO_TICKS(100));
}

void HMI_control_driver_test(void) {
    uart_hmi_init();
    Pump_driver_init();
    valve_UpDown_driver_init();

    uint8_t data[32];

    while (1) {
        int len = uart_hmi_read(data, sizeof(data));
        if (len > 0) {
            ESP_LOGI(TAG, "收到数据：%.*s", len, data);

            if (strstr((char*)data, "UpDown_motor_auto_loop")) {
                UpDown_motor_auto_loop();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // 每100ms检测一次
    }
}

