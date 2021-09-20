/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#ifndef _LED_H
#define _LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"

#define LED_GPIO_AZURE 33
#define LED_GPIO_WIFI 32

#define LED_ON  1
#define LED_OFF 0

void initialize_leds();
void toggle_azure_led(uint32_t level);
void toggle_wifi_led(uint32_t level);

#ifdef __cplusplus
}
#endif

#endif /* _LED_H */
