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
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 0); // 默认失能
    ESP_LOGI("UpDown_Stepping", "GPIO 初始化完成");
}

// ==================== 步进电机匀速旋转 ====================
void UpDown_stepper_rotate(float angle_deg, float rpm, int dir, int check_sensor) {
    if (rpm >= 600) {
        rpm = 600;
    }
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
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 1); // 使能电机
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
        gpio_set_level(UpDown_STEPPER_EN_GPIO, 0);
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
        gpio_set_level(UpDown_STEPPER_EN_GPIO, 0);
        return;
    }

    ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));

    // 清理资源
    rmt_del_encoder(uniform_encoder);
    rmt_disable(motor_chan);
    rmt_del_channel(motor_chan);
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 0); // 失能电机
}

// ==================== S 曲线加减速 ====================
void UpDown_stepper_rotate_ADS(float angle_deg, float rpm, int dir, int check_sensor) {
    if (rpm >= 600) rpm = 600;

    uint32_t steps_total = (uint32_t)((angle_deg / 360.0f) * UpDown_STEPS_PER_REV * UpDown_ROTATE_GEAR_RATIO);
    if (steps_total < 5) return;

    uint32_t freq_max = (uint32_t)((rpm / 60.0f) * UpDown_STEPS_PER_REV * UpDown_ROTATE_GEAR_RATIO);
    if (freq_max < 300) freq_max = 300;

    // 起步频率 300 Hz（约11 rpm），保证升降轴低速平稳起步
    uint32_t freq_start = 300;
    if (freq_start >= freq_max) freq_start = freq_max / 2;

    // 传感器预检：已在目标位置则跳过
    if (check_sensor && sensor_Down_get_state() == 0) {
        ESP_LOGI("UpDown_ADS", "已在目标位置，无需驱动");
        return;
    }

    // 先裁剪加减速步数，再计算匀速步数，保证三段之和 == steps_total
    uint32_t accel_steps = steps_total / 7;
    uint32_t decel_steps = accel_steps;
    uint32_t freq_delta   = freq_max - freq_start;
    uint32_t sample_points = (freq_delta < 500) ? freq_delta : 500;
    if (accel_steps > freq_delta) accel_steps = decel_steps = freq_delta;
    uint32_t uniform_steps = (steps_total > accel_steps + decel_steps)
                             ? (steps_total - accel_steps - decel_steps) : 0;

    ESP_LOGI("UpDown_ADS",
             "angle=%.1f deg, rpm=%.1f, steps=%lu, accel=%lu, uniform=%lu, decel=%lu, "
             "freq_start=%lu, freq_max=%lu, sample_points=%lu",
             angle_deg, rpm, steps_total, accel_steps, uniform_steps, decel_steps,
             freq_start, freq_max, sample_points);

    gpio_set_level(UpDown_STEPPER_DIR_GPIO, dir ? 0 : 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // RMT 通道
    rmt_channel_handle_t motor_chan = NULL;
    rmt_tx_channel_config_t tx_conf = {
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .gpio_num          = UpDown_STEPPER_STEP_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz     = UpDown_STEPPER_RESOLUTION_HZ,
        .trans_queue_depth = UpDown_STEPPER_QUEUE_DEPTH,
    };
    if (rmt_new_tx_channel(&tx_conf, &motor_chan) != ESP_OK) {
        ESP_LOGE("UpDown_ADS", "RMT 通道申请失败");
        gpio_set_level(UpDown_STEPPER_EN_GPIO, 0);
        return;
    }
    rmt_enable(motor_chan);

    // 提前创建三段所需的编码器
    stepper_motor_curve_encoder_config_t accel_cfg = {
        .resolution    = UpDown_STEPPER_RESOLUTION_HZ,
        .sample_points = sample_points,
        .start_freq_hz = freq_start,
        .end_freq_hz   = freq_max,
    };
    rmt_encoder_handle_t accel_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&accel_cfg, &accel_encoder));

    stepper_motor_curve_encoder_config_t decel_cfg = {
        .resolution    = UpDown_STEPPER_RESOLUTION_HZ,
        .sample_points = sample_points,
        .start_freq_hz = freq_max,
        .end_freq_hz   = freq_start,
    };
    rmt_encoder_handle_t decel_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&decel_cfg, &decel_encoder));

    rmt_encoder_handle_t uniform_encoder = NULL;
    if (uniform_steps > 0) {
        stepper_motor_uniform_encoder_config_t uniform_cfg = {
            .resolution = UpDown_STEPPER_RESOLUTION_HZ,
        };
        ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_cfg, &uniform_encoder));
    }

    rmt_transmit_config_t tx_cfg = {.loop_count = 0};
    bool stopped_by_sensor = false;

    // ---- 加速段 ----
    ESP_ERROR_CHECK(rmt_transmit(motor_chan, accel_encoder, &accel_steps, sizeof(accel_steps), &tx_cfg));
    if (check_sensor) {
        while (rmt_tx_wait_all_done(motor_chan, 10) == ESP_ERR_TIMEOUT) {
            if (sensor_Down_get_state() == 0) { stopped_by_sensor = true; break; }
        }
    } else {
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));
    }

    // ---- 匀速段 ----
    if (!stopped_by_sensor && uniform_steps > 0) {
        uint32_t freq = freq_max;
        tx_cfg.loop_count = uniform_steps;
        ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_encoder, &freq, sizeof(freq), &tx_cfg));
        tx_cfg.loop_count = 0;
        if (check_sensor) {
            while (rmt_tx_wait_all_done(motor_chan, 10) == ESP_ERR_TIMEOUT) {
                if (sensor_Down_get_state() == 0) { stopped_by_sensor = true; break; }
            }
        } else {
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));
        }
    }

    // ---- 减速段 ----
    if (!stopped_by_sensor) {
        ESP_ERROR_CHECK(rmt_transmit(motor_chan, decel_encoder, &decel_steps, sizeof(decel_steps), &tx_cfg));
        if (check_sensor) {
            while (rmt_tx_wait_all_done(motor_chan, 10) == ESP_ERR_TIMEOUT) {
                if (sensor_Down_get_state() == 0) { stopped_by_sensor = true; break; }
            }
        } else {
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));
        }
    }

    if (stopped_by_sensor) {
        ESP_LOGI("UpDown_ADS", "传感器触发，提前停止");
    }

    // 清理资源（rmt_disable 先于 del，确保传输已停止）
    rmt_disable(motor_chan);
    rmt_del_encoder(accel_encoder);
    if (uniform_encoder) rmt_del_encoder(uniform_encoder);
    rmt_del_encoder(decel_encoder);
    rmt_del_channel(motor_chan);
    gpio_set_level(UpDown_STEPPER_EN_GPIO, 0);
}
