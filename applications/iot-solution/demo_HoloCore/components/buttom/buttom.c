/**
 * @file buttom.c
 * @author Seahi-Mo (seahi-mo@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2025-09-23
 *
 * @copyright Ai-Thinker co.,ltd (c) 2025
 *
 */
#include <stdio.h>
#include <string.h>
#include "buttom.h"
#include <blog.h>
static TaskHandle_t event_cfg_key_task_handle = NULL;
static TaskHandle_t event_wakeup_handle = NULL;
static void debounce_timer_callback(TimerHandle_t xTimer);
static void long_press_timer_callback(TimerHandle_t xTimer);
static void double_click_timer_callback(TimerHandle_t xTimer);
/**
 * @brief 按键消抖定时器回调
 *
 * @param config
 * @param notify_task
 * @return KeyHandle*
 */
static KeyHandle *key_init(KeyConfig *config, TaskHandle_t notify_task)
{
	// 分配内存
	KeyHandle *handle = (KeyHandle *)pvPortMalloc(sizeof(KeyHandle));
	if (handle == NULL)
	{
		return NULL;
	}

	// 初始化配置
	handle->config = *config;
	handle->state = KEY_STATE_IDLE;
	handle->event = KEY_EVENT_NONE;
	handle->click_count = 0;
	handle->notify_task = notify_task;

	// 创建定时器
	handle->debounce_timer = xTimerCreate(
		"DebounceTimer",
		pdMS_TO_TICKS(config->debounce_time),
		pdFALSE, // 一次性定时器
		(void *)handle,
		debounce_timer_callback);

	handle->long_press_timer = xTimerCreate(
		"LongPressTimer",
		pdMS_TO_TICKS(config->long_press_time),
		pdFALSE, // 一次性定时器
		(void *)handle,
		long_press_timer_callback);

	handle->double_click_timer = xTimerCreate(
		"DoubleClickTimer",
		pdMS_TO_TICKS(config->double_click_interval),
		pdFALSE, // 一次性定时器
		(void *)handle,
		double_click_timer_callback);

	if (handle->debounce_timer == NULL ||
		handle->long_press_timer == NULL ||
		handle->double_click_timer == NULL)
	{
		// 清理已创建的资源
		if (handle->debounce_timer)
			vTimerDelete(handle->debounce_timer);
		if (handle->long_press_timer)
			vTimerDelete(handle->long_press_timer);
		vPortFree(handle);
		return NULL;
	}

	return handle;
}
// 读取按键状态
static uint8_t bottom_read_cfg(void)
{
	// 假设按键按下时为低电平，这里做了反转，返回1表示按下
	return bl_gpio_input_get_value(BUTTOM_CFG_PIN) == 0 ? 1 : 0;
}
static uint8_t bottom_read_sleep_and_wakeup(void)
{
	// 假设按键按下时为低电平，这里做了反转，返回1表示按下
	return bl_gpio_input_get_value(BUTTOM_SLEEP_AND_WAKEUP_PIN) == 0 ? 1 : 0;
}

