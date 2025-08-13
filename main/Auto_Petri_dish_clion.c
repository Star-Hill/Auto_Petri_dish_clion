#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/***************    单元驱动    ***************/
#include "LeftRight.h"                      //左右伺服电机      485接口   Modbus RTU 协议
#include "UpDown.h"                         //上下步进电机
#include "Rotate.h"                         //培养皿架旋转        步进电机
#include "Little_Rotate.h"                  //培养皿旋转         步进电机
#include "sensor.h"                         //传感器
#include "HMI_control_driver.h"             //HMI 硬串口   未使用
#include "Pump_and_Valve/Pump_driver.h"     //气泵
#include "Pump_and_Valve/Valve_driver.h"    //电磁阀
#include "Peristaltic_pump.h"               //蠕动泵       485接口   Modbus RTU 协议
#include "softserial.h"                     //软串口
#include "HMI.h"
/***************    模组驱动    ***************/
#include "UpDown_motor_control.h"
#include "LeftRight_motor_control.h"
#include "Pump_Valve_driver.h"
/***************    状态驱动    ***************/
#include "Machine_initialization.h"          //初始化程序
#include "Instrument_starts_canning.h"       //开始罐装程序


void app_main(void) {
    sensor_ALL_init();                      //  初始化所有传感器
    Rotate_motor_driver_init();             //  初始化并使能柱体旋转电机
    UpDown_motor_driver_init();             //  初始化并使能升降电机
    LeftRight_init();                       //  左右电机初始化
    Little_Rotate_motor_driver_init();      //  培养皿旋转电机
    Peristaltic_pump_init();                //  蠕动泵

    Pump_driver_init();                     //  泵初始化
    valve_UpDown_driver_init();             //  电磁阀初始化
    command_handler_init();                 //  初始化命令处理系统


}