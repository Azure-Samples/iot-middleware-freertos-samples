/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm_pal_freertos_sync.c
 * @brief HFSM Platform Adaptation Layer for FreeRTOS syncrhonous (single threaded) code.
 */

#include <stdint.h>

// The following two required for configASSERT:
#include "FreeRTOS.h"
#include "task.h"

#include "az_hfsm.h"
#include "az_iot_hfsm.h"
#include "az_hfsm_pal_timer.h"

// TODO: 
#include "transport_tls_socket.h"

// The retry Hierarchical Finite State Machine object.
static az_iot_hfsm_type iot_hfsm;

// PAL HFSMs for Provisioning and Hub operations.

static hfsm provisioning_hfsm;
static hfsm hub_hfsm;
static hfsm* active_hfsm = &provisioning_hfsm;

typedef enum
{
  DO_WORK = HFSM_EVENT(1),
  DO_DELAY = HFSM_EVENT(2)
} az_iot_hfsm_pal_sync_event_type;

static const hfsm_event hfsm_event_do_work = { DO_WORK, NULL };
static const hfsm_event hfsm_event_do_delay = { DO_DELAY, NULL };

static az_iot_hfsm_event_data_error error_data;
static hfsm_event hfsm_event_error = { AZ_IOT_ERROR, &error_data };

static int32_t delay_milliseconds;

static int idle(hfsm* me, hfsm_event event);
static int running(hfsm* me, hfsm_event event);
static int timeout(hfsm* me, hfsm_event event);

static state_handler pal_get_parent(state_handler child_state)
{
   return NULL;
}

void az_iot_hfsm_pal_freertos_sync_initialize()
{
    configASSERT(!hfsm_init(&provisioning_hfsm, idle, pal_get_parent));
    configASSERT(!hfsm_init(&hub_hfsm, idle, pal_get_parent)) ;
    configASSERT(!az_iot_hfsm_initialize(&iot_hfsm, &provisioning_hfsm, &hub_hfsm));
}

// TODO: 
void prvSetupNetworkCredentials( bool use_secondary );
uint32_t prvIoTHubRun();
uint32_t prvDeviceProvisioningRun();

static int idle(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case AZ_IOT_START:
        active_hfsm = me;
        // TODO: remove internals access by moving to the start event data.
        prvSetupNetworkCredentials( iot_hfsm._use_secondary_credentials );
        ret = hfsm_transition_peer(me, idle, running);
    default:
      break;
  }

  return ret;
}

static int running(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case DO_WORK:
      if (me == &provisioning_hfsm)
      {
        if (!prvDeviceProvisioningRun())
        {
            // TODO: get actual error.
            error_data.type = AZ_IOT_ERROR_TYPE_NETWORK;
            error_data.iot_status = AZ_IOT_STATUS_UNKNOWN;
            ret = hfsm_post_event((hfsm*)(&iot_hfsm), hfsm_event_error);
        }
        else
        {
            ret = hfsm_post_event((hfsm*)(&iot_hfsm), hfsm_event_az_iot_provisioning_done);
        }
      }
      else
      {
        configASSERT(me == &hub_hfsm);
        while (!prvIoTHubRun())
        {
            // TODO: get actual error.
            error_data.type = AZ_IOT_ERROR_TYPE_NETWORK;
            error_data.iot_status = AZ_IOT_STATUS_UNKNOWN;
            ret = hfsm_post_event((hfsm*)(&iot_hfsm), hfsm_event_error);
        }
      }
      
      break;

    case DO_DELAY:
      ret = hfsm_transition_peer(me, running, timeout);
      break;

    default:
      break;
  }

  return ret;
}

static int timeout(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case DO_WORK:
      vTaskDelay (pdMS_TO_TICKS(delay_milliseconds));
      ret = hfsm_post_event((hfsm*)(&iot_hfsm), hfsm_timeout_event);
      break;

    case AZ_IOT_START:
      ret = hfsm_transition_peer(me, idle, running);
      break;

    default:
      break;
  }

  return ret;
}

/**
 * @brief Message pump for syncronous operations.
 * 
 */
void az_iot_hfsm_pal_freertos_sync_do_work()
{
    hfsm_post_event((hfsm*)(&iot_hfsm), hfsm_event_az_iot_start);

    for ( ; ; )
    {
        hfsm_post_event(&provisioning_hfsm, hfsm_event_do_work);
        hfsm_post_event(&hub_hfsm, hfsm_event_do_work);
    }
}

// az_iot_hfsm_pal_timer
void* az_hfsm_pal_timer_create(hfsm* src)
{
    (void) src;
    // NOOP.
    return NULL;
}

int az_hfsm_pal_timer_start(hfsm* src, void* timer_handle, int32_t milliseconds, bool oneshot)
{
    (void) src;
    (void) timer_handle;
    configASSERT(oneshot == true);
    delay_milliseconds = milliseconds;
    hfsm_post_event(active_hfsm, hfsm_event_do_delay);
    return 0;
}

int az_hfsm_pal_timer_stop(hfsm* src, void* timer_handle)
{
    (void) src;
    (void) timer_handle;
    // NOOP.
    return 0;
}

void az_hfsm_pal_timer_destroy(hfsm* src, void* timer_handle)
{
    (void) src;
    (void) timer_handle;
    // NOOP.
}

uint64_t ullGetUnixTime( void );

uint64_t az_hfsm_pal_timer_get_miliseconds()
{
    return ullGetUnixTime();
}

/**
 * @brief BSP error handler.
 * 
 */
void Error_Handler( void );

/**
 * @brief Critical error. This function should not return.
 * 
 * @param me The calling HFSM object.
 */
void az_iot_hfsm_pal_critical(hfsm* caller)
{
    (void) caller;
    Error_Handler();
}

int32_t az_iot_hfsm_pal_get_random_jitter_msec(hfsm* hfsm)
{
    // TODO get random.
    return 0;
}