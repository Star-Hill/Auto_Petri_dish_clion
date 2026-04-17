#include "sensor.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "SENSOR";

// 初始化传感器 GPIO（所有）
void sensor_ALL_init(void) {
    gpio_config_t io_conf = {
            .pin_bit_mask =
            (1ULL << SENSOR_Down_GPIO) |
            (1ULL << SENSOR_Calibration_GPIO) |
            (1ULL << SENSOR_Entrance_GPIO) |
            (1ULL << SENSOR_RIGHT_GPIO),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "所有传感器初始化完成");
}

int sensor_Calibration_get_state(void) {
    return gpio_get_level(SENSOR_Calibration_GPIO);
}

int sensor_Entrance_get_state(void) {
    return gpio_get_level(SENSOR_Entrance_GPIO);
}

int sensor_Down_get_state(void) {
    return gpio_get_level(SENSOR_Down_GPIO);
}

int sensor_Right_get_state(void) {
    return gpio_get_level(SENSOR_RIGHT_GPIO);
}

