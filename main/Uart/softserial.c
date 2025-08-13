//
// Created by DELL on 2025/8/10.
//
#include <string.h>
#include "softserial.h"

/*
 * 计算每位时间（CPU周期）
 * @param baud_rate 波特率
 * @return 每位对应CPU周期数
 * @note 乘以2是为了补偿ESP32的GPIO采样延迟和分频机制
 */
uint64_t calculate_bit_time(uint32_t baud_rate)
{
    //uint64_t bit_time_us = (1000000UL / baud_rate); //原本计算会有四舍五入积累误差，故舍去

    return 2 *  (80000000 / baud_rate);  // 转换为CPU周期数
    //CPU_CLK_FREQ = 80Mhz 但是 cpu经过倍频 其真正频率为 = 160Mhz 通过使用esp_cpu_get_cycle_count()可以验证其频率
}


/*
 * 精确等待指定CPU周期
 * @param star 指向起始时间的指针（自动更新）
 * @param bit_cycles 需要等待的周期数
 * @notestar 变为+=bit_cycles
 */
void wait_time(uint64_t* star , uint64_t bit_cycles)
{
    uint64_t end = *star + bit_cycles;
    while(esp_cpu_get_cycle_count() < end);
    *star = end;
}

/*
 * 接收中断服务例程（ISR）
 * @param arg 软件UART结构体指针
 * @note 必须在临界区中处理中断
 */
void IRAM_ATTR software_uart_rx_isr(void* arg)
{
    SoftwareUART* uart = (SoftwareUART*)arg;
    portBASE_TYPE higher_priority_task_woken = pdFALSE;

    // 进入临界区
    portENTER_CRITICAL_ISR(&uart->lock);
    // 检测起始位（下降沿触发）
    if (gpio_get_level(uart->rx_pin) == 0)
    {
        uint8_t data = 0;
        uint64_t star = esp_cpu_get_cycle_count();
        wait_time(&star , (uart->bit_cycles)/2);//除以二保证在bit中位读取数据
        //wait_time(&star , (uart->bit_cycles)/3); //if 115200
        if(gpio_get_level(uart->rx_pin) == 0)//起始位
        {
            // 读取8位数据（LSB到 MSB）
            for (int i = 0; i < 8; i++)
            {
                wait_time(&star , uart->bit_cycles);  // 等待至采样点
                data |= (gpio_get_level(uart->rx_pin) << i);
                // 等待下一周期
            }
            // 检查停止位（应为高电平）
            wait_time(&star , uart->bit_cycles);
            if (gpio_get_level(uart->rx_pin) == 1) //停止位
            {
                // 存入缓冲区
                uint16_t next_head = (uart->rx_buffer.head + 1) % uart->rx_buff_size;
                if (next_head != uart->rx_buffer.tail)
                {
                    uart->rx_buffer.buffer[uart->rx_buffer.head] = data;
                    uart->rx_buffer.head = next_head;
                }
            }
        }

    }
    portEXIT_CRITICAL_ISR(&uart->lock);
}

/*
 * 初始化软件UART
 * @param uart 软件UART结构体指针
 * @note 默认配置：TX=11, RX=12, 波特率38400, 缓冲区128字节
 */
void software_uart_init(SoftwareUART *uart)
{
    if (uart->tx_pin == 0) uart->tx_pin = 11;    // 默认TX引脚
    if (uart->rx_pin == 0) uart->rx_pin = 12;    // 默认RX引脚
    if (uart->rx_buff_size == 0) uart->rx_buff_size = 128; // 默认缓冲区大小
    if (uart->baud_rate == 0) uart->baud_rate = 38400; // 默认波特率

    // 计算每位时间(cpu周期)
    uart->bit_cycles = calculate_bit_time(uart->baud_rate);

    //缓存区配置
    uart->rx_buffer.buffer = (uint8_t*)malloc(uart->rx_buff_size * sizeof(uint8_t));

    // 配置发送引脚为输出
    gpio_config_t tx_conf =
            {
                    .pin_bit_mask = (1ULL << uart->tx_pin),
                    .mode = GPIO_MODE_OUTPUT,
                    .pull_up_en = GPIO_PULLUP_ENABLE,
                    .pull_down_en = GPIO_PULLUP_ENABLE,
                    .intr_type = GPIO_INTR_DISABLE
            };
    ESP_ERROR_CHECK(gpio_config(&tx_conf));
    gpio_set_level(uart->tx_pin, 1);

    // 配置接收引脚为输入，并启用中断
    gpio_config_t rx_conf =
            {
                    .pin_bit_mask = (1ULL << uart->rx_pin),
                    .mode = GPIO_MODE_INPUT,
                    .pull_up_en = GPIO_PULLUP_ENABLE,
                    .pull_down_en = GPIO_PULLDOWN_DISABLE,
                    .intr_type = GPIO_INTR_NEGEDGE  // 检测起始位下降沿
            };
    ESP_ERROR_CHECK(gpio_config(&rx_conf));

    // 初始化接收缓冲区
    uart->rx_buffer.head = 0;
    uart->rx_buffer.tail = 0;

    // 初始化临界区锁
    portMUX_INITIALIZE(&uart->lock);

    // 安装GPIO中断服务
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);  // 只有不是重复安装的错误才会触发 abort
    }
    ESP_ERROR_CHECK(gpio_isr_handler_add(uart->rx_pin, software_uart_rx_isr,(void*)uart));//传入rx 与结构体的值
}


