#include "Instrument_starts_canning.h" //开始罐装程序
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "Servo_motor_RS485_Speed_Location.h"
#include "driver/uart.h"
#include "Little_Rotate.h"
/*
 *  git config --global --unset http.proxy      取消代理可以实现推送
 * */

void app_main(void)
{
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
    command_handler_init();

    /**
     * @CreateTime 2025/10/24
     * @Author Star-Hill
     * @brief 单元集成测试
     */
    /*测试x轴电机*/
    // SERVO_MOTOR_POS_Reg((int)(1000), -50000, 0, NULL);
    // SERVO_MOTOR_POS_Reg((int)(100), 600000, 0, true);
    // SERVO_MOTOR_Clear_Position(); // 位置强制清零
    // SERVO_MOTOR_POS_Reg((int)(200), -13000, 0, NULL); // 回一点校准

    /*测试大转架电机*/
    // Big_ROTATE_stepper_rotate_US(45.0f, 3.5f, 1, false);
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // Big_ROTATE_stepper_rotate_US(45.0f, 3.5f, 0, false);

    /*测试z轴电机*/
    // for (int i = 0; i < 10; ++i) {
    //     UpDown_stepper_rotate(1220.0f, 500.0f , 1, 0);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     UpDown_stepper_rotate(1220.0f, 500.0f , 0, 0);
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }

    /*********************  测试一根485总线  开始  *********************/
    // /*读取x轴电机五次电压*/
    // for (int i = 0; i < 5; ++i) {
    //     SERVO_MOTOR_read_Voltage();
    // }

    // //  左右电机初始化
    // SERVO_MOTOR_Clear_Position(); // 位置强制清零
    // /*蠕动泵命令测试*/
    // Peristaltic_pump_Run(200, 5);

    // /*读取x轴电机五次电压*/
    // for (int i = 0; i < 5; ++i) {
    //     SERVO_MOTOR_read_Voltage();
    // }
    // SERVO_MOTOR_Clear_Position(); // 位置强制清零
    /*********************  测试一根485总线  结束  *********************/

    /*小旋转电机*/
    // Little_stepper_rotate_US(720.0f, 30.0f, 0); //两圈
}
