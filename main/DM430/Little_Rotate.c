//
// Created by DELL on 2025/10/24
//
#include "Little_Rotate.h"

// ==================== 初始化 GPIO ====================
void Little_stepper_init(void) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << Little_ROTATE_STEPPER_DIR_GPIO) | (1ULL << Little_ROTATE_STEPPER_EN_GPIO),
    };
    gpio_config(&io_conf);
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 1); // 默认失能
    ESP_LOGI("Little_Stepping", "GPIO 初始化完成");
}

// ==================== 匀速旋转 ====================
void Little_stepper_rotate_US(float angle_deg, float rpm, int dir) {
    uint32_t steps_total = (uint32_t)((angle_deg / 360.0f) * Little_STEPS_PER_REV * LITTLE_ROTATE_GEAR_RATIO);
    uint32_t freq_max = (uint32_t)((rpm / 60.0f) * Little_STEPS_PER_REV * LITTLE_ROTATE_GEAR_RATIO);

    ESP_LOGI("Little_Stepping", "angle=%.1f deg, rpm=%.1f, steps_total=%lu, max_freq=%lu Hz",
             angle_deg, rpm, steps_total, freq_max);

    if (steps_total == 0) return;

    gpio_set_level(Little_ROTATE_STEPPER_DIR_GPIO, dir ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    rmt_channel_handle_t motor_chan = NULL;
    rmt_tx_channel_config_t tx_conf = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = Little_ROTATE_STEPPER_STEP_GPIO,
        .mem_block_symbols = 48,
        .resolution_hz = Little_ROTATE_STEPPER_RESOLUTION_HZ,
        .trans_queue_depth = Little_ROTATE_STEPPER_QUEUE_DEPTH,
    };

    if (rmt_new_tx_channel(&tx_conf, &motor_chan) != ESP_OK) {
        ESP_LOGE("Little_Stepping", "RMT 通道申请失败");
        gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 1);
        return;
    }
    rmt_enable(motor_chan);

    stepper_motor_uniform_encoder_config_t uniform_cfg = {
        .resolution = Little_ROTATE_STEPPER_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t uniform_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_cfg, &uniform_encoder));

    rmt_transmit_config_t tx_cfg = { .loop_count = (int)steps_total };
    ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_encoder, &freq_max, sizeof(freq_max), &tx_cfg));

    ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));

    rmt_del_encoder(uniform_encoder);
    rmt_disable(motor_chan);
    rmt_del_channel(motor_chan);
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 1);
}