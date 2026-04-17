//
// Created by DELL on 2025/8/8.
//

#include "Peristaltic_pump.h"

#include "Servo_motor_RS485_Speed_Location.h"

// ====== CRC16 校验函数 计算（Modbus RTU，多项式 0xA001，低字节先发）======
static uint16_t Peristaltic_pump_modbus_crc16(const uint8_t *buf, uint16_t len)
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

/********** RS485 发送数据 ***************/
void Peristaltic_pump_rs485_send_bytes(const uint8_t *data, uint8_t length)
{
    uart_flush_input(Peristaltic_pump_UART_NUM);
    if (uart_write_bytes(Peristaltic_pump_UART_NUM, (const char *)data, length) != length)
    {
        ESP_LOGE(Peristaltic_pump_TAG, "Send data failed.");
    }
    uart_wait_tx_done(Peristaltic_pump_UART_NUM, pdMS_TO_TICKS(MODBUS_DELAY_MS_P));
}

/********** 写单寄存器（带重试） **********/
void Peristaltic_pump_modbus_write_single_register(uint16_t reg_addr, uint16_t value)
{
    uint8_t frame[8];
    frame[0] = Peristaltic_pump_SLAVE_ADDR;
    frame[1] = Peristaltic_pump_MODBUS_WRITE_SINGLE_REGISTER;
    frame[2] = reg_addr >> 8;
    frame[3] = reg_addr & 0xFF;
    frame[4] = value >> 8;
    frame[5] = value & 0xFF;

    uint16_t crc = Peristaltic_pump_modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = crc >> 8;

#ifdef Peristaltic_pump_DEBUG
    ESP_LOG_BUFFER_HEX("TX", frame, sizeof(frame));
#endif

    for (int attempt = 0; attempt < MODBUS_MAX_RETRY; attempt++)
    {
        Peristaltic_pump_rs485_send_bytes(frame, sizeof(frame));
        vTaskDelay(pdMS_TO_TICKS(MODBUS_DELAY_MS_P));

        uint8_t rx_buf[256];
        int rx_len = uart_read_bytes(Peristaltic_pump_UART_NUM, rx_buf, sizeof(rx_buf),
                                     pdMS_TO_TICKS(MODBUS_DELAY_MS_P));
        ESP_LOGI(Peristaltic_pump_TAG, "RX LEN = %d", rx_len);
        if (rx_len >= 8)
        {
#ifdef Peristaltic_pump_DEBUG
            ESP_LOG_BUFFER_HEX("RX", rx_buf, rx_len);
#endif
            uint16_t crc_calc = Peristaltic_pump_modbus_crc16(rx_buf, rx_len - 2);
            uint16_t crc_recv = rx_buf[rx_len - 2] | (rx_buf[rx_len - 1] << 8);

            if (crc_calc == crc_recv)
            {
                return; // 通信成功
            }
            else
            {
                ESP_LOGW(Peristaltic_pump_TAG, "CRC error (try %d/%d)", attempt + 1, MODBUS_MAX_RETRY);
            }
        }
        else
        {
            ESP_LOGW(Peristaltic_pump_TAG, "No response (try %d/%d)", attempt + 1, MODBUS_MAX_RETRY);
        }
        vTaskDelay(pdMS_TO_TICKS(MODBUS_DELAY_MS_P));
    }
    ESP_LOGE(Peristaltic_pump_TAG, "Write single register failed after %d retries.", MODBUS_MAX_RETRY);
}

