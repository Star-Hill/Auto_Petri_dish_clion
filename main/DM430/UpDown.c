//
// Created by DELL on 2025/8/6.
//
#include "UpDown.h"
#include "driver/rmt_tx.h"
#include "stepper_motor_encoder.h"

static const char *TAG = "UpDown_STEPPER_CTRL";

// RMT 资源
static rmt_channel_handle_t updown_motor_chan = NULL;
static rmt_encoder_handle_t updown_uniform_encoder = NULL;

// 步进电机初始化
void UpDown_stepper_init(void) {
    // DIR + EN 引脚
    gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << UpDown_STEPPER_DIR_GPIO) | (1ULL << UpDown_STEPPER_EN_GPIO),
    };
    gpio_config(&io_conf);
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 0); // 默认失能（低电平有效）

    // 配置 RMT 通道
    rmt_tx_channel_config_t tx_conf = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .gpio_num = UpDown_STEPPER_STEP_GPIO,
            .mem_block_symbols = 48,
            .resolution_hz = UpDown_STEPPER_RESOLUTION_HZ,
            .trans_queue_depth = UpDown_STEPPER_QUEUE_DEPTH,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_conf, &updown_motor_chan));
    ESP_ERROR_CHECK(rmt_enable(updown_motor_chan));

    // 匀速编码器
    stepper_motor_uniform_encoder_config_t uniform_conf = {
            .resolution = UpDown_STEPPER_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_conf, &updown_uniform_encoder));

    ESP_LOGI(TAG, "上下电机初始化完成 (RMT)");
}

void UpDown_stepper_rotate(float angle_deg, float rpm, int dir, int check_sensor) {
    uint32_t steps = (uint32_t) ((angle_deg / 360.0f) * UpDown_STEPS_PER_REV);
    uint32_t freq = (uint32_t) ((rpm / 60.0f) * UpDown_STEPS_PER_REV);

    ESP_LOGI(TAG, "Angle=%.1f deg, RPM=%.1f, steps=%lu, freq=%lu Hz",
             angle_deg, rpm, steps, freq);

    // === 先判断是否已经在目标位置 ===
    if (check_sensor == 2 && sensor_Down_get_state() == 0) {
        ESP_LOGI(TAG, "电机已在目标位置，无需移动");
        return;
    }

    // 设置方向
    gpio_set_level(UpDown_STEPPER_DIR_GPIO, dir ? 0 : 1);
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 0);   // 使能

    rmt_transmit_config_t tx_cfg = {
            .loop_count = (int) steps,
    };

    ESP_ERROR_CHECK(rmt_transmit(updown_motor_chan, updown_uniform_encoder,
                                 &freq, sizeof(freq), &tx_cfg));

    if (check_sensor == 2) {  // 找下限
        while (1) {
            if (sensor_Down_get_state() == 0) {
                ESP_LOGW(TAG, "下限传感器触发，停止电机");
                rmt_disable(updown_motor_chan);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    } else {  // 定角度阻塞运行
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(updown_motor_chan, -1));
    }
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 1);   // 失能
    ESP_LOGI(TAG, "到达位置，停止电机");
}