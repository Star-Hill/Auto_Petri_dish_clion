//
// Created by DELL on 2025/9/10.
//

#include "Servo_motor_RS485_Speed_Location.h"
#include "sensor.h"

// ====== CRC16 校验函数 (Modbus RTU，多项式 0xA001，低字节先发)
static uint16_t modbus_crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++)
    {
        crc ^= (uint16_t)buf[pos];
        for (int i = 0; i < 8; i++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/********** RS485发送数据 **************/
void SERVO_MOTOR_rs485_send_bytes(const uint8_t *data, uint8_t length)
{
    uart_flush_input(SERVO_MOTOR_UART_NUM);
    if (uart_write_bytes(SERVO_MOTOR_UART_NUM, (const char *)data, length) != length)
    {
        ESP_LOGE(SERVO_TAG, "Send data failed.");
    }
    uart_wait_tx_done(SERVO_MOTOR_UART_NUM, pdMS_TO_TICKS(MODBUS_DELAY_MS));
}

/********** 写单寄存器（带重试） **********/
void SERVO_MOTOR_modbus_write_single_register(uint16_t reg_addr, uint16_t value)
{
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

#ifdef SERVO_MOTOR_DEBUG
    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
#endif

    for (int attempt = 0; attempt < MODBUS_MAX_RETRY; attempt++)
    {
        SERVO_MOTOR_rs485_send_bytes(frame, sizeof(frame));
        vTaskDelay(pdMS_TO_TICKS(MODBUS_DELAY_MS));

        uint8_t rx_buf[256];
        int rx_len = uart_read_bytes(SERVO_MOTOR_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(MODBUS_DELAY_MS));

        if (rx_len >= 8)
        {
#ifdef SERVO_MOTOR_DEBUG
            ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);
#endif
            uint16_t crc_calc = modbus_crc16(rx_buf, rx_len - 2);
            uint16_t crc_recv = rx_buf[rx_len - 2] | (rx_buf[rx_len - 1] << 8);

            if (crc_calc == crc_recv)
            {
                return; // 通信成功
            }
            else
            {
                ESP_LOGW(SERVO_TAG, "CRC error (try %d/%d)", attempt + 1, MODBUS_MAX_RETRY);
            }
        }
        else
        {
            ESP_LOGW(SERVO_TAG, "No response (try %d/%d)", attempt + 1, MODBUS_MAX_RETRY);
        }
        vTaskDelay(pdMS_TO_TICKS(MODBUS_DELAY_MS));
    }
    ESP_LOGE(SERVO_TAG, "Write single register failed after %d retries.", MODBUS_MAX_RETRY);
}

/********** 读单寄存器（带重试） **********/
bool modbus_read_single_register(uint16_t reg_addr, uint16_t *value)
{
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

#ifdef SERVO_MOTOR_DEBUG
    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
#endif

    for (int attempt = 0; attempt < MODBUS_MAX_RETRY; attempt++)
    {
        SERVO_MOTOR_rs485_send_bytes(frame, sizeof(frame));
        vTaskDelay(pdMS_TO_TICKS(MODBUS_DELAY_MS));

        uint8_t rx_buf[256];
        int rx_len = uart_read_bytes(SERVO_MOTOR_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(MODBUS_DELAY_MS));

        if (rx_len >= 7)
        {
#ifdef SERVO_MOTOR_DEBUG
            ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);
#endif
            uint16_t crc_calc = modbus_crc16(rx_buf, rx_len - 2);
            uint16_t crc_recv = rx_buf[rx_len - 2] | (rx_buf[rx_len - 1] << 8);
            if (crc_calc == crc_recv)
            {
                *value = (rx_buf[3] << 8) | rx_buf[4];
                return true;
            }
            else
            {
                ESP_LOGW(SERVO_TAG, "CRC error (try %d/%d)", attempt + 1, MODBUS_MAX_RETRY);
            }
        }
        else
        {
            ESP_LOGW(SERVO_TAG, "No valid response (try %d/%d)", attempt + 1, MODBUS_MAX_RETRY);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGE(SERVO_TAG, "Read register 0x%04X failed after %d retries", reg_addr, MODBUS_MAX_RETRY);
    return false;
}

/********** 写多个寄存器 (32位，带重试) **********/
void SERVO_MOTOR_modbus_write_multi_register(uint16_t reg_addr, uint32_t value)
{
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

#ifdef SERVO_MOTOR_DEBUG
    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
#endif

    for (int attempt = 0; attempt < MODBUS_MAX_RETRY; attempt++)
    {
        SERVO_MOTOR_rs485_send_bytes(frame, sizeof(frame));
        vTaskDelay(pdMS_TO_TICKS(MODBUS_DELAY_MS));

        uint8_t rx_buf[256];
        int rx_len = uart_read_bytes(SERVO_MOTOR_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(MODBUS_DELAY_MS));

        if (rx_len >= 8)
        {
#ifdef SERVO_MOTOR_DEBUG
            ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);
#endif
            uint16_t crc_calc = modbus_crc16(rx_buf, rx_len - 2);
            uint16_t crc_recv = rx_buf[rx_len - 2] | (rx_buf[rx_len - 1] << 8);
            if (crc_calc == crc_recv)
                return; // 如果成功发出则直接返回并打印发出的数据 否则就重新发出数据并使用日志打印出重新发的次数
            ESP_LOGW(SERVO_TAG, "CRC error (try %d/%d)", attempt + 1, MODBUS_MAX_RETRY);
        }
        else
        {
            ESP_LOGW(SERVO_TAG, "No response (try %d/%d)", attempt + 1, MODBUS_MAX_RETRY);
        }
        vTaskDelay(pdMS_TO_TICKS(MODBUS_DELAY_MS)); // 这里给延时，可以用宏定义进行延迟的修改
    }
    ESP_LOGE(SERVO_TAG, "Write multi-register failed after %d retries.", MODBUS_MAX_RETRY);
}

/********** 读32位寄存器（带重试） **********/
int32_t SERVO_MOTOR_modbus_read_position(uint16_t reg_high, uint16_t reg_low)
{
    uint8_t frame[8];
    frame[0] = SERVO_MOTOR_SLAVE_ADDR;
    frame[1] = SERVO_MOTOR_MODBUS_READ_SINGLE_REGISTER;
    frame[2] = reg_high >> 8;
    frame[3] = reg_high & 0xFF;
    frame[4] = 0x00;
    frame[5] = 0x02;

    uint16_t crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;

#ifdef SERVO_MOTOR_DEBUG
    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
#endif

    for (int attempt = 0; attempt < MODBUS_MAX_RETRY; attempt++)
    {
        SERVO_MOTOR_rs485_send_bytes(frame, sizeof(frame));
        vTaskDelay(pdMS_TO_TICKS(MODBUS_DELAY_MS));

        uint8_t rx_buf[256];
        int rx_len = uart_read_bytes(SERVO_MOTOR_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(200));
        if (rx_len >= 9)
        {
            uint16_t crc_calc = modbus_crc16(rx_buf, rx_len - 2);
            uint16_t crc_recv = rx_buf[rx_len - 2] | (rx_buf[rx_len - 1] << 8);
            if (crc_calc == crc_recv)
            {
                uint16_t high = (rx_buf[3] << 8) | rx_buf[4];
                uint16_t low = (rx_buf[5] << 8) | rx_buf[6];
                return ((int32_t)high << 16) | low;
            }
            ESP_LOGW(SERVO_TAG, "CRC error (try %d/%d)", attempt + 1, MODBUS_MAX_RETRY);
        }
        else
        {
            ESP_LOGW(SERVO_TAG, "Invalid response (try %d/%d)", attempt + 1, MODBUS_MAX_RETRY);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    ESP_LOGE(SERVO_TAG, "Read position failed after %d retries.", MODBUS_MAX_RETRY);
    return 0;
}

/********** UART初始化 **********/
void SERVO_MOTOR_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = SERVO_MOTOR_RS485_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB};

    ESP_ERROR_CHECK(uart_driver_install(SERVO_MOTOR_UART_NUM, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(SERVO_MOTOR_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(SERVO_MOTOR_UART_NUM, LeftRight485_TX, LeftRight485_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(SERVO_TAG, "LeftRight UART initialized.");
    ESP_LOGI(SERVO_TAG, "伺服电机 UART %d 初始化完成", SERVO_MOTOR_UART_NUM);
}

void SERVO_MOTOR_deinit(void)
{
    ESP_ERROR_CHECK(uart_driver_delete(SERVO_MOTOR_UART_NUM));
    ESP_LOGI(SERVO_TAG, "伺服电机 UART %d 已卸载释放。", SERVO_MOTOR_UART_NUM);
}
/********** 通用功能 **********/
void SERVO_MOTOR_read_pos()
{
    uint16_t pos;
#ifdef SERVO_MOTOR_DEBUG
    ESP_LOGI(SERVO_TAG, "---   伺服电机位置 ---");
#endif
    if (modbus_read_single_register(Positioning_completed_reg, &pos))
        ESP_LOGI(SERVO_TAG, "位置 0 到位 1 不到位: %u", pos);
}

void SERVO_MOTOR_read_Voltage()
{
    uint16_t V;
#ifdef SERVO_MOTOR_DEBUG
    ESP_LOGI(SERVO_TAG, "---   伺服电机电压 ---");
#endif
    if (modbus_read_single_register(Read_voltage_reg, &V))
        ESP_LOGI(SERVO_TAG, "电压: %u V", V);
}

void SERVO_MOTOR_read_Electric()
{
    uint16_t A;
#ifdef SERVO_MOTOR_DEBUG
    ESP_LOGI(SERVO_TAG, "---   伺服电机电流 ---");
#endif

    if (modbus_read_single_register(Read_current_reg, &A))
        ESP_LOGI(SERVO_TAG, "电流: %.2f A", A / 10.0f);
}

void SERVO_MOTOR_Clear_the_fault(void)
{
#ifdef SERVO_MOTOR_DEBUG
    ESP_LOGI(SERVO_TAG, "--- 清除伺服故障 ---");
#endif
    SERVO_MOTOR_modbus_write_single_register(Clear_fault_reg, 0x00);
}

void SERVO_MOTOR_Start(void)
{
#ifdef SERVO_MOTOR_DEBUG
    ESP_LOGI(SERVO_TAG, "--- 伺服电机启动 ---");
#endif
    SERVO_MOTOR_modbus_write_single_register(Motor_start_stop_reg, 0x01);
}

void SERVO_MOTOR_Stop(void)
{
#ifdef SERVO_MOTOR_DEBUG
    ESP_LOGI(SERVO_TAG, "--- 伺服电机停止 ---");
#endif
    SERVO_MOTOR_modbus_write_single_register(Motor_start_stop_reg, 0x00);
}

/********** 速度模式 **********/
void SERVO_MOTOR_Set_Speed_Mode(void)
{
#ifdef SERVO_MOTOR_DEBUG
    ESP_LOGI(SERVO_TAG, "--- 设置速度模式 ---");
#endif
    SERVO_MOTOR_modbus_write_single_register(Motor_Mode_reg, 0xC4); // 假设速度模式值
}

void SERVO_MOTOR_Set_Speed(int32_t speed_value)
{
#ifdef SERVO_MOTOR_DEBUG
    ESP_LOGI(SERVO_TAG, "--- 设置速度 ---");
#endif
    int32_t reg_value = (speed_value * 8192) / 3000; // 转速转 Modbus 数据
    SERVO_MOTOR_modbus_write_single_register(Speed_command_reg, reg_value);
}

/********** 位置清零 **********/
void SERVO_MOTOR_Clear_Position(void)
{
#ifdef SERVO_MOTOR_DEBUG
    ESP_LOGI(SERVO_TAG, "--- 位置清零 ---");
#endif
    // 根据手册：0x4C 地址，值 = 0
    SERVO_MOTOR_modbus_write_single_register(Reset_position_reg, 0x00);
}

static const char *TAG_SYSTEM = "SYSTEM--Notice";

void SERVO_MOTOR_Move_To_Position(SensorFunc sensor_get_state, float speed, const char *desc)
{
    SERVO_MOTOR_Clear_the_fault();
    SERVO_MOTOR_Set_Speed_Mode();
    SERVO_MOTOR_Start();
    // 先检测是否已经到位
    if (sensor_get_state() == 0)
    {
        ESP_LOGI(TAG_SYSTEM, "伺服电机已在%s，无需移动", desc);
        return;
    }
    ESP_LOGI(TAG_SYSTEM, "伺服电机移动到%s", desc);
    SERVO_MOTOR_Set_Speed(speed);
    // 一直检测，直到传感器触发
    while (sensor_get_state() != 0)
    {
        vTaskDelay(pdMS_TO_TICKS(MODBUS_DELAY_MS));
    }
    // 停止电机
    SERVO_MOTOR_Set_Speed(0);
    ESP_LOGI(TAG_SYSTEM, "伺服电机到达%s", desc);
    SERVO_MOTOR_Stop();
}

/********** 位置模式 **********/
void SERVO_MOTOR_set_Location_Mode(int relative)
{
#ifdef SERVO_MOTOR_DEBUG
    ESP_LOGI(SERVO_TAG, "--- 设置位置模式 ---");
#endif
    SERVO_MOTOR_modbus_write_single_register(Motor_Mode_reg, 0xD0); // 位置模式
    if (relative == 0)
    {
        SERVO_MOTOR_modbus_write_single_register(Position_Mode_Selection_reg, 0x00); // 绝对位置
        ESP_LOGI(SERVO_TAG, ">>> 已设置为绝对位置模式");
    }
    else
    {
        SERVO_MOTOR_modbus_write_single_register(Position_Mode_Selection_reg, 0x01); // 相对位置
        ESP_LOGI(SERVO_TAG, ">>> 已设置为相对位置模式");
    }
}

void SERVO_MOTOR_Set_Position_Speed(uint16_t target_rpm)
{
    const uint16_t MAX_RPM = 3000; // 手册标定最大速度
    const uint16_t MAX_REG = 8192; // 对应最大速度寄存器值
    if (target_rpm > MAX_RPM)
    {
        target_rpm = MAX_RPM; // 限幅
    }
    // 线性换算：寄存器值 = target_rpm / MAX_RPM * MAX_REG
    uint16_t reg_value = ((uint32_t)target_rpm * MAX_REG) / MAX_RPM;
    ESP_LOGI(SERVO_TAG, "设置位置模式最高速度: %d RPM -> 寄存器值: %d", target_rpm, reg_value);
    // 写入寄存器0x1D
    SERVO_MOTOR_modbus_write_single_register(Position_Mode_Speed_Peak_reg, reg_value);
}

void SERVO_MOTOR_POS_Reg(int speed_rpm, int32_t position, int mode, bool use_sensor)
{
    if (speed_rpm >= 2600)
    {
        speed_rpm = 2600;
    }
    // 清除故障
    SERVO_MOTOR_Clear_the_fault();
    // 启动电机
    SERVO_MOTOR_Start();
    // 设置位置模式的最高速度
    SERVO_MOTOR_Set_Position_Speed(speed_rpm);
    // 设置为位置模式（0=绝对，1=相对）
    SERVO_MOTOR_set_Location_Mode(mode);

   vTaskDelay(pdMS_TO_TICKS(200));
    // 发送目标位置
    SERVO_MOTOR_modbus_write_multi_register(0x50, position);
    ESP_LOGI("SERVO_MOVE", "已发送目标位置: %ld，开始监测到位状态", position);

    vTaskDelay(pdMS_TO_TICKS(200));

    // ====== 判断检测模式 ======
    if (use_sensor)
    {
        ESP_LOGI("SERVO_MOVE", "启用传感器检测模式");

        // 使用传感器检测是否到位
        const int max_wait_ms = 10000; // 超时时间 10 秒
        int elapsed_ms = 0;

        while (elapsed_ms < max_wait_ms)
        {
            if (sensor_Right_get_state() == 0)
            {
                ESP_LOGI("SERVO_MOVE", "传感器触发，停止电机");
                break;
            }
            ESP_LOGI("SERVO_MOVE", "传感器未触发，电机继续");
            vTaskDelay(pdMS_TO_TICKS(50)); // 50ms 检测一次
            elapsed_ms += 50; // 问题4修复：与 vTaskDelay 对齐，原为 += 10 导致超时提前10倍触发
        }

        if (elapsed_ms >= max_wait_ms)
            ESP_LOGW("SERVO_MOVE", "等待超时，传感器未触发");
    }
    else
    {
        ESP_LOGI("SERVO_MOVE", "启用寄存器检测模式");

        // ====== 轮询寄存器判定是否到位 ======
        uint16_t pos_status = 1;
        const int max_wait_ms = 10000; // 超时时间 10 秒
        int elapsed_ms = 0;

        while (elapsed_ms < max_wait_ms)
        {
            if (modbus_read_single_register(Positioning_completed_reg, &pos_status))
            {
                if (pos_status == 0)
                {
                    // 到位信号
                    ESP_LOGI("SERVO_MOVE", "位置已到位 (寄存器值=%u)", pos_status);
                    break;
                }
                else
                {
                    ESP_LOGD("SERVO_MOVE", "位置未到位 (寄存器值=%u)", pos_status);
                }
            }
            else
            {
                ESP_LOGW("SERVO_MOVE", "读取寄存器失败");
            }

            vTaskDelay(pdMS_TO_TICKS(200));
            elapsed_ms += 200;
        }

        if (elapsed_ms >= max_wait_ms)
            ESP_LOGW("SERVO_MOVE", "等待超时，仍未到位");
    }

    // ====== 到位或超时后停止电机 ======
    SERVO_MOTOR_Stop();
    ESP_LOGI("SERVO_MOVE", "电机已停止");
}

// static bool modbus_send_and_wait_response(const uint8_t *tx_data, uint8_t tx_len,
//                                           uint8_t *rx_buf, uint16_t rx_buf_len,
//                                           uint16_t expect_min_len, const char *desc) {
//     const int MAX_RETRY = 3;
//     for (int attempt = 1; attempt <= MAX_RETRY; attempt++) {
//         SERVO_MOTOR_rs485_send_bytes(tx_data, tx_len);
//         int rx_len = uart_read_bytes(SERVO_MOTOR_UART_NUM, rx_buf, rx_buf_len, pdMS_TO_TICKS(200));
//         if (rx_len >= expect_min_len) {
//             // 校验CRC
//             uint16_t crc_calc = modbus_crc16(rx_buf, rx_len - 2);
//             uint16_t crc_recv = rx_buf[rx_len - 2] | (rx_buf[rx_len - 1] << 8);
//             if (crc_calc == crc_recv) {
//                 return true; // 成功
//             } else {
//                 ESP_LOGW(SERVO_TAG, "[%s] CRC错误，第%d次重试", desc, attempt);
//             }
//         } else {
//             ESP_LOGW(SERVO_TAG, "[%s] 无响应，第%d次重试", desc, attempt);
//         }
//         vTaskDelay(pdMS_TO_TICKS(MODBUS_DELAY_MS)); // 小延时再发
//     }
//     ESP_LOGE(SERVO_TAG, "[%s] 多次重试失败！", desc);
//     return false;
// }