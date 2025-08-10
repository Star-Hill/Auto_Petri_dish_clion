//
// Created by DELL on 2025/8/8.
//

#include "Peristaltic_pump.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#define TAG "Peristaltic_pump_RS485"

// ====== CRC16 校验函数 计算（Modbus RTU，多项式 0xA001，低字节先发）
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

/********** RS485发送数据   ***************/
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

    uint8_t rx_buf[256];
    int rx_len = uart_read_bytes(Peristaltic_pump_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));
    if (rx_len > 0) {
        ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);
    } else {
        ESP_LOGW(TAG, "No response from device.");
    }

    // 协议要求间隔
    vTaskDelay(pdMS_TO_TICKS(50));
}

/**
 * 读取 Modbus 单个寄存器
 * @param reg_addr 寄存器地址
 * @param[out] value 读取到的值
 * @return true 成功，false 失败
 */
static bool Peristaltic_pump_modbus_read_single_register(uint16_t reg_addr, uint16_t *value) {
    uint8_t frame[8];
    frame[0] = Peristaltic_pump_SLAVE_ADDR;                      // 从站地址
    frame[1] = Peristaltic_pump_MODBUS_READ_SINGLE_REGISTER;     // 功能码 0x03
    frame[2] = reg_addr >> 8;                             // 寄存器高字节
    frame[3] = reg_addr & 0xFF;                           // 寄存器低字节
    frame[4] = 0x00;                                      // 读取数量高字节
    frame[5] = 0x01;                                      // 读取 1 个寄存器

    uint16_t crc = Peristaltic_pump_modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;  // CRC 低字节
    frame[7] = crc >> 8;    // CRC 高字节

    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
    Peristaltic_pump_rs485_send_bytes(frame, sizeof(frame));

    // 等待响应
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[256];
    int rx_len = uart_read_bytes(Peristaltic_pump_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));

    if (rx_len >= 7) { // 正常应答长度: 地址(1) + 功能码(1) + 字节数(1) + 数据(2) + CRC(2)
        ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);

        // CRC 校验
        uint16_t crc_calc = Peristaltic_pump_modbus_crc16(rx_buf, rx_len - 2);
        uint16_t crc_recv = rx_buf[rx_len - 2] | (rx_buf[rx_len - 1] << 8);
        if (crc_calc != crc_recv) {
            ESP_LOGE(TAG, "CRC error! Calc=0x%04X Recv=0x%04X", crc_calc, crc_recv);
            return false;
        }

        // 提取数据（高字节在前）
        *value = (rx_buf[3] << 8) | rx_buf[4];
        return true;
    } else {
        ESP_LOGW(TAG, "No valid response from device.");
        return false;
    }
}

void Peristaltic_pump_init(void) {
    uart_config_t uart_config = {
            .baud_rate = Peristaltic_pump_RS485_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_driver_install(Peristaltic_pump_UART_NUM, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(Peristaltic_pump_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(
            uart_set_pin(Peristaltic_pump_UART_NUM, Peristaltic_pump_485_TX, Peristaltic_pump_485_RX,
                         UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // 半双工模式
    ESP_ERROR_CHECK(uart_set_mode(Peristaltic_pump_UART_NUM, UART_MODE_RS485_HALF_DUPLEX));
    ESP_LOGI(TAG, "Peristaltic_pump UART initialized.\n");
}

void Peristaltic_pump_set_speed(int32_t rpm) {             //蠕动泵转速
    ESP_LOGI(TAG, "---   蠕动泵转速 reg 值  ---");
    int32_t reg_value = (rpm * 10);
    Peristaltic_pump_modbus_write_single_register(0x06, reg_value);
    ESP_LOGI(TAG, "当前蠕动泵的转速为: %ld \n", rpm);
}

// ====== 示例：方向状态寄存器 ======
void Peristaltic_pump_set_Reverse_Mode(uint16_t Mode) {            //逆时针1  顺时针0
    uint16_t decimal_addr = 3101;                  // 十进制地址
    uint16_t hex_addr = (uint16_t) decimal_addr;    // 实际存储是一样的，打印时显示 16 进制方便对照

    ESP_LOGI(TAG, "--- 蠕动泵方向模式: 寄存器(十进制:%u, 十六进制:0x%04X) ---", decimal_addr, hex_addr);

    Peristaltic_pump_modbus_write_single_register(hex_addr, Mode);
    if (Mode == 1) {
        ESP_LOGI(TAG, "当前蠕动泵方向: 逆时针 \n");
    } else if (Mode == 0) {
        ESP_LOGI(TAG, "当前蠕动泵方向: 顺时针 \n");
    } else {
        ESP_LOGW(TAG, "未知的方向值: %u \n", Mode);
    }
}

// ====== 示例：启动蠕动泵 ======
void Peristaltic_pump_Start(void) {
    uint16_t EN_addr = 3102;                  // 十进制地址
    uint16_t hex_addr = (uint16_t) EN_addr;
    ESP_LOGI(TAG, "--- 蠕动泵运行状态模式: 寄存器(十进制:%u, 十六进制:0x%04X) ---", EN_addr, hex_addr);

    ESP_LOGI(TAG, "--- 蠕动泵使能 reg 值---");
    Peristaltic_pump_modbus_write_single_register(hex_addr, 0X01);
    ESP_LOGI(TAG, "----- 蠕动泵使能 -----\n");
}

// ====== 示例：停止伺服电机 ======
void Peristaltic_pump_Stop(void) {
    uint16_t DIS_EN_addr = 3102;                  // 十进制地址
    uint16_t hex_addr = (uint16_t) DIS_EN_addr;
    ESP_LOGI(TAG, "--- 蠕动泵运行状态模式: 寄存器(十进制:%u, 十六进制:0x%04X) ---", DIS_EN_addr, hex_addr);

    ESP_LOGI(TAG, "--- 蠕动泵失能 reg 值 ---");
    Peristaltic_pump_modbus_write_single_register(hex_addr, 0X00);
    ESP_LOGI(TAG, "----- 蠕动泵失能 -----\n");
}


// ====== 示例：测试蠕动泵函数     蠕动泵正转反转 ======
void Peristaltic_pump_Test(void) {
    Peristaltic_pump_init();                        //蠕动泵初始化
    Peristaltic_pump_Start();                   //启动蠕动泵
    Peristaltic_pump_set_speed(300);       //蠕动泵速度
    vTaskDelay(pdMS_TO_TICKS(5000));
    Peristaltic_pump_Stop();                   //停止蠕动泵
}
