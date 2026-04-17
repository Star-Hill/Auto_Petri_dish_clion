#include "stepper_driver.h"

// 使能 / 失能辅助宏，根据 STEPPER_EN_ACTIVE_LEVEL 自动取反
#define EN_ON()   gpio_set_level(STEPPER_EN_GPIO,  STEPPER_EN_ACTIVE_LEVEL)
#define EN_OFF()  gpio_set_level(STEPPER_EN_GPIO, !STEPPER_EN_ACTIVE_LEVEL)

// 方向电平：统一经过 STEPPER_DIR_INVERT 反转
#define DIR_LEVEL(d)  ((STEPPER_DIR_INVERT) ? !(d) : (d))

// ==================== 初始化 ====================

void stepper_init(void)
{
    gpio_config_t io_conf = {
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << STEPPER_DIR_GPIO) | (1ULL << STEPPER_EN_GPIO),
    };
    gpio_config(&io_conf);
    EN_OFF();
    ESP_LOGI(STEPPER_LOG_TAG, "步进电机 GPIO 初始化完成 "
             "(STEP=%d, DIR=%d, EN=%d, EN_active=%d)",
             STEPPER_STEP_GPIO, STEPPER_DIR_GPIO,
             STEPPER_EN_GPIO, STEPPER_EN_ACTIVE_LEVEL);
}

// ==================== 内部辅助 ====================

// 创建并使能 RMT 通道；失败时自动关闭 EN 并返回 NULL
static rmt_channel_handle_t prv_create_rmt_channel(void)
{
    rmt_channel_handle_t chan = NULL;
    rmt_tx_channel_config_t tx_conf = {
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .gpio_num          = STEPPER_STEP_GPIO,
        .mem_block_symbols = STEPPER_RMT_MEM_SYMBOLS,
        .resolution_hz     = STEPPER_RMT_RESOLUTION_HZ,
        .trans_queue_depth = STEPPER_RMT_QUEUE_DEPTH,
    };
    if (rmt_new_tx_channel(&tx_conf, &chan) != ESP_OK) {
        ESP_LOGE(STEPPER_LOG_TAG, "RMT 通道申请失败");
        EN_OFF();
        return NULL;
    }
    rmt_enable(chan);
    return chan;
}

// 等待 RMT 完成，期间每 10 ms 轮询一次传感器
// 返回 true  = 传感器触发，RMT 仍在运行（调用方负责 rmt_disable）
// 返回 false = 正常完成
static bool prv_wait_or_sensor(rmt_channel_handle_t chan, stepper_sensor_fn_t sensor_fn)
{
    if (sensor_fn == NULL) {
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(chan, -1));
        return false;
    }
    while (rmt_tx_wait_all_done(chan, 10) == ESP_ERR_TIMEOUT) {
        if (sensor_fn() == 0) {
            ESP_LOGI(STEPPER_LOG_TAG, "传感器触发，停止运动");
            return true;
        }
    }
    return false;
}

// ==================== 匀速旋转 ====================

void stepper_rotate_US(float angle_deg, float rpm, int dir, stepper_sensor_fn_t sensor_fn)
{
    if (rpm > STEPPER_RPM_MAX) rpm = STEPPER_RPM_MAX;

    uint32_t steps_total = (uint32_t)((angle_deg / 360.0f)
                            * STEPPER_STEPS_PER_REV * STEPPER_GEAR_RATIO);
    uint32_t freq        = (uint32_t)((rpm / 60.0f)
                            * STEPPER_STEPS_PER_REV * STEPPER_GEAR_RATIO);

    ESP_LOGI(STEPPER_LOG_TAG, "US  angle=%.1f deg  rpm=%.1f  steps=%lu  freq=%lu Hz",
             angle_deg, rpm, steps_total, freq);

    if (steps_total == 0 || freq == 0) return;

    if (sensor_fn && sensor_fn() == 0) {
        ESP_LOGI(STEPPER_LOG_TAG, "已在目标位置，无需驱动");
        return;
    }

    gpio_set_level(STEPPER_DIR_GPIO, DIR_LEVEL(dir) ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    EN_ON();
    vTaskDelay(pdMS_TO_TICKS(50));

    rmt_channel_handle_t chan = prv_create_rmt_channel();
    if (!chan) return;

    stepper_motor_uniform_encoder_config_t enc_cfg = {
        .resolution = STEPPER_RMT_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&enc_cfg, &encoder));

    rmt_transmit_config_t tx_cfg = {.loop_count = (int)steps_total};
    ESP_ERROR_CHECK(rmt_transmit(chan, encoder, &freq, sizeof(freq), &tx_cfg));
    prv_wait_or_sensor(chan, sensor_fn);

    rmt_disable(chan);
    rmt_del_encoder(encoder);
    rmt_del_channel(chan);
    EN_OFF();
}

// ==================== S 曲线加减速旋转 ====================

