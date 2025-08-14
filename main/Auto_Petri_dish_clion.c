#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/***************    单元驱动    ***************/
#include "LeftRight.h"                      //左右伺服电机      485接口   Modbus RTU 协议
#include "UpDown.h"                         //上下步进电机
#include "Big_Rotate.h"                         //培养皿架旋转        步进电机
#include "Little_Rotate.h"                  //培养皿旋转         步进电机
#include "sensor.h"                         //传感器
#include "HMI_control_driver.h"             //HMI 硬串口   未使用
#include "Pump_and_Valve/Pump_driver.h"     //气泵
#include "Pump_and_Valve/Valve_driver.h"    //电磁阀
#include "Peristaltic_pump.h"               //蠕动泵       485接口   Modbus RTU 协议
#include "softserial.h"                     //软串口
#include "HMI.h"                            //串口屏
#include "Universal_stepper_motor_drive.h"  //通用步进电机
/***************    模组驱动    ***************/
#include "LeftRight_motor_control.h"
#include "Pump_Valve_driver.h"
/***************    状态驱动    ***************/
#include "Machine_initialization.h"          //初始化程序
#include "Instrument_starts_canning.h"       //开始罐装程序


void app_main(void) {
    All_init();
    command_handler_init();                 //  初始化命令处理系统
}
