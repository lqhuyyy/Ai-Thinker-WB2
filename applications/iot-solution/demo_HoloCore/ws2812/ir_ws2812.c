/**
 * @file ir_ws2812.c
 * @author Seahi-Mo (seahi-mo@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2025-07-22
 *
 * @copyright Ai-Thinker co.,ltd (c) 2025
 *
 */
#include "ir_ws2812.h"
#include "math.h"
#include "FreeRTOS.h"
#include "task.h"
#include "blog.h"

ws2812_strip_t *ws2812_strip_dev = NULL;
// 初始化IR LED GPIO
static void ir_led_gpio_init(void)
{
	GLB_GPIO_Type pin = IR_PIN_TX;

	GLB_GPIO_Func_Init(GPIO_FUN_ANALOG, &pin, 1);
	GLB_IR_LED_Driver_Enable();
}

void ws2812_init(ws2812_strip_t *ws2812_strip)
{
	if (ws2812_strip == NULL || ws2812_strip->led_count == 0)
	{
		blog_error("ws2812_strip is NULL or led_count is 0");
		return;
	}
	ir_led_gpio_init();
	// led_wo_init();
	IR_LEDInit(HBN_XCLK_CLK_RC32M, 1, 2, 8, 16, 16, 4);
	vTaskDelay(pdMS_TO_TICKS(1)); // 等待初始化完成
	ws2812_strip->dev = pvPortMalloc(sizeof(ws2812_dev_t) * ws2812_strip->led_count);
	ws2812_strip->brightness = 0.5;
	for (uint8_t i = 0; i < ws2812_strip->led_count; i++)
	{
		ws2812_strip->dev[i].index = i;
		ws2812_strip->dev[i].brightness = &ws2812_strip->brightness;
		ws2812_strip->dev[i].color.r = 0;
		ws2812_strip->dev[i].color.g = 0;
		ws2812_strip->dev[i].color.b = 0;
	}
	ws2812_strip_dev = ws2812_strip;
}
/**
 * @brief 反转8位数据的位顺序,通过红外发送时需要将数据反转
 *
 * @param high_data
 * @return uint8_t
 */
static uint8_t reverse_bits(uint8_t high_data)
{
	uint8_t low_data = 0;
	for (uint8_t i = 0; i < 8; i++)
	{
		low_data <<= 1;				 // 低位数据左移，腾出最低位
		low_data |= (high_data & 1); // 取高位数据的最低位，放到低位数据的最低位
		high_data >>= 1;			 // 高位数据右移，处理下一位
	}
	return low_data;
}
// 设置单个灯珠颜色，带亮度控制
void ws2812_set_pixel_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
	if (index < ws2812_strip_dev->led_count)
	{
		ws2812_strip_dev->dev[index].color.r = (uint16_t)reverse_bits(r);
		ws2812_strip_dev->dev[index].color.g = (uint16_t)reverse_bits(g);
		ws2812_strip_dev->dev[index].color.b = (uint16_t)reverse_bits(b);
	}
}
void ws2812_set_pixel_brightness(uint8_t index, float brightness)
{
	// 更新所有LED以应用新亮度
	hsv_color_t hsv = rgb_to_hsv(ws2812_strip_dev->dev[index].color);
	// 使用原始颜色的HSV，仅调整亮度（强制最大值以保持颜色）
	hsv.v = brightness;
	color_t rgb = hsv_to_rgb(hsv);
	ws2812_set_pixel_color(index, rgb.r, rgb.g, rgb.b);
	ws2812_show_leds();
}
void ws2812_set_all_pixels_color(uint8_t r, uint8_t g, uint8_t b, float brightness)
{
	color_t rgb = {r, g, b};
	for (uint8_t i = 0; i < ws2812_strip_dev->led_count; i++)
	{
		rgb.r = r;
		rgb.g = g;
		rgb.b = b;
		hsv_color_t hsv = rgb_to_hsv(rgb);
		// 使用原始颜色的HSV，仅调整亮度（强制最大值以保持颜色）
		hsv.v = brightness;
		rgb = hsv_to_rgb(hsv);
		ws2812_set_pixel_color(i, rgb.r, rgb.g, rgb.b);
	}
	ws2812_show_leds();
}

// 设置全局亮度（改进低亮度处理）
void ws2812_set_global_brightness(float brightness)
{
	// 更新所有LED以应用新亮度
	ws2812_strip_dev->brightness = brightness;
	for (uint8_t i = 0; i < ws2812_strip_dev->led_count; i++)
	{
		hsv_color_t hsv = rgb_to_hsv(ws2812_strip_dev->dev[i].color);
		// 使用原始颜色的HSV，仅调整亮度（强制最大值以保持颜色）
		hsv.v = ws2812_strip_dev->brightness;
		color_t rgb = hsv_to_rgb(hsv);
		ws2812_set_pixel_color(i, rgb.r, rgb.g, rgb.b);
	}
	ws2812_show_leds();
}
void ws2812_set_pixel_color_hsv(uint8_t index, uint8_t h, uint8_t s, uint8_t v)
{
	hsv_color_t hsv = {h, s, v};
	color_t rgb = hsv_to_rgb(hsv);
	ws2812_set_pixel_color(index, rgb.r, rgb.g, rgb.b);
}
// 将颜色缓冲区数据发送到WS2812
void ws2812_show_leds(void)
{
	for (uint8_t i = 0; i < ws2812_strip_dev->led_count; i++)
	{
		uint32_t brg_color = (ws2812_strip_dev->dev[i].color.b << 16) | (ws2812_strip_dev->dev[i].color.r << 8) | ws2812_strip_dev->dev[i].color.g;
		IR_SendCommand(0, brg_color);
	}
	vTaskDelay(pdMS_TO_TICKS(1)); // 等待初始化完成
}

// 设置灯珠数量
void ws2812_set_led_count(uint8_t count)
{
	ws2812_strip_dev->led_count = count;

	// ws2812_show_leds();
}

uint8_t ws2812_get_led_count(void)
{
	return ws2812_strip_dev->led_count;
}