#include "Instrument_starts_canning.h"       //开始罐装程序
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "Servo_motor_RS485_Speed_Location.h"
#include "driver/uart.h"
#include "Little_Rotate.h"

/*
 *  git config --global --unset http.proxy      取消代理可以实现推送
 * */


void app_main(void) {
    /**
    * @CreateTime 2025/9/8
    * @Author Star-Hill
    * @brief All_init() 初始化所有的设备
    */
    All_init();

    //上电自动校准
    //Machine_initialization();
    //ESP_LOGW("系统日志", "上电自动校准完成，开始命令队列初始");

    /**
    * @CreateTime 2025/9/8
    * @Author Star-Hill
    * @Author Star-Hill
    * @brief command_handler_init()创建任务队列表
    */
    command_handler_init();
}