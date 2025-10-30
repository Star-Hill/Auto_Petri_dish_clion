// Created by DELL on 2025/8/6
#include "UpDown.h"

// ==================== 初始化 GPIO ====================
void UpDown_stepper_init(void) {
    // 配置 DIR + EN 引脚
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << UpDown_STEPPER_DIR_GPIO) | (1ULL << UpDown_STEPPER_EN_GPIO),
    };
    gpio_config(&io_conf);
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 1); // 默认失能
    ESP_LOGI("UpDown_Stepping", "GPIO 初始化完成");
}

// ==================== 步进电机匀速旋转 ====================
void UpDown_stepper_rotate(float angle_deg, float rpm, int dir, int check_sensor) {
    uint32_t steps_total = (uint32_t)((angle_deg / 360.0f) * UpDown_STEPS_PER_REV * UpDown_ROTATE_GEAR_RATIO);
    uint32_t freq_max = (uint32_t)((rpm / 60.0f) * UpDown_STEPS_PER_REV * UpDown_ROTATE_GEAR_RATIO);

    ESP_LOGI("UpDown_Stepping", "angle=%.1f deg, rpm=%.1f, steps_total=%lu, max_freq=%lu Hz",
             angle_deg, rpm, steps_total, freq_max);

    if (steps_total == 0) return;

    // 传感器检查
    if (check_sensor && sensor_Down_get_state() == 0) {
        ESP_LOGI("UpDown_Stepping", "已在目标位置，无需驱动");
        return;
    }

    gpio_set_level(UpDown_STEPPER_DIR_GPIO, dir ? 0 : 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 0); // 使能电机
    vTaskDelay(pdMS_TO_TICKS(50));

    // ====== 动态申请 RMT 通道 ======
    rmt_channel_handle_t motor_chan = NULL;
    rmt_tx_channel_config_t tx_conf = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = UpDown_STEPPER_STEP_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = UpDown_STEPPER_RESOLUTION_HZ,
        .trans_queue_depth = UpDown_STEPPER_QUEUE_DEPTH,
    };
    if (rmt_new_tx_channel(&tx_conf, &motor_chan) != ESP_OK) {
        ESP_LOGE("UpDown_Stepping", "RMT 通道申请失败");
        gpio_set_level(UpDown_STEPPER_EN_GPIO, 1);
        return;
    }
    rmt_enable(motor_chan);

    // 配置匀速编码器
    stepper_motor_uniform_encoder_config_t uniform_cfg = {
        .resolution = UpDown_STEPPER_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t uniform_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_cfg, &uniform_encoder));

    rmt_transmit_config_t tx_cfg = {.loop_count = (int)steps_total};
    ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_encoder, &freq_max, sizeof(freq_max), &tx_cfg));

    // 传感器检测
    if (check_sensor) {
        while (sensor_Down_get_state() != 0) {
            vTaskDelay(5);
        }
        rmt_disable(motor_chan);
        rmt_del_encoder(uniform_encoder);
        rmt_del_channel(motor_chan);
        gpio_set_level(UpDown_STEPPER_EN_GPIO, 1);
        return;
    }

    ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));

    // 清理资源
    rmt_del_encoder(uniform_encoder);
    rmt_disable(motor_chan);
    rmt_del_channel(motor_chan);
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 1); // 失能电机
}
