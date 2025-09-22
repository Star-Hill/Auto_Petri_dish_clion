//
// Created by DELL on 2025/8/8.
//

#include "Peristaltic_pump.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "Servo_motor_RS485_Speed_Location.h"
#include <string.h>

#define TAG "Peristaltic_pump_RS485"

// ⚠️ 使用伺服电机已经初始化好的 UART 端口号
#define Peristaltic_pump_UART_NUM 1

// ====== CRC16 校验函数 计算（Modbus RTU，多项式 0xA001，低字节先发）======
static uint16_t Peristaltic_pump_modbus_crc16(const uint8_t *buf, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++) {
        crc ^= (uint16_t) buf[pos];
        for (int i = 0; i < 8; i++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/********** RS485 发送数据 ***************/
static void Peristaltic_pump_rs485_send_bytes(const uint8_t *data, uint8_t length) {
    uart_flush_input(Peristaltic_pump_UART_NUM);
    if (uart_write_bytes(Peristaltic_pump_UART_NUM, (const char *) data, length) != length) {
        ESP_LOGE(TAG, "Send data failed.");
    }
    uart_wait_tx_done(Peristaltic_pump_UART_NUM, pdMS_TO_TICKS(50));
}

// 写单寄存器
static void Peristaltic_pump_modbus_write_single_register(uint16_t reg_addr, uint16_t value) {
    uint8_t frame[8];
    frame[0] = Peristaltic_pump_SLAVE_ADDR;
    frame[1] = Peristaltic_pump_MODBUS_WRITE_SINGLE_REGISTER;
    frame[2] = reg_addr >> 8;
    frame[3] = reg_addr & 0xFF;
    frame[4] = value >> 8;
    frame[5] = value & 0xFF;

    uint16_t crc = Peristaltic_pump_modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;   // CRC低字节
    frame[7] = crc >> 8;     // CRC高字节

    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
    Peristaltic_pump_rs485_send_bytes(frame, sizeof(frame));

    // 延迟等待响应
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[64];
    int rx_len = uart_read_bytes(Peristaltic_pump_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));
    if (rx_len > 0) {
        ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);
    } else {
        ESP_LOGW(TAG, "No response from device.");
    }

    vTaskDelay(pdMS_TO_TICKS(50));
}


/* ===================== API 实现 ===================== */

void Peristaltic_pump_set_speed(int32_t rpm) {
    uint16_t reg_addr = 3100;
    int32_t reg_value = rpm * 10;

    ESP_LOGI(TAG, "--- 设置蠕动泵转速: %ld rpm (寄存器值=%ld) ---", rpm, reg_value);
    Peristaltic_pump_modbus_write_single_register(reg_addr, reg_value);
}

void Peristaltic_pump_set_Reverse_Mode(uint16_t Mode) {
    uint16_t reg_addr = 3101;
    Peristaltic_pump_modbus_write_single_register(reg_addr, Mode);
    ESP_LOGI(TAG, "当前蠕动泵方向: %s", (Mode == 1) ? "逆时针" : "顺时针");
}

void Peristaltic_pump_Control(bool enable) {
    uint16_t reg_addr = 3102;
    Peristaltic_pump_modbus_write_single_register(reg_addr, enable ? 0x01 : 0x00);
    ESP_LOGI(TAG, "蠕动泵 %s", enable ? "启动" : "停止");
}

void Peristaltic_pump_Cleaning(bool Cleaning_Speed) {
    uint16_t reg_addr = 3103;

    // 启动蠕动泵
    Peristaltic_pump_Control(true);

    // 设置清洗速度（全速/正常）
    Peristaltic_pump_modbus_write_single_register(reg_addr, Cleaning_Speed ? 0x01 : 0x00);
    ESP_LOGI(TAG, "蠕动泵清洗模式: %s", Cleaning_Speed ? "全速" : "正常速度");

    // 来回三次
    for (int i = 0; i < 3; i++) {
        // 顺时针
        Peristaltic_pump_set_Reverse_Mode(0);  // 0=顺时针
        ESP_LOGI(TAG, "清洗循环 %d: 顺时针 3 秒", i + 1);
        vTaskDelay(pdMS_TO_TICKS(3000));

        vTaskDelay(pdMS_TO_TICKS(100)); // 缓冲 100ms

        // 逆时针
        Peristaltic_pump_set_Reverse_Mode(1);  // 1=逆时针
        ESP_LOGI(TAG, "清洗循环 %d: 逆时针 3 秒", i + 1);
        vTaskDelay(pdMS_TO_TICKS(3000));

        vTaskDelay(pdMS_TO_TICKS(100)); // 缓冲 100ms
    }

    // 停止蠕动泵
    Peristaltic_pump_Control(false);
    ESP_LOGI(TAG, "蠕动泵清洗完成");
}


void Peristaltic_pump_Run(int32_t speed, float run_time_s) {
    Peristaltic_pump_Control(true);
    Peristaltic_pump_set_Reverse_Mode(0);  // 0=顺时针
    Peristaltic_pump_set_speed(speed);
    vTaskDelay(pdMS_TO_TICKS(run_time_s * 1000));
    Peristaltic_pump_Control(false);
}
