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
    #define LIBRARY_LOG_NAME    "IoT_HFSM_PAL"
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

static int azure_iot_sync(hfsm* me, hfsm_event event);
static int idle(hfsm* me, hfsm_event event);
static int running(hfsm* me, hfsm_event event);
static int timeout(hfsm* me, hfsm_event event);

static state_handler pal_get_parent(state_handler child_state)
{
  state_handler parent_state;

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

static int azure_iot_sync(hfsm* me, hfsm_event event)
{
  // Control should never reach this azure_iot_sync state.
  LogInfo( ("azure_iot_sync: PANIC!") );
  configASSERT(0);
  return 0;
}

static int idle(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case AZ_IOT_START:
        LogInfo( ("idle: AZ_IOT_START") );

        active_hfsm = me;
        // TODO: remove internals access by moving to the start event data.
        prvSetupNetworkCredentials( iot_hfsm._use_secondary_credentials );
        ret = hfsm_transition_peer(me, idle, running);
    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

static int running(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
    case HFSM_EXIT:
     break;

    case DO_WORK:
      LogInfo( ("running: DO_WORK") );
      if (me == &provisioning_hfsm)
      {
        if (!prvDeviceProvisioningRun())
        {
            ret = hfsm_post_event((hfsm*)(&iot_hfsm), hfsm_event_az_iot_provisioning_done);
        }
        else
        {
            // TODO: get actual error using AzureIoTProvisioningClient_GetExtendedCode
            error_data.type = AZ_IOT_ERROR_TYPE_NETWORK;
            error_data.iot_status = AZ_IOT_STATUS_UNKNOWN;
            ret = hfsm_post_event((hfsm*)(&iot_hfsm), hfsm_event_error);
        }
      }
      else
      {
        configASSERT(me == &hub_hfsm);
        
        do
        {
            ret = prvIoTHubRun();
        } while (!ret);

        // TODO: get actual error.
        error_data.type = AZ_IOT_ERROR_TYPE_NETWORK;
        error_data.iot_status = AZ_IOT_STATUS_UNKNOWN;
        ret = hfsm_post_event((hfsm*)(&iot_hfsm), hfsm_event_error);
      }
      
      hfsm_transition_peer(me, running, idle);
      break;

    case DO_DELAY:
      LogInfo( ("running: DO_DELAY") );
      ret = hfsm_transition_peer(me, running, timeout);
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

static int timeout(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case DO_WORK:
      LogInfo( ("timeout: DO_WORK") );
      vTaskDelay (pdMS_TO_TICKS(delay_milliseconds));
      ret = hfsm_post_event((hfsm*)(&iot_hfsm), hfsm_timeout_event);
      hfsm_transition_peer(me, timeout, idle);
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
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
        hfsm_post_event(active_hfsm, hfsm_event_do_work);
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

int az_hfsm_pal_timer_sazure_iot_sync(hfsm* src, void* timer_handle)
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
void az_iot_hfsm_pal_critical_error(hfsm* caller)
{
    (void) caller;
    Error_Handler();
}

int32_t az_iot_hfsm_pal_get_random_jitter_msec(hfsm* hfsm)
{
    // TODO get random.
    return 0;
}