/* ===================== 蠕动泵 UART初始化 ===================== */
void Peristaltic_Pump_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = Peristaltic_pump_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB};

    ESP_ERROR_CHECK(uart_driver_install(Peristaltic_pump_UART_NUM, 1024, 1024, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(Peristaltic_pump_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(Peristaltic_pump_UART_NUM, Peristaltic_pump_TX, Peristaltic_pump_RX,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(Peristaltic_pump_TAG, "Peristaltic pump UART initialized.");
    ESP_LOGI(Peristaltic_pump_TAG, "蠕动泵 UART %d 初始化完成", Peristaltic_pump_UART_NUM);
}

/* ===================== 卸载蠕动泵 UART ===================== */
void Peristaltic_Pump_deinit(void)
{
    ESP_ERROR_CHECK(uart_driver_delete(Peristaltic_pump_UART_NUM));
    ESP_LOGI(Peristaltic_pump_TAG, "蠕动泵 UART %d 已卸载释放。", Peristaltic_pump_UART_NUM);
}

/* ===================== API 实现 ===================== */
void Peristaltic_pump_set_speed(int32_t rpm)
{
    uint16_t reg_addr = 3100;
    int32_t reg_value = rpm * 10;

    ESP_LOGI(Peristaltic_pump_TAG, "--- 设置蠕动泵转速: %ld rpm (寄存器值=%ld) ---", rpm, reg_value);
    Peristaltic_pump_modbus_write_single_register(reg_addr, reg_value);
}

void Peristaltic_pump_set_Reverse_Mode(uint16_t Mode)
{
    uint16_t reg_addr = 3101;
    Peristaltic_pump_modbus_write_single_register(reg_addr, Mode);
    ESP_LOGI(Peristaltic_pump_TAG, "当前蠕动泵方向: %s", (Mode == 1) ? "逆时针" : "顺时针");
}

void Peristaltic_pump_Control(bool enable)
{
    if (enable == true)
    {
        SERVO_MOTOR_deinit(); // 卸载伺服电机驱动
        vTaskDelay(pdMS_TO_TICKS(200));

        Peristaltic_Pump_init();        // 安装串口驱动
        vTaskDelay(pdMS_TO_TICKS(200)); // 稳定
    }

    Peristaltic_pump_set_speed(0);

    uint16_t reg_addr = 3102;
    Peristaltic_pump_modbus_write_single_register(reg_addr, enable ? 0x01 : 0x00);
    ESP_LOGI(Peristaltic_pump_TAG, "蠕动泵 %s", enable ? "启动" : "停止");

    if (enable == false)
    {
        Peristaltic_Pump_deinit();      // 卸载串口驱动
        vTaskDelay(pdMS_TO_TICKS(200)); // 稳定

        SERVO_MOTOR_init(); // 伺服电机驱动启用
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void Peristaltic_pump_Cleaning(bool Cleaning_Speed)
{
    uint16_t reg_addr = 3103;

    // 启动蠕动泵
    Peristaltic_pump_Control(true);

    // 设置清洗速度（全速/正常）
    Peristaltic_pump_modbus_write_single_register(reg_addr, Cleaning_Speed ? 0x01 : 0x00);
    ESP_LOGI(Peristaltic_pump_TAG, "蠕动泵清洗模式: %s", Cleaning_Speed ? "全速" : "正常速度");

    // 来回三次
    for (int i = 0; i < 3; i++)
    {
        // 顺时针
        Peristaltic_pump_set_Reverse_Mode(0); // 0=顺时针
        ESP_LOGI(Peristaltic_pump_TAG, "清洗循环 %d: 顺时针 3 秒", i + 1);
        vTaskDelay(pdMS_TO_TICKS(3000));

        // 逆时针
        Peristaltic_pump_set_Reverse_Mode(1); // 1=逆时针
        ESP_LOGI(Peristaltic_pump_TAG, "清洗循环 %d: 逆时针 3 秒", i + 1);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    // 停止蠕动泵
    Peristaltic_pump_Control(false);
    ESP_LOGI(Peristaltic_pump_TAG, "蠕动泵清洗完成");
}

void Peristaltic_pump_Run(int32_t speed, float run_time_s)
{
    // 问题A修复：run_time_s 为负时（体积过小导致 time_s - 0.3f < 0），跳过运行避免 vTaskDelay 溢出
    if (run_time_s <= 0.0f)
    {
        ESP_LOGW(Peristaltic_pump_TAG, "run_time_s=%.3f <= 0，跳过运行", run_time_s);
        return;
    }
    Peristaltic_pump_Control(true);
    // Peristaltic_pump_set_Reverse_Mode(0); // 0=顺时针 --原来的
    Peristaltic_pump_set_Reverse_Mode(1); // 1=逆时针 --现在改成逆时针 蠕动泵转向修改
    Peristaltic_pump_set_speed(speed);
    vTaskDelay(pdMS_TO_TICKS((uint32_t)(run_time_s * 1000)));
    Peristaltic_pump_Control(false);
}