/*
 * 发送一个字节（8位数据 + 起始位 + 停止位）
 * @param uart 软件UART结构体指针
 * @param data 需要发送的8位数据
 */
void software_uart_tx_byte(SoftwareUART *uart, uint8_t data)
{
    portENTER_CRITICAL(&uart->lock);
    //开始时间
    uint64_t star = esp_cpu_get_cycle_count();
    // 发送起始位（低电平）
    gpio_set_level(uart->tx_pin, 0);
    //持续一个bit
    wait_time(&star , uart->bit_cycles);
    // 发送8位数据（LSB先发送）
    for (int i = 0; i < 8; i++)
    {
        gpio_set_level(uart->tx_pin, (data >> i) & 0x01);
        //持续一个bit
        wait_time(&star , uart->bit_cycles);
    }

    // 发送停止位（高电平）
    gpio_set_level(uart->tx_pin, 1);
    //持续一个bit
    wait_time(&star , uart->bit_cycles);
    portEXIT_CRITICAL(&uart->lock);
}

/*
 * 发送字符串（逐字节发送）
 * @param uart 软件UART结构体指针
 * @param str 需要发送的以NULL结尾的字符串
 */
void software_uart_tx_str(SoftwareUART *uart,const char* str)
{
    while (*str)
    {
        software_uart_tx_byte(uart,*str++);
    }
}

/*
 * 从接收缓冲区读取数据
 * @param uart 软件UART结构体指针
 * @param data 存储读取数据的缓冲区
 * @param size 需要读取的最大字节数
 * @return 实际读取的字节数
 */
int software_uart_rx_read(SoftwareUART *uart, uint8_t* data, size_t size)
{
    //临界区
    portENTER_CRITICAL(&uart->lock);

    //获取可读缓存区大小
    size_t available = (uart->rx_buffer.head - uart->rx_buffer.tail) % uart->rx_buff_size;
    size_t read_size = (available < size) ? available : size;

    //读取到data
    for (size_t i = 0; i < read_size; i++)
    {
        data[i] = uart->rx_buffer.buffer[uart->rx_buffer.tail];
        uart->rx_buffer.tail = (uart->rx_buffer.tail + 1) % uart->rx_buff_size;
    }

    portEXIT_CRITICAL(&uart->lock);
    return read_size;
}

/*
 *  该程序不带任何操作，配置下方引脚即可测试软串口通讯是否正常
 * */
void software_uart_test(void){
    static const char *TAG = "SoftwareUART";
    SoftwareUART uart = {
            .baud_rate = 115200,
            .rx_buff_size = 1024,
            .tx_pin = 5,
            .rx_pin = 6,
    };

    software_uart_init(&uart);

    ESP_LOGI(TAG, "Software UART Test Starting...");
    software_uart_tx_str(&uart, "Hello, Software UART!\r\n");

    char recv_str[128]; // 用来保存一次读到的字符串

    while (1) {
        // 读缓冲区里所有数据
        int len = software_uart_rx_read(&uart, (uint8_t *)recv_str, sizeof(recv_str) - 1);

        if (len > 0) {
            recv_str[len] = '\0'; // 保证是 C 字符串
            ESP_LOGI(TAG, "收到字符串: %s", recv_str);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}