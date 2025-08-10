#include "LeftRight.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#define TAG "LeftRight_RS485"

// ====== CRC16 校验函数 计算（Modbus RTU，多项式 0xA001，低字节先发）
static uint16_t modbus_crc16(const uint8_t *buf, uint16_t len) {
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
static void LeftRight_rs485_send_bytes(const uint8_t *data, uint8_t length) {
    uart_flush_input(LeftRight_UART_NUM);
    if (uart_write_bytes(LeftRight_UART_NUM, (const char *) data, length) != length) {
        ESP_LOGE(TAG, "Send data failed.");
    }
    uart_wait_tx_done(LeftRight_UART_NUM, pdMS_TO_TICKS(50));
}

// 写单寄存器
static void LeftRight_modbus_write_single_register(uint16_t reg_addr, uint16_t value) {
    uint8_t frame[8];
    frame[0] = LeftRight_SLAVE_ADDR;
    frame[1] = LeftRight_MODBUS_WRITE_SINGLE_REGISTER;
    frame[2] = reg_addr >> 8;
    frame[3] = reg_addr & 0xFF;
    frame[4] = value >> 8;
    frame[5] = value & 0xFF;

    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;   // CRC低字节
    frame[7] = crc >> 8;     // CRC高字节

    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
    LeftRight_rs485_send_bytes(frame, sizeof(frame));

    // 延迟等待响应
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[256];
    int rx_len = uart_read_bytes(LeftRight_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));
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
static bool modbus_read_single_register(uint16_t reg_addr, uint16_t *value) {
    uint8_t frame[8];
    frame[0] = LeftRight_SLAVE_ADDR;                      // 从站地址
    frame[1] = LeftRight_MODBUS_READ_SINGLE_REGISTER;     // 功能码 0x03
    frame[2] = reg_addr >> 8;                             // 寄存器高字节
    frame[3] = reg_addr & 0xFF;                           // 寄存器低字节
    frame[4] = 0x00;                                      // 读取数量高字节
    frame[5] = 0x01;                                      // 读取 1 个寄存器

    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;  // CRC 低字节
    frame[7] = crc >> 8;    // CRC 高字节

    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
    LeftRight_rs485_send_bytes(frame, sizeof(frame));

    // 等待响应
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[256];
    int rx_len = uart_read_bytes(LeftRight_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));

    if (rx_len >= 7) { // 正常应答长度: 地址(1) + 功能码(1) + 字节数(1) + 数据(2) + CRC(2)
        ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);

        // CRC 校验
        uint16_t crc_calc = modbus_crc16(rx_buf, rx_len - 2);
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


void LeftRight_init(void) {
    uart_config_t uart_config = {
            .baud_rate = LeftRight_RS485_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_driver_install(LeftRight_UART_NUM, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(LeftRight_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(
            uart_set_pin(LeftRight_UART_NUM, LeftRight485_TX, LeftRight485_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // 半双工模式
    ESP_ERROR_CHECK(uart_set_mode(LeftRight_UART_NUM, UART_MODE_RS485_HALF_DUPLEX));
    ESP_LOGI(TAG, "LeftRight UART initialized.\n");
}


// ====== 示例：读取电压寄存器 ======
void LeftRight_read_Voltage() {
    uint16_t speed_value;
    ESP_LOGI(TAG, "---   伺服电机电压 reg 值  ---");
    if (modbus_read_single_register(0xE1, &speed_value)) {
        ESP_LOGI(TAG, "电压寄存器的值为: %u V \n", speed_value);
    }
}

// ====== 示例：读取电流寄存器 ======
void LeftRight_read_Electric_current() {
    uint16_t electric_current;
    ESP_LOGI(TAG, "---   伺服电机电流 reg 值  ---");
    if (modbus_read_single_register(0xE2, &electric_current)) {
        ESP_LOGI(TAG, "电流寄存器的值为: %.2f A \n", electric_current / 10.0f);
    }
}

// ====== 示例：写转速寄存器 ======
void LeftRight_set_speed(int32_t rpm) {                 //负左正右
    ESP_LOGI(TAG, "---   伺服电机转速 reg 值  ---");
    int32_t reg_value = (rpm * 8192) / 3000;            // 转速转 Modbus 数据
    LeftRight_modbus_write_single_register(0x06, reg_value);
    ESP_LOGI(TAG, "当前电机的转速为: %ld \n", rpm);
}

// ====== 示例：速度模式寄存器 ======
void LeftRight_set_speed_Mode(uint16_t Mode) {
    ESP_LOGI(TAG, "---   伺服电机模式: 速度 reg 值 ---");
    LeftRight_modbus_write_single_register(0x02, Mode);
    ESP_LOGI(TAG, "当前电机处于速度模式 \n");
}

// ====== 示例：速度模式寄存器 ======                         位置模式未启用
void LeftRight_set_Location_Mode(uint16_t Mode) {
    ESP_LOGI(TAG, "---   伺服电机模式: 位置 reg 值 ---");
    LeftRight_modbus_write_single_register(0x02, Mode);
    ESP_LOGI(TAG, "当前电机处于位置模式 \n");
}

// ====== 示例：启动伺服电机 ======
void LeftRight_Start(void) {
    ESP_LOGI(TAG, "--- 伺服电机开始工作 reg 值---");
    LeftRight_modbus_write_single_register(0x00, 0X01);
    ESP_LOGI(TAG, "----- 伺服电机开始工作 -----\n");
}

// ====== 示例：停止伺服电机 ======
void LeftRight_Stop(void) {
    ESP_LOGI(TAG, "--- 伺服电机停止工作 reg 值 ---");
    LeftRight_modbus_write_single_register(0x00, 0X00);
    ESP_LOGI(TAG, "----- 伺服电机停止工作 -----\n");
}

// ====== 示例：清除故障伺服电机 ======
void LeftRight_Clear_the_fault(void) {
    ESP_LOGI(TAG, "--- 伺服电机: 清除故障 reg 值 ---");
    LeftRight_modbus_write_single_register(0x4A, 00);
    ESP_LOGI(TAG, "----- 伺服电机清除故障 -----\n");
}


// ====== 示例：测试电机函数     伺服电机左右循环移动 ======
void LeftRight_Test(void) {
    LeftRight_init();                   //左右电机初始化
    LeftRight_read_Voltage();           //输出当前驱动器的电压值
    LeftRight_read_Electric_current();  //读取当前驱动器的电流值

    while (1) {
        LeftRight_Clear_the_fault();        //清除故障
        LeftRight_set_speed_Mode(0Xc4);//伺服电机速度模式
        LeftRight_Start();                  //启动伺服电机

        LeftRight_set_speed(-200);      //左移
        vTaskDelay(pdMS_TO_TICKS(2000));
        LeftRight_set_speed(0);
        vTaskDelay(pdMS_TO_TICKS(1000));

        LeftRight_set_speed(200);      //右移
        vTaskDelay(pdMS_TO_TICKS(2000));
        LeftRight_set_speed(0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        LeftRight_Stop();                   //停止伺服电机
        LeftRight_Clear_the_fault();        //清除故障
    }
}
