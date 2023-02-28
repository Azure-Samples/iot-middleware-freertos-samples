/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#include "led.h"
#include "hal/gpio_types.h"
#include "driver/gpio.h"
#include "esp32/rom/gpio.h"

void initialize_leds()
{
     gpio_pad_select_gpio(LED_GPIO_AZURE);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_GPIO_AZURE, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(LED_GPIO_WIFI);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_GPIO_WIFI, GPIO_MODE_OUTPUT);
}

void toggle_azure_led(uint32_t level)
{
    /* LED on (output high) */
    /* LED off (output low) */
    gpio_set_level(LED_GPIO_AZURE, level);
}

void toggle_wifi_led(uint32_t level)
{
    /* LED on (output high) */
    /* LED off (output low) */
    gpio_set_level(LED_GPIO_WIFI, level);
}
