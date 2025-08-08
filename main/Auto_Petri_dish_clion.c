#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "UpDown.h"
#include "Rotate.h"
#include "Little_Rotate.h"
#include "sensor.h"
#include "UpDown_motor_control.h"
#include "Pump_and_Valve/Pump_driver.h"
#include "Pump_and_Valve/Valve_driver.h"
#include "Pump_Valve_driver.h"
#include "HMI_control_driver.h"


void app_main(void) {
    HMI_control_driver_test();
}


