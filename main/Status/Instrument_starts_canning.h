//
// Created by DELL on 2025/8/11.
//

#ifndef AUTO_PETRI_DISH_CLION_INSTRUMENT_STARTS_CANNING_H
#define AUTO_PETRI_DISH_CLION_INSTRUMENT_STARTS_CANNING_H

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sensor.h"
#include "UpDown.h"
#include "LeftRight.h"
#include "Big_Rotate.h"
#include "Pump_driver.h"
#include "Valve_driver.h"
#include "Peristaltic_pump.h"
#include "Little_Rotate.h"
#include "HMI.h"
#include "HMI_control_driver.h"

#include "Machine_initialization.h"



void Instrument_starts_canning(void);
int check_and_pick_plate(void);


void All_init(void);
void Success (void);
void Failure (void);


#endif //AUTO_PETRI_DISH_CLION_INSTRUMENT_STARTS_CANNING_H
