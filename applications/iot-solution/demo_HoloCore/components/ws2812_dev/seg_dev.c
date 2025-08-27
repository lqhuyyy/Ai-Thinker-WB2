/**
 * @file seg_dev.c
 * @author Seahi-Mo (seahi-mo@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2025-07-23
 *
 * @copyright Ai-Thinker co.,ltd (c) 2025
 *
 */
#include "seg_dev.h"
#include "blog.h"
#include <stdint.h>

#define ON true
#define OFF false

// 数码管显示温度定义
typedef struct
{
	/* 十位断码*/
	uint8_t tens[7]; // 段编码 (bit0=A, bit1=B, ..., bit6=G)
	/*个位断码*/
	uint8_t units[7]; // 段编码 (bit0=A, bit1=B, ..., bit6=G)
	/* 温度单位断码*/
	uint8_t t_unit_id;
	/* 湿度单位断码*/
	uint8_t h_unit_id;
} tempAndHumidity_t;

// 数码管显示时间定义
typedef struct
{

	// 小时断码定义
	uint8_t seg_hour_tens[7];  // 十位数
	uint8_t seg_hour_units[7]; // 个位数
	// 分隔符断码
	uint8_t seg_units;
	// 分钟断码定义
	uint8_t seg_minute_tens[7];	 // 十位数
	uint8_t seg_minute_units[7]; // 个位数
} seg_time_t;

static ws2812_strip_t ws2812_strip = {
	.led_count = 46,
	.brightness = 0.05,
};

static bool tempture_unit_icon_enable = false;
static bool humidity_unit_icon_enable = false;
// 数码管显示设备定义
typedef struct
{
	// 定义温湿度数字显示
	tempAndHumidity_t tempAndHumidity;
	// 定义时间数字显示
	seg_time_t seg_time;
	bool isUsed; // 是否被使用
} seg_dev_t;
/**
 * @brief 映射ws2812 ID 到数码管
 *
 */
static seg_dev_t seg_dev = {
	.tempAndHumidity = {
		.tens = {2, 3, 6, 5, 0, 1, 4},
		.units = {9, 10, 13, 12, 7, 8, 11},
		.t_unit_id = 15,
		.h_unit_id = 14,
	},
	.seg_time = {
		.seg_hour_tens = {19, 20, 23, 22, 17, 18, 21},
		.seg_hour_units = {26, 27, 30, 29, 24, 25, 28},
		.seg_units = 31,
		.seg_minute_tens = {34, 35, 38, 37, 32, 33, 36},
		.seg_minute_units = {41, 42, 45, 44, 39, 40, 43},
	},
};
// 数码管显示颜色定义
const uint8_t digitSegments[] = {
	0b0111111, // 0: 显示0时，A~F段亮，G段灭
	0b0000110, // 1: 显示1时，B、C段亮，其他段灭
	0b1011011, // 2
	0b1001111, // 3
	0b1100110, // 4
	0b1101101, // 5
	0b1111101, // 6
	0b0000111, // 7
	0b1111111, // 8
	0b1101111, // 9
	0b1110111, // A
	0b1111100, // B
	0b0111001, // C
	0b1011110, // D
	0b1111001, // E
	0b1110001  // F
};

static uint8_t find_no_used_seg(uint8_t *no_user_seg)
{
	if (no_user_seg == NULL)
		return 0;
	bool used[47] = {false}; // 标记0~46是否被使用
	// 标记tempAndHumidity部分使用的值
	for (int i = 0; i < 7; i++)
		used[seg_dev.tempAndHumidity.tens[i]] = true;
	for (int i = 0; i < 7; i++)
		used[seg_dev.tempAndHumidity.units[i]] = true;
	used[seg_dev.tempAndHumidity.t_unit_id] = true;
	used[seg_dev.tempAndHumidity.h_unit_id] = true;

	// 标记seg_time部分使用的值
	for (int i = 0; i < 7; i++)
		used[seg_dev.seg_time.seg_hour_tens[i]] = true;
	for (int i = 0; i < 7; i++)
		used[seg_dev.seg_time.seg_hour_units[i]] = true;
	used[seg_dev.seg_time.seg_units] = true;
	for (int i = 0; i < 7; i++)
		used[seg_dev.seg_time.seg_minute_tens[i]] = true;
	for (int i = 0; i < 7; i++)
		used[seg_dev.seg_time.seg_minute_units[i]] = true;
	uint8_t no_user_seg_count = 0;
	for (int i = 0; i < ws2812_get_led_count(); i++)
	{
		if (!used[i])
			no_user_seg[no_user_seg_count++] = i;
	}
	return no_user_seg_count;
}

