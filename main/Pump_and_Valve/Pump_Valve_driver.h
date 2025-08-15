//
// Created by DELL on 2025/8/7.
//

#ifndef AUTO_PETRI_DISH_CLION_PUMP_VALVE_DRIVER_H
#define AUTO_PETRI_DISH_CLION_PUMP_VALVE_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/// 联动测试程序（循环测试上下组合）
void Pump_Valve_test(void);

void Pump_Valve_run_combo(bool up_down, bool on_off);

#ifdef __cplusplus
}
#endif

#endif //AUTO_PETRI_DISH_CLION_PUMP_VALVE_DRIVER_H
