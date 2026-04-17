//
// Created by DELL on 2025/8/7.
//

#ifndef AUTO_PETRI_DISH_CLION_SENSOR_H
#define AUTO_PETRI_DISH_CLION_SENSOR_H

#define SENSOR_Calibration_GPIO    7
#define SENSOR_Entrance_GPIO    15
#define SENSOR_Down_GPIO    16
#define SENSOR_RIGHT_GPIO    17

// 初始化传感器 GPIO
void sensor_ALL_init(void);

// 获取传感器状态：返回 1 表示无遮挡，0 表示有遮挡
int sensor_Down_get_state(void);
int sensor_Calibration_get_state(void);
int sensor_Entrance_get_state(void);
int sensor_Right_get_state(void);

#endif //AUTO_PETRI_DISH_CLION_SENSOR_H
