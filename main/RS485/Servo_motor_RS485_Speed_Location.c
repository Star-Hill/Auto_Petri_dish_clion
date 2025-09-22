//
// Created by DELL on 2025/9/10.
//

#include "Servo_motor_RS485_Speed_Location.h"
#include "sensor.h"

#define TAG "SERVO_MOTOR_RS485"

// ====== CRC16 校验函数 (Modbus RTU，多项式 0xA001，低字节先发)
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

/********** RS485发送数据 **************/
static void SERVO_MOTOR_rs485_send_bytes(const uint8_t *data, uint8_t length) {
    uart_flush_input(SERVO_MOTOR_UART_NUM);
    if (uart_write_bytes(SERVO_MOTOR_UART_NUM, (const char *) data, length) != length) {
        ESP_LOGE(TAG, "Send data failed.");
    }
    uart_wait_tx_done(SERVO_MOTOR_UART_NUM, pdMS_TO_TICKS(50));
}

/********** 写单寄存器 **********/
void SERVO_MOTOR_modbus_write_single_register(uint16_t reg_addr, uint16_t value) {
    uint8_t frame[8];
    frame[0] = SERVO_MOTOR_SLAVE_ADDR;
    frame[1] = SERVO_MOTOR_MODBUS_WRITE_SINGLE_REGISTER;
    frame[2] = reg_addr >> 8;
    frame[3] = reg_addr & 0xFF;
    frame[4] = value >> 8;
    frame[5] = value & 0xFF;

    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;

    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
    SERVO_MOTOR_rs485_send_bytes(frame, sizeof(frame));

    vTaskDelay(pdMS_TO_TICKS(20));
    uint8_t rx_buf[256];
    int rx_len = uart_read_bytes(SERVO_MOTOR_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));
    if (rx_len > 0) ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);
    else
        ESP_LOGW(TAG, "No response from device.");
    vTaskDelay(pdMS_TO_TICKS(50));
}

/********** 读单寄存器 **********/
bool modbus_read_single_register(uint16_t reg_addr, uint16_t *value) {
    uint8_t frame[8];
    frame[0] = SERVO_MOTOR_SLAVE_ADDR;
    frame[1] = SERVO_MOTOR_MODBUS_READ_SINGLE_REGISTER;
    frame[2] = reg_addr >> 8;
    frame[3] = reg_addr & 0xFF;
    frame[4] = 0x00;
    frame[5] = 0x01;

    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;

    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
    SERVO_MOTOR_rs485_send_bytes(frame, sizeof(frame));

    // 等待响应
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t rx_buf[256];
    int rx_len = uart_read_bytes(SERVO_MOTOR_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));

    if (rx_len >= 7) {
        ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);
        uint16_t crc_calc = modbus_crc16(rx_buf, rx_len - 2);
        uint16_t crc_recv = rx_buf[rx_len - 2] | (rx_buf[rx_len - 1] << 8);
        if (crc_calc != crc_recv) {
            ESP_LOGE(TAG, "CRC error! Calc=0x%04X Recv=0x%04X", crc_calc, crc_recv);
            return false;
        }
        *value = (rx_buf[3] << 8) | rx_buf[4];
        return true;
    } else {
        ESP_LOGW(TAG, "No valid response from device.");
        return false;
    }
}

/********** 写多个寄存器 (32位) **********/
void SERVO_MOTOR_modbus_write_multi_register(uint16_t reg_addr, uint32_t value) {
    uint8_t frame[13];
    frame[0] = SERVO_MOTOR_SLAVE_ADDR;
    frame[1] = SERVO_MOTOR_MODBUS_WRITE_MULTIPLE_REGISTER;
    frame[2] = reg_addr >> 8;
    frame[3] = reg_addr & 0xFF;
    frame[4] = 0x00;
    frame[5] = 0x02;
    frame[6] = 0x04;
    frame[7] = (value >> 24) & 0xFF;
    frame[8] = (value >> 16) & 0xFF;
    frame[9] = (value >> 8) & 0xFF;
    frame[10] = value & 0xFF;

    uint16_t crc = modbus_crc16(frame, 11);
    frame[11] = crc & 0xFF;
    frame[12] = crc >> 8;

    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
    SERVO_MOTOR_rs485_send_bytes(frame, sizeof(frame));

    vTaskDelay(pdMS_TO_TICKS(20));
    uint8_t rx_buf[256];
    int rx_len = uart_read_bytes(SERVO_MOTOR_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));
    if (rx_len > 0) ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);
    else
        ESP_LOGW(TAG, "No response from device.");
    vTaskDelay(pdMS_TO_TICKS(50));
}