void stepper_rotate_ADS(float angle_deg, float rpm, int dir, stepper_sensor_fn_t sensor_fn)
{
    if (rpm > STEPPER_RPM_MAX) rpm = STEPPER_RPM_MAX;

    uint32_t steps_total = (uint32_t)((angle_deg / 360.0f)
                            * STEPPER_STEPS_PER_REV * STEPPER_GEAR_RATIO);
    if (steps_total < 5) return;

    uint32_t freq_max = (uint32_t)((rpm / 60.0f)
                         * STEPPER_STEPS_PER_REV * STEPPER_GEAR_RATIO);
    if (freq_max < STEPPER_ADS_FREQ_MIN) freq_max = STEPPER_ADS_FREQ_MIN;

    uint32_t freq_start = STEPPER_ADS_FREQ_START;
    if (freq_start >= freq_max) freq_start = freq_max / 2;

    if (sensor_fn && sensor_fn() == 0) {
        ESP_LOGI(STEPPER_LOG_TAG, "已在目标位置，无需驱动");
        return;
    }

    // 先裁剪加减速步数，再计算匀速步数，保证三段步数之和 == steps_total
    uint32_t accel_steps   = steps_total / STEPPER_ADS_ACCEL_DIV;
    uint32_t decel_steps   = accel_steps;
    uint32_t freq_delta    = freq_max - freq_start;
    uint32_t sample_points = (freq_delta < STEPPER_ADS_SAMPLE_CAP)
                              ? freq_delta : STEPPER_ADS_SAMPLE_CAP;
    if (accel_steps > freq_delta) accel_steps = decel_steps = freq_delta;
    uint32_t uniform_steps = (steps_total > accel_steps + decel_steps)
                              ? (steps_total - accel_steps - decel_steps) : 0;

    ESP_LOGI(STEPPER_LOG_TAG,
             "ADS angle=%.1f deg  rpm=%.1f  steps=%lu  "
             "accel=%lu  uniform=%lu  decel=%lu  "
             "f_start=%lu  f_max=%lu  sample_pts=%lu",
             angle_deg, rpm, steps_total,
             accel_steps, uniform_steps, decel_steps,
             freq_start, freq_max, sample_points);

    gpio_set_level(STEPPER_DIR_GPIO, DIR_LEVEL(dir) ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    EN_ON();
    vTaskDelay(pdMS_TO_TICKS(50));

    rmt_channel_handle_t chan = prv_create_rmt_channel();
    if (!chan) return;

    // 提前创建全部编码器，避免段间申请内存失败导致资源泄漏
    stepper_motor_curve_encoder_config_t accel_cfg = {
        .resolution    = STEPPER_RMT_RESOLUTION_HZ,
        .sample_points = sample_points,
        .start_freq_hz = freq_start,
        .end_freq_hz   = freq_max,
    };
    rmt_encoder_handle_t accel_enc = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&accel_cfg, &accel_enc));

    stepper_motor_curve_encoder_config_t decel_cfg = {
        .resolution    = STEPPER_RMT_RESOLUTION_HZ,
        .sample_points = sample_points,
        .start_freq_hz = freq_max,
        .end_freq_hz   = freq_start,
    };
    rmt_encoder_handle_t decel_enc = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&decel_cfg, &decel_enc));

    rmt_encoder_handle_t uniform_enc = NULL;
    if (uniform_steps > 0) {
        stepper_motor_uniform_encoder_config_t uniform_cfg = {
            .resolution = STEPPER_RMT_RESOLUTION_HZ,
        };
        ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_cfg, &uniform_enc));
    }

    rmt_transmit_config_t tx_cfg = {.loop_count = 0};
    bool triggered = false;

    // ---- 加速段 ----
    ESP_ERROR_CHECK(rmt_transmit(chan, accel_enc, &accel_steps, sizeof(accel_steps), &tx_cfg));
    triggered = prv_wait_or_sensor(chan, sensor_fn);

    // ---- 匀速段 ----
    if (!triggered && uniform_steps > 0) {
        uint32_t freq      = freq_max;
        tx_cfg.loop_count  = uniform_steps;
        ESP_ERROR_CHECK(rmt_transmit(chan, uniform_enc, &freq, sizeof(freq), &tx_cfg));
        tx_cfg.loop_count  = 0;
        triggered = prv_wait_or_sensor(chan, sensor_fn);
    }

    // ---- 减速段 ----
    if (!triggered) {
        ESP_ERROR_CHECK(rmt_transmit(chan, decel_enc, &decel_steps, sizeof(decel_steps), &tx_cfg));
        prv_wait_or_sensor(chan, sensor_fn);
    }

    // 统一清理（rmt_disable 先于 del，确保无正在进行的传输）
    rmt_disable(chan);
    rmt_del_encoder(accel_enc);
    if (uniform_enc) rmt_del_encoder(uniform_enc);
    rmt_del_encoder(decel_enc);
    rmt_del_channel(chan);
    EN_OFF();
}

// ==================== 梯形加减速旋转 ====================

