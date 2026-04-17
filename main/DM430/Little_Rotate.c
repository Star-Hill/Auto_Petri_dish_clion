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

    //gpio_set_level(Little_ROTATE_STEPPER_DIR_GPIO, dir ? 1 : 0);    //原来的   这一行前面的双斜杠表示不执行
    gpio_set_level(Little_ROTATE_STEPPER_DIR_GPIO, dir ? 0 : 1);    //2026-4-10 修改旋转方向 小旋转电机的转向修改
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
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 1);  // 失能电机
}

// ==================== S 曲线加减速  ====================
void Little_ROTATE_stepper_rotate_ADS(float angle_deg, float rpm, int dir) {
    // 计算总步数
    uint32_t steps_total = (uint32_t)((angle_deg / 360.0f) * Little_STEPS_PER_REV * LITTLE_ROTATE_GEAR_RATIO);
    if (steps_total < 5) return;

    // 最大频率
    uint32_t freq_max = (uint32_t)((rpm / 60.0f) * Little_STEPS_PER_REV * LITTLE_ROTATE_GEAR_RATIO);
    if (freq_max < 300) freq_max = 300;

    //  将起步频率降低，使加速视觉可见
    uint32_t freq_start = 100;
    if (freq_start > freq_max - 50) freq_start = freq_max / 2;

    // 计算加减速段
    uint32_t accel_steps = steps_total / 7;
    uint32_t decel_steps = accel_steps;

    // 错误2修复：先裁剪加减速步数，再计算 uniform_steps
    // 原代码先算 uniform_steps 再裁剪，导致裁剪后实际总步数 < steps_total，电机转角偏少
    uint32_t delta = (freq_max > freq_start) ? (freq_max - freq_start) : (freq_start - freq_max);
    if (accel_steps > delta) accel_steps = decel_steps = delta;

    uint32_t uniform_steps = (steps_total > (accel_steps + decel_steps)) ?
                             (steps_total - accel_steps - decel_steps) : 0;

    // 配置方向与使能
    gpio_set_level(Little_ROTATE_STEPPER_DIR_GPIO, dir ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));

    // RMT 通道
    rmt_channel_handle_t motor_chan = NULL;
    rmt_tx_channel_config_t tx_conf = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = Little_ROTATE_STEPPER_STEP_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = Little_ROTATE_STEPPER_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_conf, &motor_chan));
    ESP_ERROR_CHECK(rmt_enable(motor_chan));
    rmt_transmit_config_t tx_cfg = {.loop_count = 0};

    // ---- 加速 ----
    stepper_motor_curve_encoder_config_t accel_cfg = {
        .resolution = Little_ROTATE_STEPPER_RESOLUTION_HZ,
        .sample_points = accel_steps,
        .start_freq_hz = freq_start,
        .end_freq_hz = freq_max,
    };
    rmt_encoder_handle_t accel_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&accel_cfg, &accel_encoder));
    ESP_ERROR_CHECK(rmt_transmit(motor_chan, accel_encoder, &accel_steps, sizeof(accel_steps), &tx_cfg));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));

    // ---- 匀速 ---- 使用步数控制，而非 loop_count
    if (uniform_steps > 0) {
        stepper_motor_uniform_encoder_config_t uniform_cfg = {
            .resolution = Little_ROTATE_STEPPER_RESOLUTION_HZ,
        };
        rmt_encoder_handle_t uniform_encoder = NULL;
        ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_cfg, &uniform_encoder));

        // 设置匀速频率参数
        uint32_t freq = freq_max;

        // 由 loop_count 控制步数
        tx_cfg.loop_count = uniform_steps;

        ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_encoder, &freq, sizeof(freq), &tx_cfg));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));

        rmt_del_encoder(uniform_encoder);
    }

    // 恢复 loop_count，防止减速段继续循环
    tx_cfg.loop_count = 0;

    // ---- 减速 ----
    stepper_motor_curve_encoder_config_t decel_cfg = {
        .resolution = Little_ROTATE_STEPPER_RESOLUTION_HZ,
        .sample_points = decel_steps,
        .start_freq_hz = freq_max,
        .end_freq_hz = freq_start,
    };
    rmt_encoder_handle_t decel_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&decel_cfg, &decel_encoder));
    ESP_ERROR_CHECK(rmt_transmit(motor_chan, decel_encoder, &decel_steps, sizeof(decel_steps), &tx_cfg));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));

    // 收尾
    rmt_del_encoder(accel_encoder);
    rmt_del_encoder(decel_encoder);
    rmt_disable(motor_chan);
    rmt_del_channel(motor_chan);

    // 关闭电机
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 1);
}


