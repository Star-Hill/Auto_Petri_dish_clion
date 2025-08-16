#include "HMI.h"                            //串口屏
#include "Instrument_starts_canning.h"       //开始罐装程序

/*
 *  git config --global --unset http.proxy      取消代理可以实现推送
 * */


void app_main(void) {
    All_init();
    command_handler_init();                 //  初始化命令处理系统

}
