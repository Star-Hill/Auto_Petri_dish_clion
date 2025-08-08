//
// Created by DELL on 2025/8/7.
//

#ifndef AUTO_PETRI_DISH_CLION_PUMP_H
#define AUTO_PETRI_DISH_CLION_PUMP_H

#include "driver/gpio.h"

#define Pump_Down_GPIO     3
#define Pump_Up_GPIO     9

void Pump_driver_init(void);

// 控制电磁阀1
void Pump_Down_on(void);
void Pump_Down_off(void);

// 控制电磁阀2
void Pump_Up_on(void);
void Pump_Up_off(void);

void Pump_test(void);

#endif //AUTO_PETRI_DISH_CLION_PUMP_H
