// Created by DELL on 2025/8/6
#include "Big_Rotate.h"

// ==================== 初始化 GPIO ====================
void Big_ROTATE_stepper_init(void) {
    // 配置 DIR + EN 引脚
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << Big_ROTATE_STEPPER_DIR_GPIO) | (1ULL << Big_ROTATE_STEPPER_EN_GPIO),
    };
    gpio_config(&io_conf);
    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 0); // 默认失能
    ESP_LOGI("Big_Stepping", "GPIO 初始化完成");
}

// ==================== 匀速旋转 ====================
void Big_ROTATE_stepper_rotate_US(float angle_deg, float rpm, int dir, bool check_sensor) {
    // 限制 rpm 最大为 6.0f
    if (rpm > 6.0f) {
        rpm = 6.0f;
    }
    uint32_t steps_total = (uint32_t)((angle_deg / 360.0f) * Big_ROTATE_STEPS_PER_REV * BIG_ROTATE_GEAR_RATIO);
    uint32_t freq_max = (uint32_t)((rpm / 60.0f) * Big_ROTATE_STEPS_PER_REV * BIG_ROTATE_GEAR_RATIO);

    ESP_LOGI("Big_Stepping", "angle=%.1f deg, rpm=%.1f, steps_total=%lu, max_freq=%lu Hz",
             angle_deg, rpm, steps_total, freq_max);

    if (steps_total == 0) return;

    // 传感器检查
    if (check_sensor && sensor_Calibration_get_state() == 0) {
        ESP_LOGI("Big_Stepping", "已在目标位置，无需驱动");
        return;
    }

    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 1);  //使能
    vTaskDelay(pdMS_TO_TICKS(50));

    gpio_set_level(Big_ROTATE_STEPPER_DIR_GPIO, dir ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(50));

    // ====== 动态申请 RMT 通道 ======
    rmt_channel_handle_t motor_chan = NULL;
    rmt_tx_channel_config_t tx_conf = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = Big_ROTATE_STEPPER_STEP_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = Big_ROTATE_STEPPER_RESOLUTION_HZ,
        .trans_queue_depth = Big_ROTATE_STEPPER_QUEUE_DEPTH,
    };
    if (rmt_new_tx_channel(&tx_conf, &motor_chan) != ESP_OK) {
        ESP_LOGE("Big_Stepping", "RMT 通道申请失败");
        gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 0);
        return;
    }
    rmt_enable(motor_chan);

    // 配置匀速编码器
    stepper_motor_uniform_encoder_config_t uniform_cfg = {
        .resolution = Big_ROTATE_STEPPER_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t uniform_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_cfg, &uniform_encoder));

    rmt_transmit_config_t tx_cfg = {.loop_count = (int)steps_total};
    ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_encoder, &freq_max, sizeof(freq_max), &tx_cfg));

    // 传感器检测
    if (check_sensor) {
        while (sensor_Calibration_get_state() != 0) {
            vTaskDelay(5);
        }
        rmt_disable(motor_chan);
        rmt_del_encoder(uniform_encoder);
        rmt_del_channel(motor_chan);
        gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 0);
        return;
    }

    ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));

    // 清理资源
    rmt_del_encoder(uniform_encoder);
    rmt_disable(motor_chan);
    rmt_del_channel(motor_chan);
    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 0); // 失能电机
}

