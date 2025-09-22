//
// Created by DELL on 2025/8/6.
//
#include "Big_Rotate.h"
#include "driver/rmt_tx.h"
#include "stepper_motor_encoder.h"

static rmt_channel_handle_t big_motor_chan = NULL;

void Big_ROTATE_stepper_init(void) {
    // 配置 DIR + EN 引脚
    gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << Big_ROTATE_STEPPER_DIR_GPIO) | (1ULL << Big_ROTATE_STEPPER_EN_GPIO),
    };
    gpio_config(&io_conf);
    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 0); // 默认失能（低电平有效）

    // 配置 RMT 通道
    rmt_tx_channel_config_t tx_conf = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .gpio_num = Big_ROTATE_STEPPER_STEP_GPIO,
            .mem_block_symbols = 64,
            .resolution_hz = Big_ROTATE_STEPPER_RESOLUTION_HZ,
            .trans_queue_depth = Big_ROTATE_STEPPER_QUEUE_DEPTH,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_conf, &big_motor_chan));
    ESP_ERROR_CHECK(rmt_enable(big_motor_chan));

    ESP_LOGI("Big_Stepping", "RMT 步进电机初始化完成");
}

void Big_ROTATE_stepper_rotate(float angle_deg, float rpm, int dir, bool check_sensor, uint32_t sample_points) {
    uint32_t steps_total = (uint32_t) ((angle_deg / 360.0f) * Big_ROTATE_STEPS_PER_REV * BIG_ROTATE_GEAR_RATIO);
    uint32_t freq_max = (uint32_t) ((rpm / 60.0f) * Big_ROTATE_STEPS_PER_REV * BIG_ROTATE_GEAR_RATIO);

    ESP_LOGI("Big_Stepping", "angle=%.1f deg, rpm=%.1f, steps_total=%lu, max_freq=%lu Hz",
             angle_deg, rpm, steps_total, freq_max);

    if (steps_total == 0) return;

    // ====== 传感器检查 ======
    if (check_sensor && sensor_Calibration_get_state() == 0) {
        ESP_LOGI("Big_Stepping", "已在目标位置，无需驱动");
        return;
    }

    gpio_set_level(Big_ROTATE_STEPPER_DIR_GPIO, dir ? 1 : 0);
    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 0);

    // ====== 三段步数 ======
    uint32_t accel_steps = steps_total / 10;       //  加速
    uint32_t decel_steps = steps_total / 10;       //  减速
    uint32_t uniform_steps = steps_total - accel_steps - decel_steps;

    ESP_LOGI("Big_Stepping", "accel=%lu, uniform=%lu, decel=%lu",
             accel_steps, uniform_steps, decel_steps);

    // ====== 编码器配置 ======
    stepper_motor_curve_encoder_config_t accel_cfg = {
            .resolution     = Big_ROTATE_STEPPER_RESOLUTION_HZ,
            .sample_points  = accel_steps,       // 加速段点数 = 步数
            .start_freq_hz  = freq_max / 10,     // 起始频率
            .end_freq_hz    = freq_max,          // 匀速频率
    };
    stepper_motor_uniform_encoder_config_t uniform_cfg = {
            .resolution     = Big_ROTATE_STEPPER_RESOLUTION_HZ,
    };
    stepper_motor_curve_encoder_config_t decel_cfg = {
            .resolution     = Big_ROTATE_STEPPER_RESOLUTION_HZ,
            .sample_points  = decel_steps,       // 减速段点数 = 步数
            .start_freq_hz  = freq_max,
            .end_freq_hz    = freq_max / 10,
    };

    rmt_encoder_handle_t accel_encoder = NULL;
    rmt_encoder_handle_t uniform_encoder = NULL;
    rmt_encoder_handle_t decel_encoder = NULL;

    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&accel_cfg, &accel_encoder));
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_cfg, &uniform_encoder));
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&decel_cfg, &decel_encoder));

    rmt_transmit_config_t tx_cfg = {
            .loop_count = 0,
    };

    // ====== 加速 ======
    if (accel_steps > 0) {
        tx_cfg.loop_count = 0;
        ESP_ERROR_CHECK(rmt_transmit(big_motor_chan, accel_encoder,
                                     &accel_steps, sizeof(accel_steps), &tx_cfg));
    }

    // ====== 匀速 ======
    if (uniform_steps > 0) {
        tx_cfg.loop_count = (int)uniform_steps;  // 匀速步数
        ESP_ERROR_CHECK(rmt_transmit(big_motor_chan, uniform_encoder,
                                     &freq_max, sizeof(freq_max), &tx_cfg));
    }

    // ====== 减速 ======
    if (decel_steps > 0) {
        tx_cfg.loop_count = 0;
        ESP_ERROR_CHECK(rmt_transmit(big_motor_chan, decel_encoder,
                                     &decel_steps, sizeof(decel_steps), &tx_cfg));
    }

    ESP_ERROR_CHECK(rmt_tx_wait_all_done(big_motor_chan, -1));
    // ====== 清理 ======
    rmt_del_encoder(accel_encoder);
    rmt_del_encoder(uniform_encoder);
    rmt_del_encoder(decel_encoder);

    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 1);  // 高电平失能
}
