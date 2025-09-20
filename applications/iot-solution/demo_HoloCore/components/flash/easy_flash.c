/**
 * @file easy_flash.c
 * @author SeaHi-Mo(you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-05-26
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "blog.h"
#include <easyflash.h>
#include "wifi_code.h"
#include "device_state.h"
static char *flash_key[] = {"ssid", "pass", "pmk", "band", "chan_id", "mqtt_host", "mqtt_port", "mqtt_clientID", "mqtt_username", "mqtt_password", "ha_name", "ha_manufacturer", "ac_type", "ac_gcode", "reset_num"};
static char *ac_flash_key[] = {"temp", "mode"};

static bool ef_set_bytes(const char *key, char *value, int len)
{
    int result = ef_set_env_blob(key, value, len);
    return result == EF_NO_ERR ? true : false;
}

static int ef_get_bytes(const char *key, char *value, int len)
{
    size_t read_len = 0;
    int result = ef_get_env_blob(key, value, len, &read_len);
    return read_len;
}

bool ef_del_key(const char *key)
{
    return ef_del_env(key);
}

bool flash_save_wifi_info(void *value)
{
    wifi_info_t *wifi_info = (wifi_info_t *)value;
    int result = 0;

    blog_info(" wifi info: ssid:%s, password:%s, pmk:%s, band:%d, chan_id:%d", wifi_info->ssid, wifi_info->password, wifi_info->pmk, wifi_info->band, wifi_info->chan_id);

    if (strlen(wifi_info->ssid) != 0)
        result = ef_set_bytes(flash_key[FLASH_WIFI_SSID], wifi_info->ssid, strlen(wifi_info->ssid));
    if (strlen(wifi_info->password) != 0)
        result = ef_set_bytes(flash_key[FLASH_WIFI_PASSWORD], wifi_info->password, strlen(wifi_info->password));

    if (strlen(wifi_info->pmk) != 0)
        result = ef_set_bytes(flash_key[FLASH_WIFI_PMK], wifi_info->pmk, strlen(wifi_info->pmk));
    if (wifi_info->band)
        result = ef_set_bytes(flash_key[FLASH_WIFI_BAND], (char *)&wifi_info->band, sizeof(wifi_info->band));
    if (wifi_info->chan_id)
    {
        char *chan_id = pvPortMalloc(2);
        memset(chan_id, 0, 2);
        sprintf(chan_id, "%d", wifi_info->chan_id);
        result = ef_set_bytes(flash_key[FLASH_WIFI_CHAN_ID], chan_id, strlen(chan_id));
        vPortFree(chan_id);
    }

    return result == EF_NO_ERR ? true : false;
}

int flash_get_wifi_info(void *value)
{

    if (value == NULL)
    {
        return -1;
    }
    int result = 0;
    size_t read_len = 0;
    wifi_info_t *wifi_info = (wifi_info_t *)value;
    memset(wifi_info->ssid, 0, 64);
    memset(wifi_info->password, 0, 64);
    memset(wifi_info->pmk, 0, 64);
    result = ef_get_bytes(flash_key[FLASH_WIFI_SSID], wifi_info->ssid, sizeof(wifi_info->ssid));
    wifi_info->ssid[result] = '\0';
    result = ef_get_bytes(flash_key[FLASH_WIFI_PASSWORD], wifi_info->password, 64);
    if (result == 0)
    {
        result = ef_get_bytes(flash_key[FLASH_WIFI_PASSWORD], wifi_info->password, 64);
        blog_warn("password is NULL!");
    }
    wifi_info->password[result] = '\0';
    result = ef_get_bytes(flash_key[FLASH_WIFI_PMK], wifi_info->pmk, 64);
    char *chan_id = pvPortMalloc(2);
    memset(chan_id, 0, 2);
    result = ef_get_bytes(flash_key[FLASH_WIFI_CHAN_ID], chan_id, 2);
    wifi_info->chan_id = atoi(chan_id);
    vPortFree(chan_id);
    return result;
}

bool flash_save_mqtt_info(void *value)
{
    if (value == NULL)
    {
        blog_error("value is NULL!");
        return false;
    }
    ha_mqtt_info_t *mqtt_info = (ha_mqtt_info_t *)value;
    int result = 0;
    if (mqtt_info->mqtt_host != NULL)
    {
        result = ef_set_bytes(flash_key[FLASH_MQTT_HOST], mqtt_info->mqtt_host, strlen(mqtt_info->mqtt_host));
    }
    else
    {
        return false;
    }
    if (mqtt_info->port != 0)
    {
        char *port_data = pvPortMalloc(5);
        memset(port_data, 0, 5);
        sprintf(port_data, "%d", mqtt_info->port);
        result = ef_set_bytes(flash_key[FLASH_MQTT_PORT], port_data, strlen(port_data));
        vPortFree(port_data);
    }
    else
    {
        return false;
    }
    return true;
}

int flash_get_mqtt_info(void *value)
{

    ha_mqtt_info_t *mqtt_info = (ha_mqtt_info_t *)value;
    if (mqtt_info == NULL)
    {
        mqtt_info = pvPortMalloc(sizeof mqtt_info);
    }
    char *flash_data = pvPortMalloc(128);
    int result = 0;
    memset(flash_data, 0, 128);
    memset(mqtt_info, 0, sizeof(ha_mqtt_info_t));
    result = ef_get_bytes(flash_key[FLASH_MQTT_HOST], flash_data, 128);
    if (result)
    {
        mqtt_info->mqtt_host = pvPortMalloc(64);
        memset(mqtt_info->mqtt_host, 0, sizeof(mqtt_info->mqtt_host));
        strcpy(mqtt_info->mqtt_host, flash_data);
        // mqtt_info->mqtt_host[result] = '\0';
    }
    else
    {
        vPortFree(flash_data);
        return -1;
    }
    memset(flash_data, 0, 128);

    result = ef_get_bytes(flash_key[FLASH_MQTT_PORT], flash_data, 128);
    if (result)
    {
        flash_data[result] = '\0';
        mqtt_info->port = atoi(flash_data);
    }
    else
    {
        vPortFree(flash_data);
        return -1;
    }
    memset(flash_data, 0, 128);

    vPortFree(flash_data);
    return result;
}

bool flash_save_ha_device_msg(void *value)
{
    if (value == NULL)
    {
        blog_error("value is NULL!");
        return false;
    }
    homeAssisatnt_device_t *dev_ha = (homeAssisatnt_device_t *)value;
    int result = 0;
    if (dev_ha->name != NULL)
    {
        result = ef_set_bytes(flash_key[FLASH_HA_DEV_NAME], dev_ha->name, strlen(dev_ha->name));
    }
    else
    {
        return false;
    }
    if (dev_ha->manufacturer != NULL)
    {
        result = ef_set_bytes(flash_key[FLASH_HA_MANUFACTURER], dev_ha->manufacturer, strlen(dev_ha->manufacturer));
    }
    else
    {
        return false;
    }

    return true;
}

int flash_get_ha_device_msg(void *value)
{
    homeAssisatnt_device_t *dev_ha = (homeAssisatnt_device_t *)value;
    if (dev_ha == NULL)
    {
        dev_ha = pvPortMalloc(sizeof dev_ha);
    }
    char *flash_data = pvPortMalloc(128);
    int result = 0;
    memset(flash_data, 0, 128);

    result = ef_get_bytes(flash_key[FLASH_HA_DEV_NAME], flash_data, 128);
    if (result)
    {
        dev_ha->name = pvPortMalloc(64);
        memset(dev_ha->name, 0, sizeof(dev_ha->name));

        strcpy(dev_ha->name, flash_data);
        // dev_ha->name[result+1] = '\0';
    }
    else
    {
        vPortFree(flash_data);
        return -1;
    }

    memset(flash_data, 0, 128);
    result = ef_get_bytes(flash_key[FLASH_HA_MANUFACTURER], flash_data, 128);

    if (result)
    {
        dev_ha->manufacturer = pvPortMalloc(64);
        memset(dev_ha->manufacturer, 0, sizeof(dev_ha->manufacturer));

        strcpy(dev_ha->manufacturer, flash_data);
    }
    else
    {

        vPortFree(flash_data);
        return -1;
    }
    vPortFree(flash_data);
    return result;
}

bool flash_save_new_temp(float temperature)
{
    char *temp_str = pvPortMalloc(2);
    memset(temp_str, 0, 2);
    sprintf(temp_str, "%.0f", temperature);
    bool ret = ef_set_bytes(ac_flash_key[0], temp_str, 2);
    vPortFree(temp_str);
    return ret;
}

float flash_get_temperature(void)
{
    char *temp = pvPortMalloc(2);
    float temperature = 0.0;
    memset(temp, 0, 2);
    ef_get_bytes(ac_flash_key[0], temp, 2);
    if ((temp[0] >= '0' && temp[0] <= '9') && (temp[1] >= '0' && temp[1] <= '9'))
    {
        temperature = atof(temp);
    }
    else
        temperature = 0.0;

    vPortFree(temp);
    return temperature;
}

int flash_save_reset_count(int count)
{
    char *count_str = pvPortMalloc(2);
    memset(count_str, 0, 2);
    sprintf(count_str, "%d", count);
    bool ret = ef_set_bytes(flash_key[FLASH_HA_RESET_NUM], count_str, 2);
    vPortFree(count_str);
    return ret;
}

int flash_get_reset_count(void)
{
    char *count = pvPortMalloc(2);
    int reset_count = 0;
    memset(count, 0, 2);
    ef_get_bytes(flash_key[FLASH_HA_RESET_NUM], count, 2);
    if ((count[0] >= '0' && count[0] <= '9'))
    {
        reset_count = atoi(count);
    }
    else
        reset_count = -1;

    vPortFree(count);
    return reset_count;
}