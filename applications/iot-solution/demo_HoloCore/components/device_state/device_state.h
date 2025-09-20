/**
 * @file device_state.h
 * @author Seahi (seahi-mo@foxmail.com)
 * @brief
 * @version 0.1
 * @date 2024-05-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H
#include <stdint.h>
#include "wifi_code.h"
#include "easy_flash.h"
#include <../wifi_mgmr.h>
#include "bl_sys.h"
#include "blufi_config.h"
#include "homeAssistantPort.h"
#include "dev_ha.h"

#define DEVICE_HW_SERSION "V1.0.0"

#define DEVICE_QUEUE_HANDLE_SIZE 1024

typedef enum
{
    DEVICE_STATE_NONE = -1,
    DEVICE_SATE_SYSYTEM_INIT,
    DEVICE_STATE_WIFI_CONNECTED,
    DEVICE_STATE_WIFI_CONNECT_ERROR,
    DEVICE_STATE_WIFI_DISCONNECT,
    DEVICE_STATE_WIFI_SCAN_FINISH,
    DEVICE_STATE_BLUFI_CONFIG,
    DEVICE_STATE_HOMEASSISTANT_CONNECT,
} device_state_t;

typedef struct device_state_handle
{
    device_state_t device_state;
    wifi_info_t wifi_info;
    int ac_type;
    homeAssisatnt_device_t *ha_dev;
} dev_msg_t;

void device_state_init(void *arg);
void device_state_update(int is_iqr, dev_msg_t *dev_msg);
#endif