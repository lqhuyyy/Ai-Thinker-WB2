/**
 * @file buttom.h
 * @author Seahi-Mo (seahi-mo@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2025-09-23
 *
 * @copyright Ai-Thinker co.,ltd (c) 2025
 *
 */
#ifndef __BUTTOM_H__
#define __BUTTOM_H__
#include <bl_gpio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#define BUTTOM_CFG_PIN 5
#define BUTTOM_SLEEP_AND_WAKEUP_PIN 1

// 按键事件类型
typedef enum
{
	KEY_EVENT_NONE,			// 无事件
	KEY_EVENT_CLICK,		// 单击事件
	KEY_EVENT_DOUBLE_CLICK, // 双击事件
	KEY_EVENT_LONG_PRESS	// 长按事件
} KeyEventType;

// 按键状态
typedef enum
{
	KEY_STATE_IDLE,		   // 空闲状态
	KEY_STATE_PRESSED,	   // 按下状态
	KEY_STATE_RELEASED,	   // 释放状态
	KEY_STATE_LONG_PRESSED // 长按状态
} KeyState;

// 按键配置结构体
typedef struct
{
	// 按键引脚读取函数，用户需要实现
	// 返回值：1表示按键按下，0表示按键释放
	uint8_t (*read_key)(void);

	// 长按判定时间(毫秒)
	uint32_t long_press_time;

	// 双击间隔时间(毫秒)
	uint32_t double_click_interval;

	// 按键消抖时间(毫秒)
	uint32_t debounce_time;
} KeyConfig;

// 按键控制结构体
typedef struct
{
	KeyConfig config;
	KeyState state;
	KeyEventType event;

	// 时间戳记录
	TickType_t press_time;
	TickType_t release_time;

	// 单击计数，用于检测双击
	uint8_t click_count;

	// FreeRTOS定时器
	TimerHandle_t debounce_timer;
	TimerHandle_t long_press_timer;
	TimerHandle_t double_click_timer;

	// 事件通知句柄
	TaskHandle_t notify_task;
} KeyHandle;

// 初始化按键
KeyHandle *key_init(KeyConfig *config, TaskHandle_t notify_task);

// 按键处理任务
void key_task(void *pvParameters);

// 获取当前按键事件
KeyEventType key_get_event(KeyHandle *handle);

// 清除按键事件
void key_clear_event(KeyHandle *handle);
#endif // !__BUTTOM_H__