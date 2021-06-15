/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief HFSM for Azure IoT Operations.
 * 
 * @details Implements fault handling for Device Provisioning + IoT Hub operations
 */
#ifndef AZ_IOT_HFSM_H
#define AZ_IOT_HFSM_H

#include <azure/az_core.h>
#include "az_hfsm.h"

#ifndef AZ_IOT_HFSM_CREDENTIAL_COUNT
#define AZ_IOT_HFSM_CREDENTIAL_COUNT 1
#endif

typedef struct
{
  hfsm hfsm;
  int _credential_idx;
  int _retry_count;
  int _start_seconds;
  void* _timer_handle;
  void* _provisioning_handle;
  void* _iothub_handle;
} az_iot_hfsm_type;

// AzIoTHFSM-specific events.
typedef enum
{
  ERROR = HFSM_EVENT(1),
  TIMEOUT = HFSM_EVENT(2),
  AZ_IOT_ERROR = HFSM_EVENT(3),
  AZ_IOT_START = HFSM_EVENT(4),
  AZ_IOT_PROVISIONING_DONE = HFSM_EVENT(5),

} az_iot_hfsm_event_type;

int az_iot_hfsm_initialize(az_iot_hfsm_type* handle);
int az_iot_hfsm_post_sync(az_iot_hfsm_type* handle, hfsm_event event);

// Platform Adaptation Layer (PAL)

// az_iot_hfsm_pal_timer
void* az_iot_hfsm_pal_timer_create();
int az_iot_hfsm_pal_timer_start(void* timer_handle, int32_t seconds);
int az_iot_hfsm_pal_timer_stop(void* timer_handle);
void az_iot_hfsm_pal_timer_destroy(void* timer_handle);
int32_t az_iot_hfsm_pal_timer_get_seconds();

int az_iot_hfsm_pal_provisioning_start(void* provisioning_handle);
int az_iot_hfsm_pal_hub_start(void* hub_handle);

// az_iot_hfsm_pal_critical
void az_iot_hfsm_pal_critical();

#endif //AZ_IOT_HFSM_H
