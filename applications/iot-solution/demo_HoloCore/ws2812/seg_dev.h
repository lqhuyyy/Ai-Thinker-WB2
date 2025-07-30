/**
 * @file seg_dev.h
 * @author Seahi-Mo (seahi-mo@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2025-07-23
 *
 * @copyright Ai-Thinker co.,ltd (c) 2025
 *
 */
#ifndef SEG_DEV_H
#define SEG_DEV_H
#include <stdint.h>
#include "ir_ws2812.h"
#include "stdbool.h"

typedef enum
{
	SEG_A = 0,
	SEG_B,
	SEG_C,
	SEG_D,
	SEG_E,
	SEG_F,
	SEG_G,
	NUM_SEGMENTS,
} segment_t;

typedef enum
{
	SEG_TEMP_AN_HUIT_TEN = 0,
	SEG_TEMP_AN_HUIT_UNIT,
	SEG_TIMER_HOUR_TEN,
	SEG_TIMER_HOUR_UNIT,
	SEG_TIMER_MINUTE_TEN,
	SEG_TIMER_MINUTE_UNIT,
} seg_index_t;
/**
 * @brief 初始化数码管
 *
 */
void seg_dev_init(void);
/**
 * @brief 显示一个数字
 *
 * @param seg_index  数码管索引
 * @param digit 数字
 * @param color 颜色
 * @param brightness 亮度
 */
void seg_dispaly_number(seg_index_t seg_index, int digit, color_t color, float brightness);
/**
 * @brief 显示温度
 *
 * @param temperature 温度值
 * @param color 颜色
 * @param brightness 亮度
 */
void seg_dispaly_tempture(int temperature, color_t color, float brightness);
/**
 * @brief 显示湿度
 *
 * @param humidity 湿度值
 * @param color 颜色
 * @param brightness 亮度
 */
void seg_dispaly_humidity(int humidity, color_t color, float brightness);
/**
 * @brief 显示时间
 *
 * @param hour 小时
 * @param minute 分钟
 * @param color 颜色
 * @param brightness 亮度
 */
void seg_display_time(int hour, int minute, color_t color, float brightness);
#endif // SEG_DEV_H
