//
// Created by DELL on 2025/8/7.
//

#ifndef AUTO_PETRI_DISH_CLION_SENSOR_H
#define AUTO_PETRI_DISH_CLION_SENSOR_H

#define SENSOR_Calibration_GPIO    8
#define SENSOR_Entrance_GPIO    18

#define SENSOR_Up_GPIO      19
#define SENSOR_Down_GPIO    20
#define SENSOR_LEFT_ONE_GPIO    15
#define SENSOR_LEFT_TWO_GPIO    16
#define SENSOR_LEFT_THREE_GPIO    17

// 初始化传感器 GPIO
void sensor_upDown_init();

// 获取传感器状态：返回 1 表示无遮挡，0 表示有遮挡
int sensor_Up_get_state(void);
int sensor_Down_get_state(void);
int sensor_Calibration_get_state(void);
int sensor_Entrance_get_state(void);
int sensor_Left_One_get_state(void);
int sensor_Left_Two_get_state(void);
int sensor_Left_Three_get_state(void);

//传感器测试程序
void sensor_all_state_test();


#endif //AUTO_PETRI_DISH_CLION_SENSOR_H
