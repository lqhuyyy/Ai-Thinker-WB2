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
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#define ON true
#define OFF false

static TimerHandle_t seg_timer = NULL;
static TimerHandle_t seg_timer_1 = NULL;
static TimerHandle_t seg_timer_dot = NULL;
static color_t seg_timer_dot_color = {0xff, 0, 0};
static float seg_timer_dot_brightness_max = 0.5;
static float seg_timer_dot_brightness = 0.1;
static float direction = 0.05;

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
	.led_count =62,
	.brightness = 0.5,
	.pin = 12,
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
/**
 * @brief 时间的 十位 数码管的断码颜色配置
 *
 */
static color_t time_ten[7] = {
	{202, 64, 188},
	{202, 75, 135},
	{202, 98, 7},
	{202, 98, 0},
	{202, 98, 7},
	{202, 75, 135},
	{192, 83, 92},
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

	ws2812_init(&ws2812_strip);
	// ws2812_set_all_pixels_color(0, 0, 0, 0.0);
	// 初始化ws2812
	// /*初始化数码管，关闭不使用的灯珠*/
	// seg_disable_all();
	uint8_t no_user_seg[ws2812_strip.led_count];
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
	seg_timer_dot_brightness_max = brightness;
	for (int i = 0; i < NUM_SEGMENTS; i++)
	{
		int is_segment_on = (segments >> i) & 1;

		switch (seg_index)
		{
		case SEG_TEMP_AN_HUIT_TEN:
			ws2812_set_pixel_color(seg_dev.tempAndHumidity.tens[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.tempAndHumidity.tens[i], is_segment_on ? seg_timer_dot_brightness_max : 0);
			break;
		case SEG_TEMP_AN_HUIT_UNIT:
			ws2812_set_pixel_color(seg_dev.tempAndHumidity.units[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.tempAndHumidity.units[i], is_segment_on ? seg_timer_dot_brightness_max : 0);
			break;
		case SEG_TIMER_HOUR_TEN:
			ws2812_set_pixel_color(seg_dev.seg_time.seg_hour_tens[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.seg_time.seg_hour_tens[i], is_segment_on ? seg_timer_dot_brightness_max : 0);
			break;
		case SEG_TIMER_HOUR_UNIT:
			ws2812_set_pixel_color(seg_dev.seg_time.seg_hour_units[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.seg_time.seg_hour_units[i], is_segment_on ? seg_timer_dot_brightness_max : 0);
			break;
		case SEG_TIMER_MINUTE_TEN:
			ws2812_set_pixel_color(seg_dev.seg_time.seg_minute_tens[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.seg_time.seg_minute_tens[i], is_segment_on ? seg_timer_dot_brightness_max : 0);
			break;
		case SEG_TIMER_MINUTE_UNIT:
			ws2812_set_pixel_color(seg_dev.seg_time.seg_minute_units[i], color.r, color.g, color.b);
			ws2812_set_pixel_brightness(seg_dev.seg_time.seg_minute_units[i], is_segment_on ? seg_timer_dot_brightness_max : 0);
			break;
		default:
			break;
		}
	}
}

static void seg_dispaly_tempture_unit(bool enable, color_t color, float brightness)
{
	seg_timer_dot_brightness_max = brightness;
	ws2812_set_pixel_color(seg_dev.tempAndHumidity.t_unit_id, color.r, color.g, color.b);
	ws2812_set_pixel_brightness(seg_dev.tempAndHumidity.t_unit_id, enable ? seg_timer_dot_brightness_max : 0);
	tempture_unit_icon_enable = enable;
}

static void seg_dispaly_humidity_unit(bool enable, color_t color, float brightness)
{
	seg_timer_dot_brightness_max = brightness;
	ws2812_set_pixel_color(seg_dev.tempAndHumidity.h_unit_id, color.r, color.g, color.b);
	ws2812_set_pixel_brightness(seg_dev.tempAndHumidity.h_unit_id, enable ? seg_timer_dot_brightness_max : 0);
	humidity_unit_icon_enable = enable;
}
void seg_dispaly_tempture(int temperature, color_t color, float brightness)
{
	seg_timer_dot_brightness_max = brightness;
	seg_dispaly_number(SEG_TEMP_AN_HUIT_TEN, temperature / 10, color, brightness);
	seg_dispaly_number(SEG_TEMP_AN_HUIT_UNIT, temperature % 10, color, brightness);
	seg_dispaly_humidity_unit(OFF, color, brightness);
	seg_dispaly_tempture_unit(ON, color, brightness);
	ws2812_show_leds();
}

void seg_dispaly_humidity(int humidity, color_t color, float brightness)
{
	seg_timer_dot_brightness_max = brightness;
	seg_dispaly_number(SEG_TEMP_AN_HUIT_TEN, humidity / 10, color, brightness);
	seg_dispaly_number(SEG_TEMP_AN_HUIT_UNIT, humidity % 10, color, brightness);
	seg_dispaly_humidity_unit(ON, color, brightness);
	seg_dispaly_tempture_unit(OFF, color, brightness);
	ws2812_show_leds();
}

void seg_display_time(int hour, int minute, color_t color, float brightness)
{
	seg_timer_dot_brightness_max = brightness;
	seg_dispaly_number(SEG_TIMER_HOUR_TEN, hour / 10, color, seg_timer_dot_brightness_max);
	seg_dispaly_number(SEG_TIMER_HOUR_UNIT, hour % 10, color, seg_timer_dot_brightness_max);
	seg_dispaly_number(SEG_TIMER_MINUTE_TEN, minute / 10, color, seg_timer_dot_brightness_max);
	seg_dispaly_number(SEG_TIMER_MINUTE_UNIT, minute % 10, color, seg_timer_dot_brightness_max);

	ws2812_set_pixel_color(seg_dev.seg_time.seg_units, color.r, color.g, color.b);
	ws2812_set_pixel_brightness(seg_dev.seg_time.seg_units, seg_timer_dot_brightness_max);
	ws2812_show_leds();
	vTaskDelay(pdMS_TO_TICKS(200));
	ws2812_set_pixel_color(seg_dev.seg_time.seg_units, color.r, color.g, color.b);
	ws2812_set_pixel_brightness(seg_dev.seg_time.seg_units, 0);
	ws2812_show_leds();
	vTaskDelay(pdMS_TO_TICKS(200));
	ws2812_set_pixel_color(seg_dev.seg_time.seg_units, color.r, color.g, color.b);
	ws2812_set_pixel_brightness(seg_dev.seg_time.seg_units, seg_timer_dot_brightness_max);
	ws2812_show_leds();
}

void seg_display_time_ex_color_mode(int hour, int minute, int color_mode, float brightness)
{

	seg_timer_dot_brightness_max = brightness;

	uint8_t segments = digitSegments[hour / 10];
	seg_timer_dot_brightness_max = brightness;
	// 设置小时的十位
	for (int i = 0; i < NUM_SEGMENTS; i++)
	{
		int is_segment_on = (segments >> i) & 1;
		ws2812_set_pixel_color(seg_dev.seg_time.seg_hour_tens[i], time_ten[i].r, time_ten[i].g, time_ten[i].b);
		ws2812_set_pixel_brightness(seg_dev.seg_time.seg_hour_tens[i], is_segment_on ? seg_timer_dot_brightness_max : 0);
	}
	// 设置小时的个位
	segments = digitSegments[hour % 10];
	for (int i = 0; i < NUM_SEGMENTS; i++)
	{
		int is_segment_on = (segments >> i) & 1;
		ws2812_set_pixel_color(seg_dev.seg_time.seg_hour_units[i], time_ten[i].r, time_ten[i].g, time_ten[i].b);
		ws2812_set_pixel_brightness(seg_dev.seg_time.seg_hour_units[i], is_segment_on ? seg_timer_dot_brightness_max : 0);
	}
	// 设置分钟的十位
	segments = digitSegments[minute / 10];
	for (int i = 0; i < NUM_SEGMENTS; i++)
	{
		int is_segment_on = (segments >> i) & 1;
		ws2812_set_pixel_color(seg_dev.seg_time.seg_minute_tens[i], time_ten[i].r, time_ten[i].g, time_ten[i].b);
		ws2812_set_pixel_brightness(seg_dev.seg_time.seg_minute_tens[i], is_segment_on ? seg_timer_dot_brightness_max : 0);
	}
	// 设置分钟的个位
	segments = digitSegments[minute % 10];
	for (int i = 0; i < NUM_SEGMENTS; i++)
	{
		int is_segment_on = (segments >> i) & 1;
		ws2812_set_pixel_color(seg_dev.seg_time.seg_minute_units[i], time_ten[i].r, time_ten[i].g, time_ten[i].b);
		ws2812_set_pixel_brightness(seg_dev.seg_time.seg_minute_units[i], is_segment_on ? seg_timer_dot_brightness_max : 0);
	}
	ws2812_show_leds();
	vTaskDelay(pdMS_TO_TICKS(200));
	ws2812_set_pixel_color(seg_dev.seg_time.seg_units, time_ten[0].r, time_ten[0].g, time_ten[0].b);
	ws2812_set_pixel_brightness(seg_dev.seg_time.seg_units, 0);
	ws2812_show_leds();
	vTaskDelay(pdMS_TO_TICKS(200));
	ws2812_set_pixel_color(seg_dev.seg_time.seg_units, time_ten[0].r, time_ten[0].g, time_ten[0].b);
	ws2812_set_pixel_brightness(seg_dev.seg_time.seg_units, seg_timer_dot_brightness_max);
	ws2812_show_leds();
}
/**
 * @brief 关闭数码管
 *
 * @param arg
 */
static void seg_display_timer_handle(TimerHandle_t Timer)
{
	loading_t status = (loading_t)pvTimerGetTimerID(Timer);

	static int blink_cont = 0;

	switch (status)
	{
	case SEG_LOADING_BLUFI_CONFIG:
		ws2812_set_pixel_color(16, 0X00, 0x00, 0XFF);
		break;
	case SEG_LOADING_WIFI_DISCONNECT:
		// 进行红色闪烁，闪烁周期为500ms
		ws2812_set_pixel_color(16, 0xFF, 0x00, 0x00);
		break;
	default:
		break;
	}

	ws2812_set_pixel_brightness(16, blink_cont % 2 ? seg_timer_dot_brightness_max : 0);
	blink_cont++;
	if (blink_cont >= 10)
	{
		blink_cont = 0;
	}
	ws2812_show_leds();
}
void seg_display_loading(loading_t status)
{
	// 状态显示

	blog_info("seg_display_loading %d", status);

	switch (status)
	{
	case SEG_LOADING_BLUFI_CONFIG:
		if (seg_timer == NULL)
			seg_timer = xTimerCreate("seg_timer", pdMS_TO_TICKS(500), pdTRUE, (void *)status, seg_display_timer_handle);

		if (xTimerIsTimerActive(seg_timer) != pdTRUE)
		{
			xTimerStart(seg_timer, pdMS_TO_TICKS(100));
		}

		break;
	case SEG_LOADING_WIFI_CONNECT:

		if (seg_timer != NULL && xTimerIsTimerActive(seg_timer) == pdTRUE)
		{
			xTimerStop(seg_timer, pdMS_TO_TICKS(100));
			xTimerDelete(seg_timer, pdMS_TO_TICKS(100));
			seg_timer = NULL;
		}
		if (seg_timer != NULL && xTimerIsTimerActive(seg_timer_1) == pdTRUE)
		{
			xTimerStop(seg_timer_1, pdMS_TO_TICKS(100));
			xTimerDelete(seg_timer_1, pdMS_TO_TICKS(100));
			seg_timer_1 = NULL;
		}
		ws2812_set_pixel_color(16, 0x00, 0xff, 0x00);
		ws2812_set_pixel_brightness(16, seg_timer_dot_brightness_max);
		ws2812_show_leds();
		break;
	case SEG_LOADING_WIFI_DISCONNECT:
		// wifi断开时，红色闪烁
		if (seg_timer_1 == NULL)
			seg_timer_1 = xTimerCreate("seg_timer_1", pdMS_TO_TICKS(500), pdTRUE, (void *)status, seg_display_timer_handle);

		if (xTimerIsTimerActive(seg_timer) == pdTRUE)
		{
			xTimerStop(seg_timer, pdMS_TO_TICKS(100));
			xTimerDelete(seg_timer, pdMS_TO_TICKS(100));
			seg_timer = NULL;
		}
		// 如果正在呼吸状态，停止呼吸
		if (xTimerIsTimerActive(seg_timer_1) != pdTRUE)
		{
			xTimerStart(seg_timer_1, pdMS_TO_TICKS(100));
		}
		break;
	default:
		break;
	}
}