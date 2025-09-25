/**
 * @file device_state.c
 * @author Seahi (seahi-mo@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2024-05-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <blog.h>
#include <device_state.h>
#include <bl_gpio.h>
#include "seg_dev.h"
#include "timers.h"
#include "sntp.h"

#include <utils_time.h>
#include "bilibili_follower.h"

color_t GREEN = {0x00, 0xff, 0x00};
color_t BLUE = {0x00, 0x00, 0xff};

static QueueHandle_t device_queue_handle;
static TimerHandle_t device_state_timer_handle = NULL;
static TimerHandle_t SNTP_gernerate_timer_handle = NULL;
static TimerHandle_t fans_count_timer_handle = NULL;
static bool is_blufi_config = false;
static bool seg_is_timer_running = true;
static void device_state_task(void *arg)
{
    dev_msg_t *dev_msg = pvPortMalloc(sizeof(dev_msg_t));
    int ac_type = 0;
    BaseType_t type = pdFALSE;
    int i = 0;

    while (1)
    {
        if (xQueueReceive(device_queue_handle, dev_msg, pdMS_TO_TICKS(1000) == pdTRUE))
        {
            switch (dev_msg->device_state)
            {
            case DEVICE_SATE_SYSYTEM_INIT:
            {
                blog_info("<<<<<<<<<<<<<<<  DEVICE_SATE_SYSYTEM_INIT");
                seg_display_loading(SEG_LOADING_BLUFI_CONFIG);
            }
            break;
            case DEVICE_STATE_WIFI_SCAN_FINISH:
                blog_info("<<<<<<<<<<<<<<<  DEVICE_STATE_WIFI_SCAN_FINISH");
                // 1:读取WiFi信息
                flash_get_wifi_info(&dev_msg->wifi_info);
                if (strlen(dev_msg->wifi_info.ssid) != 0 && is_blufi_config == false)
                {
                    blog_info("get wifi info ssid=%s password=%s", dev_msg->wifi_info.ssid, dev_msg->wifi_info.password);
                    // 在扫描的设备当中查找是否有该ssid

                    for (size_t i = WIFI_MGMR_SCAN_ITEMS_MAX - 1; i > 0; i--)
                    {
                        // 识别到该设备之后，发起连接
                        if (!memcmp(wifiMgmr.scan_items[i].ssid, dev_msg->wifi_info.ssid, strlen(dev_msg->wifi_info.ssid)))
                        {
                            blog_info("scan \"%s\" is OK", dev_msg->wifi_info.ssid);
                            dev_msg->wifi_info.band = 0;
                            dev_msg->wifi_info.chan_id = 4212 + wifiMgmr.scan_items[i].channel * 5;
                            quick_connect_wifi(&dev_msg->wifi_info);

                            // 退出循环
                            goto __EXIT;
                        }
                    }
                    blog_warn("no wifi info")
                }
            __EXIT:
                break;
            case DEVICE_STATE_WIFI_CONNECTED:
            {
                blog_info("<<<<<<<<<<<<<<<  DEVICE_STATE_WIFI_CONNECTED");
                // 读取连的AP信息
                blog_info("ssid =%s,password=%s addr=%s", dev_msg->wifi_info.ssid, dev_msg->wifi_info.password, dev_msg->wifi_info.ipv4_addr);
                seg_display_loading(SEG_LOADING_WIFI_CONNECT);

                xTimerStart(SNTP_gernerate_timer_handle, pdMS_TO_TICKS(100));

                flash_save_reset_count(0);
                // 如果连接信息保存的不一致，则重新保存
                wifi_info_t flash_wifi_info = {0};
                flash_get_wifi_info(&flash_wifi_info);
                if (memcmp(flash_wifi_info.ssid, dev_msg->wifi_info.ssid, strlen(dev_msg->wifi_info.ssid)) ||
                    memcmp(flash_wifi_info.password, dev_msg->wifi_info.password, strlen(dev_msg->wifi_info.password)) || (flash_wifi_info.chan_id != dev_msg->wifi_info.chan_id))
                {
                    // 重新保存新的WiFi信息
                    flash_save_wifi_info(&dev_msg->wifi_info);
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            break;

            case DEVICE_STATE_BLUFI_CONFIG:
                is_blufi_config = true;
                seg_display_loading(SEG_LOADING_BLUFI_CONFIG);
                blufi_config_start();
                break;
            default:
                break;
            case DEVIDE_STATE_CFG_STATE_SHORT_PRESS:
                blog_info("<<<<<<<<<<<<<<<  DEVIDE_STATE_CFG_STATE_SHORT_PRESS");
                // 切换显示
                if (seg_is_timer_running == false)
                {
                    if (xTimerIsTimerActive(SNTP_gernerate_timer_handle) == pdFALSE)
                        xTimerStart(SNTP_gernerate_timer_handle, pdMS_TO_TICKS(100));
                    if (xTimerIsTimerActive(fans_count_timer_handle) == pdTRUE)
                        xTimerStop(fans_count_timer_handle, pdMS_TO_TICKS(100));
                    seg_is_timer_running = true;
                }
                else
                {
                    // ws2812_set_all_pixels_color(0, 0, 0, 0);
                    seg_is_timer_running = false;
                    // 判断timer 是否在运行
                    if (xTimerIsTimerActive(SNTP_gernerate_timer_handle) == pdTRUE)
                        xTimerStop(SNTP_gernerate_timer_handle, pdMS_TO_TICKS(100));
                    int fans_count = flash_get_follower_count();
                    if (fans_count == 0)
                    {
                        /* code */
                        fans_count = bilibili_get_fans_count("355202584");
                        flash_save_follower_count(fans_count);
                    }
                    seg_display_fans_count_color_mode(fans_count, 0, 0.05);
                    // 判断 timer 是否在运行
                    if (xTimerIsTimerActive(fans_count_timer_handle) == pdFALSE)
                        xTimerStart(fans_count_timer_handle, pdMS_TO_TICKS(100));
                }
                break;
            case DEVICE_STATE_CFG_STATE_LONG_PRESS:
                blog_info("<<<<<<<<<<<<<<<  DEVICE_STATE_CFG_STATE_LONG_PRESS");
                // 进入配网模式
                blufi_config_start();
                break;
            case DEVICE_STATE_CFG_STATE_DOUBLE_CLICK:
                blog_info("<<<<<<<<<<<<<<<  DEVICE_STATE_CFG_STATE_DOUBLE_CLICK");
                break;
            case DEVICE_STATE_HTTP_REQUEST:
                blog_info("<<<<<<<<<<<<<<<  DEVICE_STATE_HTTP_REQUEST");

                int fans_count = bilibili_get_fans_count("355202584");

                if (fans_count != -1)
                {
                    int fans_count_flash = flash_get_follower_count();
                    if (fans_count_flash != fans_count)
                    {
                        flash_save_follower_count(fans_count);
                        seg_display_fans_count_color_mode(fans_count, 0, 0.05);
                    }
                }
                break;
            }

            memset(dev_msg, 0, sizeof(dev_msg_t));
        }
        i++;
        if (i == 9)
        {
            i = 0;
        }
    }
}
static void device_state_timer_callback(TimerHandle_t xTimer)
{
    int ret = pvTimerGetTimerID(xTimer);

    if (ret == 0)
    {
        flash_save_reset_count(0);
        xTimerDelete(xTimer, pdMS_TO_TICKS(100));
    }
    else if (ret == 1)
    {
        uint32_t seconds = 0, frags = 0;
        utils_time_date_t date;
        sntp_get_time(&seconds, &frags);
        utils_time_date_from_epoch(seconds + 8 * 60 * 60, &date);
        blog_info("Date & time is: %u-%02u-%02u %02u:%02u:%02u (Day %u of week, Day %u of Year)",
                  date.ntp_year,
                  date.ntp_month,
                  date.ntp_date,
                  date.ntp_hour,
                  date.ntp_minute,
                  date.ntp_second,
                  date.ntp_week_day,
                  date.day_of_year);

        // seg_display_time((int)date.ntp_hour, (int)date.ntp_minute, BLUE, 0.2);
        seg_display_time_ex_color_mode((int)date.ntp_hour, (int)date.ntp_minute, 0, 0.05);
    }
    else if (ret == 2)
    {
        dev_msg_t dev_msg = {0};
        dev_msg.device_state = DEVICE_STATE_HTTP_REQUEST;
        device_state_update(false, &dev_msg);
    }
}