void stepper_rotate_TRAP(float angle_deg, float rpm, int dir, stepper_sensor_fn_t sensor_fn)
{
    if (rpm > STEPPER_RPM_MAX) rpm = STEPPER_RPM_MAX;

    uint32_t steps_total = (uint32_t)((angle_deg / 360.0f)
                            * STEPPER_STEPS_PER_REV * STEPPER_GEAR_RATIO);
    if (steps_total < (uint32_t)(STEPPER_TRAP_RAMP_SEGS * 2 + 1)) return;

    uint32_t freq_max = (uint32_t)((rpm / 60.0f)
                         * STEPPER_STEPS_PER_REV * STEPPER_GEAR_RATIO);
    if (freq_max < STEPPER_ADS_FREQ_MIN) freq_max = STEPPER_ADS_FREQ_MIN;

    uint32_t freq_start = STEPPER_ADS_FREQ_START;
    if (freq_start >= freq_max) freq_start = freq_max / 2;

    if (sensor_fn && sensor_fn() == 0) {
        ESP_LOGI(STEPPER_LOG_TAG, "已在目标位置，无需驱动");
        return;
    }

    // 先裁剪，再计算匀速步数，保证三段之和 == steps_total
    uint32_t accel_steps = steps_total / STEPPER_ADS_ACCEL_DIV;
    uint32_t decel_steps = accel_steps;
    if (accel_steps + decel_steps >= steps_total) {
        accel_steps = decel_steps = steps_total / 2;
    }
    uint32_t uniform_steps = steps_total - accel_steps - decel_steps;

    // 每个斜坡段的基准步数；最后一段补齐余数
    uint32_t seg_base = accel_steps / STEPPER_TRAP_RAMP_SEGS;
    if (seg_base == 0) seg_base = 1;  // 极短距离保护

    ESP_LOGI(STEPPER_LOG_TAG,
             "TRAP angle=%.1f  rpm=%.1f  steps=%lu  "
             "accel=%lu  uniform=%lu  decel=%lu  segs=%d  "
             "f_start=%lu  f_max=%lu",
             angle_deg, rpm, steps_total,
             accel_steps, uniform_steps, decel_steps,
             STEPPER_TRAP_RAMP_SEGS, freq_start, freq_max);

    gpio_set_level(STEPPER_DIR_GPIO, DIR_LEVEL(dir) ? 1 : 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    EN_ON();
    vTaskDelay(pdMS_TO_TICKS(50));

    rmt_channel_handle_t chan = prv_create_rmt_channel();
    if (!chan) return;

    // 梯形曲线只需一个 uniform_encoder，各段重复使用
    stepper_motor_uniform_encoder_config_t enc_cfg = {
        .resolution = STEPPER_RMT_RESOLUTION_HZ,
    };
    rmt_encoder_handle_t encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&enc_cfg, &encoder));

    rmt_transmit_config_t tx_cfg = {.loop_count = 0};
    bool triggered = false;
    uint32_t freq_range = freq_max - freq_start;

    // ---- 加速段：线性升频，分 STEPPER_TRAP_RAMP_SEGS 段 ----
    // 第 i 段（0-indexed）的频率 = freq_start + freq_range * (i+1) / N
    // 使速度在加速段末尾恰好到达 freq_max
    uint32_t accel_sent = 0;
    for (int i = 0; i < STEPPER_TRAP_RAMP_SEGS && !triggered; i++) {
        uint32_t freq = freq_start
                        + (uint32_t)((float)freq_range * (i + 1) / STEPPER_TRAP_RAMP_SEGS);
        uint32_t seg  = (i == STEPPER_TRAP_RAMP_SEGS - 1)
                        ? (accel_steps - accel_sent)   // 最后一段补齐余数
                        : seg_base;
        accel_sent += seg;

        tx_cfg.loop_count = (int)seg;
        ESP_ERROR_CHECK(rmt_transmit(chan, encoder, &freq, sizeof(freq), &tx_cfg));
        triggered = prv_wait_or_sensor(chan, sensor_fn);
    }

    // ---- 匀速段 ----
    if (!triggered && uniform_steps > 0) {
        tx_cfg.loop_count = (int)uniform_steps;
        ESP_ERROR_CHECK(rmt_transmit(chan, encoder, &freq_max, sizeof(freq_max), &tx_cfg));
        triggered = prv_wait_or_sensor(chan, sensor_fn);
    }

    // ---- 减速段：线性降频，与加速段对称 ----
    // 第 i 段（0-indexed）的频率 = freq_max - freq_range * (i+1) / N
    uint32_t decel_sent = 0;
    for (int i = 0; i < STEPPER_TRAP_RAMP_SEGS && !triggered; i++) {
        uint32_t freq = freq_max
                        - (uint32_t)((float)freq_range * (i + 1) / STEPPER_TRAP_RAMP_SEGS);
        uint32_t seg  = (i == STEPPER_TRAP_RAMP_SEGS - 1)
                        ? (decel_steps - decel_sent)
                        : seg_base;
        decel_sent += seg;

        tx_cfg.loop_count = (int)seg;
        ESP_ERROR_CHECK(rmt_transmit(chan, encoder, &freq, sizeof(freq), &tx_cfg));
        triggered = prv_wait_or_sensor(chan, sensor_fn);
    }

    tx_cfg.loop_count = 0;

    rmt_disable(chan);
    rmt_del_encoder(encoder);
    rmt_del_channel(chan);
    EN_OFF();
}