static void seg_disable_all(void)
{
	ws2812_set_all_pixels_color(0, 0, 0, 0.0);
}
void seg_dev_init(void)
{

	ws2812_init(&ws2812_strip); // 初始化ws2812
	/*初始化数码管，关闭不使用的灯珠*/
	uint8_t no_user_seg[47];
	uint8_t no_user_seg_num = find_no_used_seg(no_user_seg);
	if (no_user_seg_num > 0)
	{
		for (int i = 0; i < no_user_seg_num; i++)
		{
			ws2812_set_pixel_brightness(no_user_seg[i], 0);
			ws2812_set_pixel_color(no_user_seg[i], 0, 0, 0);
		}
	}
}

// 设置单个数码管显示特定数字
void seg_dispaly_number(seg_index_t seg_index, int digit, color_t color, float brightness)
{
	if (digit < 0 || digit > 9)
	{
		blog_error("错误：数字必须在0-9之间");
		return;
	}

	uint8_t segments = digitSegments[digit];
	for (int i = 0; i < NUM_SEGMENTS; i++)
	{
		int is_segment_on = (segments >> i) & 1;

		switch (seg_index)
		{
		case SEG_TEMP_AN_HUIT_TEN:
			ws2812_set_pixel_color(seg_dev.tempAndHumidity.tens[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.tempAndHumidity.tens[i], is_segment_on ? brightness : 0);
			break;
		case SEG_TEMP_AN_HUIT_UNIT:
			ws2812_set_pixel_color(seg_dev.tempAndHumidity.units[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.tempAndHumidity.units[i], is_segment_on ? brightness : 0);
			break;
		case SEG_TIMER_HOUR_TEN:
			ws2812_set_pixel_color(seg_dev.seg_time.seg_hour_tens[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.seg_time.seg_hour_tens[i], is_segment_on ? brightness : 0);
			break;
		case SEG_TIMER_HOUR_UNIT:
			ws2812_set_pixel_color(seg_dev.seg_time.seg_hour_units[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.seg_time.seg_hour_units[i], is_segment_on ? brightness : 0);
			break;
		case SEG_TIMER_MINUTE_TEN:
			ws2812_set_pixel_color(seg_dev.seg_time.seg_minute_tens[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.seg_time.seg_minute_tens[i], is_segment_on ? brightness : 0);
			break;
		case SEG_TIMER_MINUTE_UNIT:
			ws2812_set_pixel_color(seg_dev.seg_time.seg_minute_units[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.seg_time.seg_minute_units[i], is_segment_on ? brightness : 0);
			break;
		default:
			break;
		}
	}
	ws2812_show_leds();
}

static void seg_dispaly_tempture_unit(bool enable, color_t color, float brightness)
{
	ws2812_set_pixel_color(seg_dev.tempAndHumidity.t_unit_id, color.r, color.g, color.b);
	ws2812_set_pixel_brightness(seg_dev.tempAndHumidity.t_unit_id, enable ? brightness : 0);
	tempture_unit_icon_enable = enable;
}

static void seg_dispaly_humidity_unit(bool enable, color_t color, float brightness)
{
	ws2812_set_pixel_color(seg_dev.tempAndHumidity.h_unit_id, color.r, color.g, color.b);
	ws2812_set_pixel_brightness(seg_dev.tempAndHumidity.h_unit_id, enable ? brightness : 0);
	humidity_unit_icon_enable = enable;
}
void seg_dispaly_tempture(int temperature, color_t color, float brightness)
{
	seg_dispaly_number(SEG_TEMP_AN_HUIT_TEN, temperature / 10, color, brightness);
	seg_dispaly_number(SEG_TEMP_AN_HUIT_UNIT, temperature % 10, color, brightness);
	seg_dispaly_humidity_unit(OFF, color, brightness);
	seg_dispaly_tempture_unit(ON, color, brightness);
}

void seg_dispaly_humidity(int humidity, color_t color, float brightness)
{
	seg_dispaly_number(SEG_TEMP_AN_HUIT_TEN, humidity / 10, color, brightness);
	seg_dispaly_number(SEG_TEMP_AN_HUIT_UNIT, humidity % 10, color, brightness);
	seg_dispaly_humidity_unit(ON, color, brightness);
	seg_dispaly_tempture_unit(OFF, color, brightness);
}

void seg_display_time(int hour, int minute, color_t color, float brightness)
{
	seg_dispaly_number(SEG_TIMER_HOUR_TEN, hour / 10, color, brightness);
	seg_dispaly_number(SEG_TIMER_HOUR_UNIT, hour % 10, color, brightness);
	seg_dispaly_number(SEG_TIMER_MINUTE_TEN, minute / 10, color, brightness);
	seg_dispaly_number(SEG_TIMER_MINUTE_UNIT, minute % 10, color, brightness);
	ws2812_set_pixel_color(seg_dev.seg_time.seg_units, color.r, color.g, color.b);
	ws2812_set_pixel_brightness(seg_dev.seg_time.seg_units, brightness);
}