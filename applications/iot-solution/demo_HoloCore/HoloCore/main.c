#include <bl_ir.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <bl_irq.h>
#include <bl_sys.h>

#include "seg_dev.h"
color_t RED = {0xff, 0x00, 0x00};
color_t GREEN = {0x00, 0xff, 0x00};
color_t BLUE = {0x00, 0x00, 0xff};

void main(void)
{
    bl_sys_init();  // 初始化系统
    seg_dev_init(); // 初始化数码管
    seg_display_time(15, 13, GREEN, 0.1);
    while (1)
    {
        for (size_t i = 0; i <= 99; i++)
        {
            // 显示数字
            seg_dispaly_tempture(i, BLUE, 0.05);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}