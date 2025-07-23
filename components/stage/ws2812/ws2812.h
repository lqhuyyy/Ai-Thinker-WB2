/**
 * @file ir_ws2812.h
 * @author Seahi-Mo (seahi-mo@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2025-07-22
 *
 * @copyright Ai-Thinker co.,ltd (c) 2025
 *
 */
#ifndef __WS2812_H__
#define __WS2812_H__

#include <bl602_glb.h>
#include <bl602_gpio.h>
#include <bl602_ir.h>
#include "stdint.h"
#include "color_mode.h"
#include "bl_timer.h"

#define IR_PIN_TX GLB_GPIO_PIN_11

typedef struct
{
	color_t color;
	float *brightness;
	uint8_t index;
} ws2812_dev_t;
// 定义WS2812灯条结构体
typedef struct
{
	ws2812_dev_t *dev;
	uint8_t led_count;
	float brightness;
} ws2812_strip_t;
/**
 * @brief 初始化WS2812
 *
 * @param led_count 灯珠数量
 */
void ws2812_init(ws2812_strip_t *ws2812_strip);
/**
 * @brief 设置LED数量
 *
 * @param count
 */
void ws2812_set_led_count(uint8_t count);
/**
 * @brief 设置指定位置LED的颜色
 *
 * @param index 灯珠位置
 * @param r 红色值
 * @param g 绿色值
 * @param b 蓝色值
 */
void ws2812_set_pixel_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
/**
 * @brief 设置指定位置LED的亮度
 *
 * @param index 灯珠位置
 * @param brightness 亮度
 */
void ws2812_set_pixel_brightness(uint8_t index, float brightness);
/**
 * @brief 设置所有LED的颜色
 *
 * @param r
 * @param g
 * @param b
 */
void ws2812_set_all_pixels_color(uint8_t r, uint8_t g, uint8_t b, float brightness);
/**
 * @brief 设置所有LED的亮度
 *
 * @param index
 * @param h
 * @param s
 * @param v
 */
void ws2812_set_pixel_color_hsv(uint8_t index, uint8_t h, uint8_t s, uint8_t v);
/**
 * @brief 设置所有LED的亮度
 *
 * @param brightness
 */
void ws2812_set_global_brightness(float brightness);
/**
 * @brief 显示LED颜色
 *
 */
void ws2812_show_leds(void);
/**
 * @brief 获取LED数量
 *
 * @return uint8_t
 */
uint8_t ws2812_get_led_count(void);
#endif // !__SPI_WS2812_H__