void device_state_init(void *arg)
{
    device_queue_handle = xQueueCreate(2, sizeof(dev_msg_t));
    BaseType_t err = xTaskCreate(device_state_task, "device_state_task", DEVICE_QUEUE_HANDLE_SIZE * 8, NULL, 9, NULL);

    wifi_device_init(blufi_wifi_event);
    // blufi_wifi_init();
    if (err == pdPASS)
    {
        blog_info("\"device_state_task\" is create OK");
    }
    else
        blog_error("\"device_state_task\" is create error");

    // 检查是否为配网模式
    int reset_count = flash_get_reset_count();
    blog_info("reset_count =%d", reset_count);

    if (reset_count == -1)
        flash_save_reset_count(0);
    dev_msg_t dev_msg = {0};
    device_state_timer_handle = xTimerCreate("device_state_timer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, device_state_timer_callback);
    SNTP_gernerate_timer_handle = xTimerCreate("SNTP_gernerate_timer", pdMS_TO_TICKS(1000), pdTRUE, (void *)1, device_state_timer_callback);

    fans_count_timer_handle = xTimerCreate("fans_count_timer", pdMS_TO_TICKS(10000), pdTRUE, (void *)2, device_state_timer_callback);
    xTimerStart(device_state_timer_handle, pdMS_TO_TICKS(100));
    if (reset_count >= 3 || reset_count == -1)
    {
        dev_msg.device_state = DEVICE_STATE_BLUFI_CONFIG;
    }
    else
    {
        dev_msg.device_state = DEVICE_SATE_SYSYTEM_INIT;
        if (reset_count > 10)
        {
            flash_save_reset_count(0);
        }
        else
        {

            flash_save_reset_count(reset_count + 1);
        }
    }
    device_state_update(false, &dev_msg); // WiFi 准备OK,等待连接
}

void device_state_update(int is_iqr, dev_msg_t *dev_msg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (is_iqr)
    {
        xQueueSendFromISR(device_queue_handle, dev_msg, &xHigherPriorityTaskWoken);
    }
    else
    {
        xQueueSend(device_queue_handle, dev_msg, portMAX_DELAY);
    }
}