// ==================== S 曲线加减速  ====================
void Big_ROTATE_stepper_rotate_ADS(float angle_deg, float rpm, int dir) {
    // 计算总步数
    uint32_t steps_total = (uint32_t)((angle_deg / 360.0f) * Big_ROTATE_STEPS_PER_REV * BIG_ROTATE_GEAR_RATIO);
    // 错误3修复：缺少零步数保护，steps_total=0 时 curve encoder 行为未定义
    if (steps_total < 5) return;

    // 最大频率
    uint32_t freq_max = (uint32_t)((rpm / 60.0f) * Big_ROTATE_STEPS_PER_REV * BIG_ROTATE_GEAR_RATIO);
    uint32_t freq_start = 3000;
    if (freq_start > freq_max - 50) freq_start = freq_max / 2;

    // 计算加减速段
    uint32_t accel_steps = steps_total / 10;
    uint32_t decel_steps = accel_steps;

    // 错误1修复：sample_points 不能超过 |freq_max - freq_start|，否则触发 ESP_ERR_INVALID_ARG
    uint32_t freq_delta = (freq_max > freq_start) ? (freq_max - freq_start) : (freq_start - freq_max);
    uint32_t sample_points = (freq_delta < 1000) ? freq_delta : 1000;
    // 同时将加减速步数限制在 sample_points 以内，保持 sample_points <= accel_steps 关系合理
    if (accel_steps > freq_delta) accel_steps = decel_steps = freq_delta;

    // uniform_steps 在加减速步数确定后计算，保证总步数正确
    uint32_t uniform_steps = (steps_total > (accel_steps + decel_steps))
                                 ? (steps_total - accel_steps - decel_steps)
                                 : 0;

    ESP_LOGI("Big_Stepper", "Total steps=%lu, accel=%lu, uniform=%lu, decel=%lu, freq_start=%lu, freq_max=%lu, sample_points=%lu",
             steps_total, accel_steps, uniform_steps, decel_steps, freq_start, freq_max, sample_points);

    // 配置方向与使能
    gpio_set_level(Big_ROTATE_STEPPER_DIR_GPIO, dir ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 1); // 使能（1=使能，0=失能）
    vTaskDelay(pdMS_TO_TICKS(10));

    // RMT 通道
    rmt_channel_handle_t motor_chan = NULL;
    rmt_tx_channel_config_t tx_conf = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = Big_ROTATE_STEPPER_STEP_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = Big_ROTATE_STEPPER_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_conf, &motor_chan));
    ESP_ERROR_CHECK(rmt_enable(motor_chan));
    rmt_transmit_config_t tx_cfg = {.loop_count = 0};

    // ---- 加速 ----
    stepper_motor_curve_encoder_config_t accel_cfg = {
        .resolution     = Big_ROTATE_STEPPER_RESOLUTION_HZ,
        .sample_points  = sample_points,
        .start_freq_hz  = freq_start,
        .end_freq_hz    = freq_max,
    };
    rmt_encoder_handle_t accel_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&accel_cfg, &accel_encoder));
    ESP_ERROR_CHECK(rmt_transmit(motor_chan, accel_encoder, &accel_steps, sizeof(accel_steps), &tx_cfg));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));

    // ---- 匀速 ----
    if (uniform_steps > 0) {
        stepper_motor_uniform_encoder_config_t uniform_cfg = {
            .resolution = Big_ROTATE_STEPPER_RESOLUTION_HZ,
        };
        rmt_encoder_handle_t uniform_encoder = NULL;
        ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_cfg, &uniform_encoder));

        uint32_t freq = freq_max;
        tx_cfg.loop_count = uniform_steps;

        ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_encoder, &freq, sizeof(freq), &tx_cfg));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));
        rmt_del_encoder(uniform_encoder);
    }

    // ---- 减速 ----
    stepper_motor_curve_encoder_config_t decel_cfg = {
        .resolution     = Big_ROTATE_STEPPER_RESOLUTION_HZ,
        .sample_points  = sample_points,
        .start_freq_hz  = freq_max,
        .end_freq_hz    = freq_start,
    };
    rmt_encoder_handle_t decel_encoder = NULL;
    tx_cfg.loop_count = 0;

    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&decel_cfg, &decel_encoder));
    ESP_ERROR_CHECK(rmt_transmit(motor_chan, decel_encoder, &decel_steps, sizeof(decel_steps), &tx_cfg));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));

    // 收尾
    rmt_del_encoder(accel_encoder);
    rmt_del_encoder(decel_encoder);
    rmt_disable(motor_chan);
    rmt_del_channel(motor_chan);

    gpio_set_level(Big_ROTATE_STEPPER_EN_GPIO, 0); // 失能
}
