/* OLED screen Example

   For other examples please check:
   https://github.com/espressif/esp-iot-solution/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "oled.h"

void oled_show_signs(ssd1306_handle_t dev);

void oled_init(ssd1306_handle_t dev)
{
    oled_show_signs(dev);
}

void oled_clean(ssd1306_handle_t dev)
{
    iot_ssd1306_clear_screen(dev, 0);
    oled_show_signs(dev);
}

void oled_show_signs(ssd1306_handle_t dev)
{
    iot_ssd1306_draw_bitmap(dev, 0, 2, &c_chSingal816[0], 16, 8);
    iot_ssd1306_draw_bitmap(dev, 24, 2, &c_chBluetooth88[0], 8, 8);
    iot_ssd1306_draw_bitmap(dev, 40, 2, &c_chMsg816[0], 16, 8);
    iot_ssd1306_draw_bitmap(dev, 64, 2, &c_chGPRS88[0], 8, 8);
    iot_ssd1306_draw_bitmap(dev, 90, 2, &c_chAlarm88[0], 8, 8);
    iot_ssd1306_draw_bitmap(dev, 112, 2, &c_chBat816[0], 16, 8);
}

esp_err_t oled_show_string( ssd1306_handle_t dev, const uint8_t * pucString, uint32_t ulStringLength )
{ 
    iot_ssd1306_draw_string(dev, 0, 16, pucString, ulStringLength, 16, 1);

    return iot_ssd1306_refresh_gram(dev);
}
