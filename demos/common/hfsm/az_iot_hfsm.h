/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief HFSM for Azure IoT Operations.
 * 
 * @details Implements fault handling for Device Provisioning + IoT Hub operations
 */
#ifndef _az_IOT_HFSM_H
#define _az_IOT_HFSM_H

#include <stdint.h>
#include <stdbool.h>

#include <azure/az_core.h>
#include <azure/iot/az_iot_common.h>
#include "az_hfsm.h"

#ifndef AZ_IOT_HFSM_MIN_RETRY_DELAY_MSEC
#define AZ_IOT_HFSM_MIN_RETRY_DELAY_MSEC 1000
#endif 

#ifndef AZ_IOT_HFSM_MAX_RETRY_DELAY_MSEC
#define AZ_IOT_HFSM_MAX_RETRY_DELAY_MSEC 100000
#endif 

#ifndef AZ_IOT_HFSM_MAX_RETRY_JITTER_MSEC
#define AZ_IOT_HFSM_MAX_RETRY_JITTER_MSEC 5000
#endif 

#define AZ_IOT_HFSM_PROVISIONING_ENABLED

typedef struct
{
  az_hfsm hfsm;
  bool _use_secondary_credentials;
  int16_t _retry_attempt;
  uint64_t _start_time_msec;
  void* _timer_handle;
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
  az_hfsm* _provisioning_hfsm;
#endif
  az_hfsm* _iothub_hfsm;
} az_iot_hfsm_type;

// AzIoTHFSM-specific events.
typedef enum
{
  AZ_IOT_ERROR = AZ_HFSM_EVENT(1),
  AZ_IOT_START = AZ_HFSM_EVENT(2),
  AZ_IOT_PROVISIONING_DONE = AZ_HFSM_EVENT(3),
} az_iot_hfsm_event_type;

extern const az_hfsm_event az_hfsm_event_az_iot_start;

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
extern const az_hfsm_event az_hfsm_event_az_iot_provisioning_done;
#endif

typedef enum
{
  AZ_IOT_OK,
  AZ_IOT_ERROR_TYPE_NETWORK,
  AZ_IOT_ERROR_TYPE_SECURITY,
  AZ_IOT_ERROR_TYPE_SERVICE,
} az_iot_hfsm_event_data_error_type;

typedef struct {
  az_iot_hfsm_event_data_error_type type;
  az_iot_status iot_status;
} az_iot_hfsm_event_data_error;

int32_t az_iot_hfsm_initialize(
  az_iot_hfsm_type* iot_hfsm, 
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
  az_hfsm* provisioning_hfsm, 
#endif
  az_hfsm* hub_hfsm);

// Platform Adaptation Layer (PAL)

/**
 * @brief Critical error. This function should not return.
 * 
 * @param me The calling HFSM object.
 */
void az_iot_hfsm_pal_critical_error(az_hfsm* hfsm);

/**
 * @brief Get random jitter in milliseconds.
 * 
 * @details This function must return a random number representing the milliseconds of jitter applied during
 *          Azure IoT retries. The recommended maximum jitter is 5000ms (5 seconds).
 * 
 * @param hfsm The requesting HFSM.
 * @return int32_t The random jitter in milliseconds.
 */
int32_t az_iot_hfsm_pal_get_random_jitter_msec(az_hfsm* hfsm);

#endif //_az_IOT_HFSM_H
