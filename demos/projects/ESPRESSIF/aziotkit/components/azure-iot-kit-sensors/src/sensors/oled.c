/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

/* OLED screen Example

   For other examples please check:
   https://github.com/espressif/esp-iot-solution/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include "oled.h"
#include "driver/gpio.h"
#include "sensors/ssd1306_fonts.h"

void oled_init(ssd1306_handle_t dev)
{
    oled_clean(dev);
}

void oled_clean(ssd1306_handle_t dev)
{
    iot_ssd1306_clear_screen(dev, 0);
}

esp_err_t oled_show_string(ssd1306_handle_t dev, const uint8_t *string, uint32_t stringLength)
{ 
    iot_ssd1306_draw_string(dev, 0, 16, string, stringLength, 16, 1);

    return iot_ssd1306_refresh_gram(dev);
}
