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

/// 控制上下泵+阀组合运行
/// @param is_up 是否为上泵（true为上，false为下）
/// @param duration_ms 工作时间（毫秒）
void Pump_Valve_run_combo(bool is_up, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif //AUTO_PETRI_DISH_CLION_PUMP_VALVE_DRIVER_H