/********** 读 32位寄存器  **********/
int32_t SERVO_MOTOR_modbus_read_position(uint16_t reg_high, uint16_t reg_low) {
    uint8_t frame[8];
    frame[0] = SERVO_MOTOR_SLAVE_ADDR;
    frame[1] = SERVO_MOTOR_MODBUS_READ_SINGLE_REGISTER;
    frame[2] = reg_high >> 8;
    frame[3] = reg_high & 0xFF;
    frame[4] = 0x00;
    frame[5] = 0x02;   // 一共读 2 个寄存器（高位和低位）

    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;

    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
    SERVO_MOTOR_rs485_send_bytes(frame, sizeof(frame));

    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t rx_buf[256];
    int rx_len = uart_read_bytes(SERVO_MOTOR_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(200));

    if (rx_len < 9) {
        ESP_LOGW(TAG, "No valid response");
        return 0;
    }

    ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);

    // 按 Modbus 协议解析：
    // rx_buf[0] = 从站地址
    // rx_buf[1] = 功能码 (0x03)
    // rx_buf[2] = 字节数 (应该是 4)
    // rx_buf[3..6] = 高 16 位 + 低 16 位
    uint16_t high = (rx_buf[3] << 8) | rx_buf[4];
    uint16_t low = (rx_buf[5] << 8) | rx_buf[6];

    int32_t value = ((int32_t) high << 16) | low;

    ESP_LOGI("POSITION", "Decoded position = %ld", value);
    return value;
}

