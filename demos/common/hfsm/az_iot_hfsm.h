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

#ifndef MIN_RETRY_DELAY_MSEC
#define MIN_RETRY_DELAY_MSEC 1000
#endif 

#ifndef MAX_RETRY_DELAY_MSEC
#define MAX_RETRY_DELAY_MSEC 100000
#endif 

typedef struct
{
  hfsm hfsm;
  bool _use_secondary_credentials;
  int16_t _retry_attempt;
  uint64_t _start_time_msec;
  void* _timer_handle;
  hfsm* _provisioning_hfsm;
  hfsm* _iothub_hfsm;
} az_iot_hfsm_type;

// AzIoTHFSM-specific events.
typedef enum
{
  AZ_IOT_ERROR = HFSM_EVENT(4),
  AZ_IOT_START = HFSM_EVENT(5),
  AZ_IOT_PROVISIONING_DONE = HFSM_EVENT(5),
} az_iot_hfsm_event_type;

extern const hfsm_event hfsm_event_az_iot_start;
extern const hfsm_event hfsm_event_az_iot_provisioning_done;

typedef struct {
   bool is_communications;
   bool is_security;
   bool is_azure_iot;
   az_iot_status iot_status;
} az_iot_hfsm_event_data_error;

int az_iot_hfsm_initialize(az_iot_hfsm_type* iot_hfsm, hfsm* provisioning_hfsm, hfsm* hub_hfsm);
int az_iot_hfsm_post_sync(az_iot_hfsm_type* iot_hfsm, hfsm_event event);

// Platform Adaptation Layer (PAL)

/**
 * @brief Critical error. This function should not return.
 * 
 * @param me The calling HFSM object.
 */
void az_iot_hfsm_pal_critical(hfsm* hfsm);

/**
 * @brief Get random jitter in milliseconds.
 * 
 * @details This function must return a random number representing the milliseconds of jitter applied during
 *          Azure IoT retries. The recommended maximum jitter is 5000ms (5 seconds).
 * 
 * @param hfsm The requesting HFSM.
 * @return int32_t The random jitter in milliseconds.
 */
int32_t az_iot_hfsm_pal_get_random_jitter_msec(hfsm* hfsm);

#endif //AZ_IOT_HFSM_H
