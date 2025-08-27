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

static homeAssisatnt_device_t ha_dev;
static QueueHandle_t device_queue_handle;

static void device_state_task(void *arg)
{
    dev_msg_t *dev_msg = pvPortMalloc(sizeof(dev_msg_t));
    int ac_type = 0;

    while (1)
    {
        xQueueReceive(device_queue_handle, dev_msg, portMAX_DELAY);
        switch (dev_msg->device_state)
        {
        case DEVICE_SATE_SYSYTEM_INIT:
        {
            blog_info("<<<<<<<<<<<<<<<  DEVICE_SATE_SYSYTEM_INIT");

            // 1:读取WiFi信息
            flash_get_wifi_info(&dev_msg->wifi_info);

            if (strlen(dev_msg->wifi_info.ssid) != 0)
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
                    }
                }
            }
            // 读取HA MQTT信息
            if (flash_get_mqtt_info(&ha_dev.mqtt_info))
            {
                if (flash_get_ha_device_msg(&ha_dev))
                {
                    device_homeAssistant_init(&ha_dev);
                }
            }
        }
        break;
        case DEVICE_STATE_WIFI_CONNECTED:
        {
            blog_info("<<<<<<<<<<<<<<<  DEVICE_STATE_WIFI_CONNECTED");
            // 读取连的AP信息
            blog_info("ssid =%s,password=%s addr=%s", dev_msg->wifi_info.ssid, dev_msg->wifi_info.password, dev_msg->wifi_info.ipv4_addr);

            // 启动HA 连接
            homeAssistant_device_start();
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
        // case DEVICE_STATE_ATCMD_WIFICFG_SET:
        //     blog_info("<<<<<<<<<<<<<<< DEVICE_STATE_ATCMD_WIFICFG_SET");
        //     // wifi_info_t* wifi_info = dev_msg->wifi_info;
        //     blog_info("get wifi info ssid=%s password=%s", dev_msg->wifi_info.ssid, dev_msg->wifi_info.password);
        //     // 如果已经成功正在连接
        //     if (wifi_device_connect_status())
        //     {
        //         // 保存信息，并重新启动
        //         blog_warn("The system will restart in 2 seconds");
        //         flash_save_wifi_info(&dev_msg->wifi_info);
        //         for (size_t i = 0; i < 3; i++)
        //         {
        //             vTaskDelay(pdMS_TO_TICKS(1000));
        //             blog_warn("The system will restart in 2 seconds:%d s", i + 1);
        //         }
        //         bl_sys_reset_system();
        //     }
        //     else
        //     {
        //         quick_connect_wifi(&dev_msg->wifi_info);
        //     }
        //     break;
        case DEVICE_STATE_BLUFI_CONFIG:

            blufi_config_start();
            break;

        case DEVICE_STATE_HOMEASSISTANT_CONNECT:
            blog_info("<<<<<<<<<<<<<<< DEVICE_STATE_HOMEASSISTANT_CONNECT");
            break;

        default:
            break;
        }
        memset(dev_msg, 0, sizeof(dev_msg_t));
    }
}

void device_state_init(void *arg)
{
    device_queue_handle = xQueueCreate(2, sizeof(dev_msg_t));
    BaseType_t err = xTaskCreate(device_state_task, "device_state_task", DEVICE_QUEUE_HANDLE_SIZE * 2, NULL, 10, NULL);

    wifi_device_init(blufi_wifi_event);
    // blufi_wifi_init();
    if (err == pdPASS)
    {
        blog_info("\"device_state_task\" is create OK");
    }
    else
        blog_error("\"device_state_task\" is create error");
    dev_msg_t dev_msg = {0};
    dev_msg.device_state = DEVICE_SATE_SYSYTEM_INIT;
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