/********** UART初始化 **********/
void SERVO_MOTOR_init(void) {
    static bool uart_initialized = false;
    if (uart_initialized) return;

    uart_config_t uart_config = {
            .baud_rate = SERVO_MOTOR_RS485_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_driver_install(SERVO_MOTOR_UART_NUM, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(SERVO_MOTOR_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(SERVO_MOTOR_UART_NUM, LeftRight485_TX, LeftRight485_RX,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(SERVO_MOTOR_UART_NUM, UART_MODE_RS485_HALF_DUPLEX));
    ESP_LOGI(TAG, "LeftRight UART initialized.");
    uart_initialized = true;
    ESP_LOGI(TAG, "伺服电机初始化完成");
}

/********** 通用功能 **********/
void SERVO_MOTOR_read_Voltage() {
    uint16_t V;
    ESP_LOGI(TAG, "---   伺服电机电压 ---");
    if (modbus_read_single_register(0xE1, &V))
        ESP_LOGI(TAG, "电压: %u V", V);
}

void SERVO_MOTOR_read_Electric() {
    uint16_t A;
    ESP_LOGI(TAG, "---   伺服电机电流 ---");
    if (modbus_read_single_register(0xE2, &A))
        ESP_LOGI(TAG, "电流: %.2f A", A / 10.0f);
}

void SERVO_MOTOR_Clear_the_fault(void) {
    ESP_LOGI(TAG, "--- 清除伺服故障 ---");
    SERVO_MOTOR_modbus_write_single_register(0x4A, 0x00);
}

void SERVO_MOTOR_Start(void) {
    ESP_LOGI(TAG, "--- 伺服电机启动 ---");
    SERVO_MOTOR_modbus_write_single_register(0x00, 0x01);
}

void SERVO_MOTOR_Stop(void) {
    ESP_LOGI(TAG, "--- 伺服电机停止 ---");
    SERVO_MOTOR_modbus_write_single_register(0x00, 0x00);
}

/********** 速度模式 **********/
void SERVO_MOTOR_Set_Speed_Mode(void) {
    ESP_LOGI(TAG, "--- 设置速度模式 ---");
    SERVO_MOTOR_modbus_write_single_register(0x02, 0xC4);  // 假设速度模式值
}

void SERVO_MOTOR_Set_Speed(int32_t speed_value) {
    ESP_LOGI(TAG, "--- 设置速度 ---");
    int32_t reg_value = (speed_value * 8192) / 3000;            // 转速转 Modbus 数据
    SERVO_MOTOR_modbus_write_single_register(0x06, reg_value);
}

/********** 位置清零 **********/
void SERVO_MOTOR_Clear_Position(void) {
    ESP_LOGI(TAG, "--- 位置清零 ---");
    // 根据手册：0x4C 地址，值 = 0
    SERVO_MOTOR_modbus_write_single_register(0x4C, 0x00);
}

static const char *TAG_SYSTEM = "SYSTEM--Notice";

void SERVO_MOTOR_Move_To_Position(SensorFunc sensor_get_state, float speed, const char *desc) {
    SERVO_MOTOR_Clear_the_fault();
    SERVO_MOTOR_Set_Speed_Mode();
    SERVO_MOTOR_Start();
    // 先检测是否已经到位
    if (sensor_get_state() == 0) {
        ESP_LOGI(TAG_SYSTEM, "伺服电机已在%s，无需移动", desc);
        return;
    }
    ESP_LOGI(TAG_SYSTEM, "伺服电机移动到%s", desc);
    SERVO_MOTOR_Set_Speed(speed);
    // 一直检测，直到传感器触发
    while (sensor_get_state() != 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    // 停止电机
    SERVO_MOTOR_Set_Speed(0);
    ESP_LOGI(TAG_SYSTEM, "伺服电机到达%s", desc);
    SERVO_MOTOR_Stop();
}


/********** 位置模式 **********/
void SERVO_MOTOR_set_Location_Mode(int relative) {
    ESP_LOGI(TAG, "--- 设置位置模式 ---");
    SERVO_MOTOR_modbus_write_single_register(0x02, 0xD0);
    if (relative == 0) {
        SERVO_MOTOR_modbus_write_single_register(0x51, 0x00);  // 绝对位置
        ESP_LOGI(TAG, ">>> 已设置为绝对位置模式");
    } else {
        SERVO_MOTOR_modbus_write_single_register(0x51, 0x01);  // 相对位置
        ESP_LOGI(TAG, ">>> 已设置为相对位置模式");
    }
}

void SERVO_MOTOR_Set_Position_Speed(uint16_t target_rpm) {
    const uint16_t MAX_RPM = 3000;   // 手册标定最大速度
    const uint16_t MAX_REG = 8192;   // 对应最大速度寄存器值
    if (target_rpm > MAX_RPM) {
        target_rpm = MAX_RPM;  // 限幅
    }
    // 线性换算：寄存器值 = target_rpm / MAX_RPM * MAX_REG
    uint16_t reg_value = ((uint32_t) target_rpm * MAX_REG) / MAX_RPM;
    ESP_LOGI(TAG, "设置位置模式最高速度: %d RPM -> 寄存器值: %d", target_rpm, reg_value);
    // 写入寄存器0x1D
    SERVO_MOTOR_modbus_write_single_register(0x1D, reg_value);
}

/********** 专门读取位置反馈 **********/
int32_t SERVO_MOTOR_Read_Feedback_Position(void) {
    return SERVO_MOTOR_modbus_read_position(0xE8, 0xE9);
}

/********** 专门读取位置给定 **********/
int32_t SERVO_MOTOR_Read_Command_Position(void) {
    return SERVO_MOTOR_modbus_read_position(0xE6, 0xE7);
}

void SERVO_MOTOR_Move_Position_Speed(int speed_rpm, int position, int mode, SensorFunc sensor_func) {
    // 清除故障
    SERVO_MOTOR_Clear_the_fault();
    // 启动电机
    SERVO_MOTOR_Start();
    // 设置位置模式的最高速度
    SERVO_MOTOR_Set_Position_Speed(speed_rpm);
    // 设置为位置模式
    SERVO_MOTOR_set_Location_Mode(mode);        //0--绝对  1--相对

    // ====== 先检测一次传感器 ======
    if (sensor_func != NULL && sensor_func() == 0) {
        ESP_LOGW("SERVO_MOVE", "启动前传感器已触发，直接停止电机");
        SERVO_MOTOR_Stop();
        return;
    }

    //发送目标位置
    ESP_LOGI("SERVO_MOVE", "Move to position=%d, speed=%d RPM", position, speed_rpm);
    SERVO_MOTOR_modbus_write_multi_register(0x50, position);

    // ====== 发完位置后继续检测传感器 ======
    if (sensor_func != NULL) {
        while (1) {
            if (sensor_func() == 0) {
                ESP_LOGI("SERVO_MOVE", "传感器触发，停止电机");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10)); // 10ms 检测一次
        }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    // 停止电机（根据需求选择是否立即停）
    SERVO_MOTOR_Stop();
}