//
// Created by DELL on 2025/8/11.
//

#ifndef AUTO_PETRI_DISH_CLION_INSTRUMENT_STARTS_CANNING_H
#define AUTO_PETRI_DISH_CLION_INSTRUMENT_STARTS_CANNING_H

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/***************    单元驱动    ***************/
#include "sensor.h"                 //传感器
#include "UpDown.h"                 //上下步进电机
#include "LeftRight.h"              //左右伺服电机
#include "Big_Rotate.h"             //培养皿架旋转
#include "Little_Rotate.h"          //培养皿旋转
#include "Pump_driver.h"            //气泵
#include "Valve_driver.h"           //电磁阀
#include "Peristaltic_pump.h"       //蠕动泵

#include "HMI.h"                    //串口屏
#include "HMI_control_driver.h"     //软串口底层

/***************    组合驱动    ***************/
#include "Pump_Valve_driver.h"      //电磁阀 & 气泵一体化
#include "Machine_initialization.h" //回零


void Instrument_starts_canning(int num, int volume, float gear);

int check_and_pick_plate(float gear);


void All_init(void);

void Success(float gear);

void Failure(float gear);

// 检查是否需要暂停（封装函数）
void check_pause(void);


#endif //AUTO_PETRI_DISH_CLION_INSTRUMENT_STARTS_CANNING_H
