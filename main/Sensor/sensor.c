#include "sensor.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "SENSOR";

// 初始化传感器 GPIO（所有）
void sensor_ALL_init(void) {
    gpio_config_t io_conf = {
            .pin_bit_mask =
            (1ULL << SENSOR_Up_GPIO) |
            (1ULL << SENSOR_Down_GPIO) |
            (1ULL << SENSOR_Calibration_GPIO) |
            (1ULL << SENSOR_Entrance_GPIO) |
            (1ULL << SENSOR_LEFT_GPIO) |
            (1ULL << SENSOR_MIDDLE_GPIO) |
            (1ULL << SENSOR_RIGHT_GPIO),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "所有传感器初始化完成");
}

// 获取各个传感器状态（1 表示无遮挡，0 表示有遮挡）
int sensor_Up_get_state(void) {
    return gpio_get_level(SENSOR_Up_GPIO);
}

int sensor_Down_get_state(void) {
    return gpio_get_level(SENSOR_Down_GPIO);
}

int sensor_Calibration_get_state(void) {
    return gpio_get_level(SENSOR_Calibration_GPIO);
}

int sensor_Entrance_get_state(void) {
    return gpio_get_level(SENSOR_Entrance_GPIO);
}

int sensor_Left_get_state(void) {
    return gpio_get_level(SENSOR_LEFT_GPIO);
}

int sensor_Middle_get_state(void) {
    return gpio_get_level(SENSOR_MIDDLE_GPIO);
}

int sensor_Right_get_state(void) {
    return gpio_get_level(SENSOR_RIGHT_GPIO);
}

void sensor_all_state_test(void) {
    sensor_ALL_init();                  // 实际上初始化了全部传感器

    // 读取所有传感器状态
    int state_up = sensor_Up_get_state();
    int state_down = sensor_Down_get_state();
    int state_calib = sensor_Calibration_get_state();
    int state_entrance = sensor_Entrance_get_state();
    int state_left = sensor_Left_get_state();
    int state_middle = sensor_Middle_get_state();
    int state_right = sensor_Right_get_state();

    // 打印传感器状态（0=有遮挡，1=无遮挡）
    ESP_LOGI(TAG, "传感器状态检测：");

    ESP_LOGI(TAG, "上位置传感器（上一）：%s", state_up == 0 ? "有遮挡" : "无遮挡");
    ESP_LOGI(TAG, "下位置传感器（下一）：%s", state_down == 0 ? "有遮挡" : "无遮挡");
    ESP_LOGI(TAG, "校准传感器：%s", state_calib == 0 ? "有遮挡" : "无遮挡");
    ESP_LOGI(TAG, "入口检测传感器：%s", state_entrance == 0 ? "有遮挡" : "无遮挡");
    ESP_LOGI(TAG, "左位置传感器：%s", state_left == 0 ? "有遮挡" : "无遮挡");
    ESP_LOGI(TAG, "中位置传感器：%s", state_middle == 0 ? "有遮挡" : "无遮挡");
    ESP_LOGI(TAG, "右三位置传感器：%s", state_right == 0 ? "有遮挡" : "无遮挡");

    vTaskDelay(pdMS_TO_TICKS(2000));
}

