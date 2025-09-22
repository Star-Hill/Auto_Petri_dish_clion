//
// Created by DELL on 2025/8/12.
//

#ifndef AUTO_PETRI_DISH_CLION_HMI_H
#define AUTO_PETRI_DISH_CLION_HMI_H

/*
 * 官方库
 * */
#include <string.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <errno.h>

/*
 * 本地库
 * */
#include "Machine_initialization.h"

// 外部可见的暂停变量
extern volatile bool g_pause_flag;

// 队列最大命令长度
#define CMD_MAX_LEN 64

// 初始化命令处理系统（创建队列 + 启动任务）
void command_handler_init(void);

// 提供外部访问的队列（如果你需要从其他地方直接发命令）
extern QueueHandle_t command_queue;
#endif //AUTO_PETRI_DISH_CLION_HMI_H