static void event_cfg_key_handler_task(void *pvParameters)
{
	KeyHandle *key_handle = (KeyHandle *)pvParameters;
	KeyEventType event;

	for (;;)
	{
		// 等待按键事件通知
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		// 获取并处理事件
		event = key_get_event(key_handle);
		switch (event)
		{
		case KEY_EVENT_CLICK:
			// 处理单击事件
			printf("Key click detected!\r\n");
			break;

		case KEY_EVENT_DOUBLE_CLICK:
			// 处理双击事件
			printf("Key double click detected!\r\n");
			break;

		case KEY_EVENT_LONG_PRESS:
			// 处理长按事件
			printf("Key long press detected!\r\n");
			break;

		default:
			break;
		}
		// 清除事件
		key_clear_event(key_handle);
	}
}
static void event_wakeup_handler_task(void *pvParameters)
{
}
void bottom_init(void)
{
	bl_gpio_enable_input(BUTTOM_CFG_PIN, 0, 0);
	bl_gpio_enable_input(BUTTOM_SLEEP_AND_WAKEUP_PIN, 0, 0);
	KeyConfig cfg_key_config = {
		.read_key = bottom_read_cfg,
		.long_press_time = 3000,	  // 1秒判定为长按
		.double_click_interval = 300, // 300毫秒内双击有效
		.debounce_time = 50			  // 50毫秒消抖
	};
	KeyConfig bottom_read_sleep_and_wakeup = {
		.read_key = bottom_read_cfg,
		.long_press_time = 3000,	  // 1秒判定为长按
		.double_click_interval = 300, // 300毫秒内双击有效
		.debounce_time = 50			  // 50毫秒消抖
	};
	xTaskCreate(event_cfg_key_handler_task, "event_cfg_key_handler_task", 1024, NULL, 5, &event_cfg_key_task_handle);
	xTaskCreate(event_wakeup_handler_task, "event_wakeup_handler_task", 1024, NULL, 6, &event_wakeup_handle);
	KeyHandle *fcg_key_handle = key_init(&cfg_key_config, event_cfg_key_task_handle);
	if (fcg_key_handle == NULL)
	{
		// 初始化失败处理
		while (1)
			;
	}
	KeyHandle *wakeup_handle = key_init(&bottom_read_sleep_and_wakeup, event_wakeup_handle);
	if (wakeup_handle == NULL)
	{
		// 初始化失败处理
		while (1)
			;
	}
}
// 长按定时器回调
static void long_press_timer_callback(TimerHandle_t xTimer)
{
	KeyHandle *handle = (KeyHandle *)pvTimerGetTimerID(xTimer);

	// 检查按键是否仍处于按下状态
	if (handle->state == KEY_STATE_PRESSED && handle->config.read_key() == 1)
	{
		handle->state = KEY_STATE_LONG_PRESSED;
		handle->event = KEY_EVENT_LONG_PRESS;

		// 通知任务有事件发生
		if (handle->notify_task)
		{
			xTaskNotifyGive(handle->notify_task);
		}
	}
}

// 双击检测定时器回调
static void double_click_timer_callback(TimerHandle_t xTimer)
{
	KeyHandle *handle = (KeyHandle *)pvTimerGetTimerID(xTimer);

	if (handle->click_count == 1)
	{
		// 只有一次点击，判定为单击事件
		handle->event = KEY_EVENT_CLICK;

		// 通知任务有事件发生
		if (handle->notify_task)
		{
			xTaskNotifyGive(handle->notify_task);
		}
	}

	handle->click_count = 0;
	handle->state = KEY_STATE_IDLE;
}

// 按键处理任务
void key_task(void *pvParameters)
{
	KeyHandle *handle = (KeyHandle *)pvParameters;
	uint8_t current_state, last_state = 0;

	for (;;)
	{
		current_state = handle->config.read_key();

		// 检测按键状态变化
		if (current_state != last_state)
		{
			if (current_state == 1)
			{ // 按键按下
				// 启动消抖定时器
				xTimerStart(handle->debounce_timer, 0);
			}
			else
			{ // 按键释放
				// 停止长按定时器
				xTimerStop(handle->long_press_timer, 0);

				if (handle->state == KEY_STATE_PRESSED)
				{
					// 按键被释放，且未触发长按
					handle->state = KEY_STATE_RELEASED;
					handle->release_time = xTaskGetTickCount();
					handle->click_count++;

					if (handle->click_count == 1)
					{
						// 第一次点击，启动双击检测定时器
						xTimerStart(handle->double_click_timer, 0);
					}
					else if (handle->click_count == 2)
					{
						// 第二次点击，判定为双击事件
						xTimerStop(handle->double_click_timer, 0);
						handle->event = KEY_EVENT_DOUBLE_CLICK;

						// 通知任务有事件发生
						if (handle->notify_task)
						{
							xTaskNotifyGive(handle->notify_task);
						}

						handle->click_count = 0;
						handle->state = KEY_STATE_IDLE;
					}
				}
				else if (handle->state == KEY_STATE_LONG_PRESSED)
				{
					// 长按后释放，回到空闲状态
					handle->state = KEY_STATE_IDLE;
				}
			}
		}

		last_state = current_state;
		vTaskDelay(pdMS_TO_TICKS(5)); // 5ms轮询一次
	}
}

// 获取当前按键事件
KeyEventType key_get_event(KeyHandle *handle)
{
	KeyEventType event = handle->event;
	return event;
}

// 清除按键事件
void key_clear_event(KeyHandle *handle)
{
	handle->event = KEY_EVENT_NONE;
}