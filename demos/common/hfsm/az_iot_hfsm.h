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

#include <stdint.h>
#include <stdbool.h>

#include <azure/az_core.h>
#include <azure/iot/az_iot_common.h>
#include "az_hfsm.h"

typedef struct
{
  hfsm hfsm;
  bool _use_secondary_credentials;
  int _retry_count;
  int _start_seconds;
  void* _timer_handle;
} az_iot_hfsm_type;

// AzIoTHFSM-specific events.
typedef enum
{
  AZ_IOT_ERROR = HFSM_EVENT(4),
  AZ_IOT_START = HFSM_EVENT(5),
  AZ_IOT_PROVISIONING_DONE = HFSM_EVENT(5),
} az_iot_hfsm_event_type;

typedef struct {
   bool is_communications;
   bool is_security;
   bool is_azure_iot;
   az_iot_status iot_status;
} az_iot_hfsm_event_data_error;

int az_iot_hfsm_initialize(az_iot_hfsm_type* handle);
int az_iot_hfsm_post_sync(az_iot_hfsm_type* handle, hfsm_event event);

// Platform Adaptation Layer (PAL)

/**
 * @brief 
 * 
 * @param me The calling HFSM object.
 * @param provisioning_handle 
 * @return int 
 */
int az_iot_hfsm_pal_provisioning_start(hfsm* caller, bool use_secondary_credentials);

/**
 * @brief 
 * 
 * @param me The calling HFSM object.
 * @param hub_handle 
 * @return int 
 */
int az_iot_hfsm_pal_hub_start(hfsm* caller, bool use_secondary_credentials);

/**
 * @brief Critical error. This function should not return.
 * 
 * @param me The calling HFSM object.
 */
void az_iot_hfsm_pal_critical(hfsm* caller, az_iot_hfsm_event_data_error error);

#endif //AZ_IOT_HFSM_H
