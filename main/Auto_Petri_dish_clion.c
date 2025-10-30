#include "Instrument_starts_canning.h"       //开始罐装程序
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "Servo_motor_RS485_Speed_Location.h"
#include "driver/uart.h"
#include "Little_Rotate.h"

/*
 *  git config --global --unset http.proxy      取消代理可以实现推送
 * */


void app_main(void) {
    /**
    * @CreateTime 2025/9/8
    * @Author Star-Hill
    * @brief All_init() 初始化所有的设备
    */
    All_init();

    /**
    * @CreateTime 2025/9/8
    * @Author Star-Hill
    * @Author Star-Hill
    * @brief command_handler_init()创建任务队列表
    */

     for (int i = 0; i < 3; ++i) {
         SERVO_MOTOR_read_Voltage();
     }
    command_handler_init();


    // UpDown_stepper_rotate(1220.0f, 80.0f, 1, 0);
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // Pump_Valve_run_combo(0, 1);
    // vTaskDelay(pdMS_TO_TICKS(5000));
    // Pump_Valve_run_combo(0, 0);


    /**
    * @CreateTime 2025/10/24
    * @Author Star-Hill
    * @brief 单元集成测试
    */

    /*测试转架电机*/
    // for (int i = 0; i < 8; ++i) {
    //     Big_ROTATE_stepper_rotate_US(45.0f, 3.5f, 1, false);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }

    /*测试z轴电机*/
    // for (int i = 0; i < 5; ++i) {
    //     UpDown_stepper_rotate(1220.0f, 160.0f , 1, 0);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     UpDown_stepper_rotate(1220.0f, 160.0f , 0, 0);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }

    /*读取十次电压*/
    // for (int i = 0; i < 10; ++i) {
    //     SERVO_MOTOR_read_Voltage();
    // }

    /*位置模式测试*/
    // SERVO_MOTOR_POS_Reg((int)(1500), -20000, 0, NULL);
    // SERVO_MOTOR_POS_Reg((int)(200), 600000, 0, true);
    // SERVO_MOTOR_Clear_Position(); // 位置强制清零

    // for (int i = 0; i < 10; ++i) {
    //     SERVO_MOTOR_POS_Reg((int)(1500), -50000, 0, NULL);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     SERVO_MOTOR_POS_Reg((int)(1500), 50000, 0, NULL);
    // }


}