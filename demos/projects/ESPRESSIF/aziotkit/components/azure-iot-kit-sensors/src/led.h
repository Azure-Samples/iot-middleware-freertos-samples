// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef _LED_H
#define _LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"

#define LED_GPIO_AZURE 33
#define LED_GPIO_WIFI 32

void toggle_azure_led(uint32_t level);
void toggle_wifi_led(uint32_t level);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */