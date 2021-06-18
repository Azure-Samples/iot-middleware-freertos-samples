/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm_pal_freertos_sync.c
 * @brief HFSM Platform Adaptation Layer for FreeRTOS syncrhonous (single threaded) code.
 */

#include <stdint.h>
#include <azure/core/internal/az_precondition_internal.h>

#include "az_hfsm.h"
#include "az_hfsm_pal_timer.h"
#include "az_iot_hfsm.h"
#include "az_iot_hfsm_sync_adapter.h"

// TODO: change logging to az_log_internal.
/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for DEMO.
 * 3. Include the header file "logging_stack.h", if logging is enabled for DEMO.
 */

#include "logging_levels.h"

/* Logging configuration for the Demo. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "IoT_HFSM_SYNC"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif

/* 
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

/* Map the SdkLog macro to the logging function to enable logging */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif

#include "logging_stack.h"
/**************************************************/

// The retry Hierarchical Finite State Machine object.
// A single Provisioning + Hub client is supported when syncrhonous mode is used.
static az_iot_hfsm_type iot_hfsm;

// HFSM adapters for Provisioning and Hub operations.
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
static az_hfsm provisioning_hfsm;
#endif
static az_hfsm hub_hfsm;

// The two HFSM objects share the same implementation. Only one is active at any time.
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
static az_hfsm* active_hfsm = &provisioning_hfsm;
#else
static az_hfsm* active_hfsm = &hub_hfsm;
#endif

typedef enum
{
  AZ_IOT_HFSM_SYNC_DO_WORK = AZ_HFSM_EVENT(1),
  AZ_IOT_HFSM_SYNC_DO_DELAY = AZ_HFSM_EVENT(2)
} az_iot_hfsm_sync_event_type;

static const az_hfsm_event az_hfsm_event_do_work = { AZ_IOT_HFSM_SYNC_DO_WORK, NULL };
static const az_hfsm_event az_hfsm_event_do_delay = { AZ_IOT_HFSM_SYNC_DO_DELAY, NULL };

static az_hfsm_event az_hfsm_sync_event_error = { AZ_IOT_ERROR, NULL };

static int32_t delay_milliseconds;

static int32_t azure_iot_sync(az_hfsm* me, az_hfsm_event event);
static int32_t idle(az_hfsm* me, az_hfsm_event event);
static int32_t running(az_hfsm* me, az_hfsm_event event);
static int32_t timeout(az_hfsm* me, az_hfsm_event event);

static az_hfsm_state_handler provisioning_and_hub_get_parent(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if ((child_state == azure_iot_sync))
  {
    parent_state = NULL;
  }
  else 
  {
    parent_state = azure_iot_sync;
  }

  return parent_state;
}

static int32_t azure_iot_sync(az_hfsm* me, az_hfsm_event event)
{
  switch ((int32_t)event.type)
  {
    case AZ_HFSM_ENTRY:
    case AZ_HFSM_EXIT:
      break;
    
    default:
      // Control should never reach this azure_iot_sync state.
      LogInfo( ("azure_iot_sync: PANIC!") );
      az_iot_hfsm_pal_critical_error(me);
  }

  return 0;
}

static int32_t idle(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = 0;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_ENTRY:
    case AZ_HFSM_EXIT:
    case AZ_IOT_HFSM_SYNC_DO_WORK:
      break;

    case AZ_IOT_START:
        LogInfo( ("idle: AZ_IOT_START") );

        active_hfsm = me;
        // TODO: remove internals access by moving to the start event data.
        az_iot_hfsm_sync_adapter_pal_set_credentials( iot_hfsm._use_secondary_credentials );
        ret = az_hfsm_transition_peer(me, idle, running);
        break;
        
    default:
      ret = AZ_HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

static int32_t running(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = 0;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_ENTRY:
    case AZ_HFSM_EXIT:
     break;

    case AZ_IOT_HFSM_SYNC_DO_WORK:
      LogInfo( ("running: AZ_IOT_HFSM_SYNC_DO_WORK") );
      az_iot_hfsm_event_data_error status;

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
      if (me == &provisioning_hfsm)
      {
        status = az_iot_hfsm_sync_adapter_pal_run_provisioning();
        if (status.type == AZ_IOT_OK)
        {
          ret = az_hfsm_post_event((az_hfsm*)(&iot_hfsm), az_hfsm_event_az_iot_provisioning_done);
        }
      }
      else
      {
#endif
        _az_PRECONDITION(me == &hub_hfsm);
        
        do
        {
            status = az_iot_hfsm_sync_adapter_pal_run_hub();
        } while (status.type == AZ_IOT_OK);
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
      }
#endif

      if (status.type != AZ_IOT_OK)
      {
        az_hfsm_sync_event_error.data = &status;
        ret = az_hfsm_post_event((az_hfsm*)(&iot_hfsm), az_hfsm_sync_event_error);
      }

      az_hfsm_transition_peer(me, running, idle);
      break;

    case AZ_IOT_HFSM_SYNC_DO_DELAY:
      LogInfo( ("running: AZ_IOT_HFSM_SYNC_DO_DELAY") );
      ret = az_hfsm_transition_peer(me, running, timeout);
      break;

    default:
      ret = AZ_HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

static int32_t timeout(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = 0;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_ENTRY:
    case AZ_HFSM_EXIT:
      break;

    case AZ_IOT_HFSM_SYNC_DO_WORK:
      LogInfo( ("timeout: AZ_IOT_HFSM_SYNC_DO_WORK") );
      az_iot_hfsm_sync_adapter_sleep(delay_milliseconds);
      ret = az_hfsm_post_event((az_hfsm*)(&iot_hfsm), az_hfsm_timeout_event);
      az_hfsm_transition_peer(me, timeout, idle);
      break;

    default:
      ret = AZ_HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// az_iot_hfsm_pal_timer
void* az_hfsm_pal_timer_create(az_hfsm* src)
{
    (void) src;
    // NOOP.
    return src; // must return non-NULL.
}

int32_t az_hfsm_pal_timer_start(az_hfsm* src, void* timer_handle, int32_t milliseconds, bool oneshot)
{
    (void) src;
    (void) timer_handle;
    _az_PRECONDITION(oneshot == true);
    delay_milliseconds = milliseconds;
    az_hfsm_post_event(active_hfsm, az_hfsm_event_do_delay);
    return 0;
}

int32_t az_hfsm_pal_timer_stop(az_hfsm* src, void* timer_handle)
{
    (void) src;
    (void) timer_handle;
    // NOOP.
    return 0;
}

void az_hfsm_pal_timer_destroy(az_hfsm* src, void* timer_handle)
{
    (void) src;
    (void) timer_handle;
    // NOOP.
}

/**
 * @brief Constructor
 * 
 * @return int32_t non-zero on error.
 */
int32_t az_iot_hfsm_sync_adapter_sync_initialize()
{
  int32_t ret;

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
  ret = az_hfsm_init(&provisioning_hfsm, idle, provisioning_and_hub_get_parent);

  if (!ret)
  {
#endif
    ret = az_hfsm_init(&hub_hfsm, idle, provisioning_and_hub_get_parent);

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
  }
#endif

  if (!ret)
  {
    az_iot_hfsm_initialize(
      &iot_hfsm, 
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
      &provisioning_hfsm, 
#endif
      &hub_hfsm);
  }

  return ret;
}

/**
 * @brief Message pump for syncronous operations.
 * 
 */
void az_iot_hfsm_sync_adapter_sync_do_work()
{
    az_hfsm_post_event((az_hfsm*)(&iot_hfsm), az_hfsm_event_az_iot_start);

    for ( ; ; )
    {
        az_hfsm_post_event(active_hfsm, az_hfsm_event_do_work);
    }
}
