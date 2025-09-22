//
// Created by DELL on 2025/8/6.
//
#include "driver/rmt_tx.h"
#include "stepper_motor_encoder.h"
#include "Little_Rotate.h"

static rmt_channel_handle_t motor_chan = NULL;
static rmt_encoder_handle_t uniform_encoder = NULL;

void Little_stepper_init(void) {
    // DIR + EN 引脚
    gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << Little_ROTATE_STEPPER_DIR_GPIO) | (1ULL << Little_ROTATE_STEPPER_EN_GPIO),
    };
    gpio_config(&io_conf);

    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 1); // 默认失能       这里高电平失能

    // 创建 RMT 通道
    rmt_tx_channel_config_t tx_chan_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .gpio_num = Little_ROTATE_STEPPER_STEP_GPIO,
            .mem_block_symbols = 48,
            .resolution_hz = Little_ROTATE_STEPPER_RESOLUTION_HZ,  // 1 MHz 分辨率
            .trans_queue_depth = Little_ROTATE_STEPPER_QUEUE_DEPTH,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &motor_chan));
    ESP_ERROR_CHECK(rmt_enable(motor_chan));

    // 创建匀速编码器
    stepper_motor_uniform_encoder_config_t uniform_conf = {
            .resolution = Little_ROTATE_STEPPER_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_conf, &uniform_encoder));

    ESP_LOGI("Little_Stepping", "RMT 步进电机初始化完成");
}

void Little_stepper_rotate(float angle_deg, float rpm, int dir) {
    uint32_t steps_needed = (uint32_t)((angle_deg / 360.0f) * Little_STEPS_PER_REV);            // 计算需要的步数
    uint32_t freq_hz = (uint32_t)((rpm / 60.0f) * Little_STEPS_PER_REV);                        // 计算频率

    ESP_LOGI("Little_Stepping", "angle=%.1f deg, rpm=%.1f, steps=%lu, freq=%lu Hz",
             angle_deg, rpm, steps_needed, freq_hz);

    // 设置方向
    gpio_set_level(Little_ROTATE_STEPPER_DIR_GPIO, dir ? 1 : 0);
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 0); // 使能

    // 配置传输
    rmt_transmit_config_t tx_config = {
            .loop_count = (int)steps_needed,  // 输出 N 个脉冲
    };

    // 启动 RMT 传输
    ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_encoder,
                                 &freq_hz, sizeof(freq_hz),&tx_config));

    // 阻塞等待传输完成
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));

    // 停止并失能
    gpio_set_level(Little_ROTATE_STEPPER_EN_GPIO, 1);
}
