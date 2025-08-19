#include "HMI.h"                            //串口屏
#include "Instrument_starts_canning.h"       //开始罐装程序

/*
 *  git config --global --unset http.proxy      取消代理可以实现推送
 * */


void app_main(void) {
    All_init();
/*    command_handler_init();                 //  初始化命令处理系统*/
    /****************   左右电机--中位置    *******************/
/*    while(1){
        LeftRight_Move_To_Position(sensor_Right_get_state, 2000, "中间位置");
        vTaskDelay(pdMS_TO_TICKS(2000));
        LeftRight_Move_To_Position(sensor_Left_get_state, -2000, "中间位置");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }*/

}
