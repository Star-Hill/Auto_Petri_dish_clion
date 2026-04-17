//
// Created by StarHill on 2025/11/12.
//

#ifndef AUTO_PETRI_DISH_CLION_BUZZER_H
#define AUTO_PETRI_DISH_CLION_BUZZER_H

#include "driver/gpio.h"

// 定义蜂鸣器 GPIO 引脚
#define BUZZER_GPIO 8  // 可根据实际修改

void buzzer_init(void);
void buzzer_on(void);
void buzzer_off(void);
void buzzer_beep_music(void);  // 发声一段时间

#endif //AUTO_PETRI_DISH_CLION_BUZZER_H