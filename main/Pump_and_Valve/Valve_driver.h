//
// Created by DELL on 2025/8/7.
//

#ifndef AUTO_PETRI_DISH_CLION_VALVE_H
#define AUTO_PETRI_DISH_CLION_VALVE_H

#include "driver/gpio.h"

// 电磁阀 GPIO 宏定义
#define Valve_Down_GPIO    10
#define Valve_Up_GPIO    11

// 初始化函数
void valve_UpDown_driver_init(void);

// 控制函数
void Valve_Down_on(void);
void Valve_Down_off(void);

void Valve_Up_on(void);
void Valve_Up_off(void);

//测试电磁阀程序
void Valve_test(void);
#endif //AUTO_PETRI_DISH_CLION_VALVE_H
