idf_component_register(
        SRCS                            #自己写的文件需引入
            "Auto_Petri_dish_clion.c"
            "DM430/UpDown.c"
            "DM430/Big_Rotate.c"
            "DM430/Little_Rotate.c"
            "DM430/Universal_stepper_motor_drive.c"

            "RS485/LeftRight.c"
            "RS485/Peristaltic_pump.c"

            "Sensor/sensor.c"
            "Uart/HMI_control_driver.c"
            "Uart/softserial.c"

            "Pump_and_Valve/Pump_driver.c"
            "Pump_and_Valve/Valve_driver.c"
            "Pump_and_Valve/Pump_Valve_driver.c"

            "Sensor_motor_control/LeftRight/LeftRight_motor_control.c"

            "Status/HMI.c"
            "Status/Machine_initialization.c"
            "Status/Instrument_starts_canning.c"


        INCLUDE_DIRS                    #路径
            "."
            "DM430"
            "Sensor"
            "Sensor_motor_control/LeftRight"
            "RS485"
            "Pump_and_Valve"
            "Uart"
            "Status"

        REQUIRES                        #组件中找不到的需要显示说明
            esp_driver_ledc
            esp_driver_gpio
            esp_timer
            esp_driver_gptimer
            esp_driver_uart
            nvs_flash

)
