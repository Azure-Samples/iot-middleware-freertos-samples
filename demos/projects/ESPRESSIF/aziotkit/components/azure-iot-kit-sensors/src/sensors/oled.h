/* OLED screen Example

   For other examples please check:
   https://github.com/espressif/esp-iot-solution/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */
#ifndef _APP_OLED_H_
#define _APP_OLED_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver/gpio.h"
#include "sensors/iot_ssd1306.h"
#include "sensors/ssd1306_fonts.h"

esp_err_t oled_show_string( ssd1306_handle_t dev, const uint8_t * pucString, uint32_t ulStringLength );
void oled_clean(ssd1306_handle_t dev);
void oled_init(ssd1306_handle_t dev);

#ifdef __cplusplus
}
#endif

#endif
