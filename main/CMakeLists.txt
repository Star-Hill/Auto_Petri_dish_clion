idf_component_register(
        SRCS                            #自己写的文件需引入
            "Auto_Petri_dish_clion.c"               #主函数

            "DM430/stepper_motor_encoder.c"         #步进电机官方 RMT 驱动
            "DM430/Little_Rotate.c"                 #小旋转
            "DM430/UpDown.c"                        #上下电机
            "DM430/Big_Rotate.c"                    #大旋转

            "RS485/Servo_motor_RS485_Speed_Location.c"      #伺服电机
            "RS485/Peristaltic_pump.c"                      #蠕动泵

            "Sensor/sensor.c"                               #传感器
            "Sensor/buzzer.c"
            "Uart/HMI_control_driver.c"                     #HMI硬串口屏幕

            "Pump_and_Valve/Pump_driver.c"                  #泵
            "Pump_and_Valve/Valve_driver.c"                 #阀
            "Pump_and_Valve/Pump_Valve_driver.c"            #泵和阀

            "Status/HMI.c"                                  #状态初始
            "Status/Machine_initialization.c"               #回零
            "Status/Instrument_starts_canning.c"            #操作




        INCLUDE_DIRS                    #路径
            "."
            "DM430"
            "Sensor"
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
            esp_driver_rmt

        PRIV_REQUIRES soft_uart
)
