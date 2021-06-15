/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief HFSM for Azure IoT Operations.
 * 
 * @details Implements fault handling for Device Provisioning + IoT Hub operations
 */

#include <stdint.h>

// The following two required for configASSERT:
#include "FreeRTOS.h"
#include "task.h"

#include "az_hfsm.h"
#include "az_iot_hfsm.h"

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
    #define LIBRARY_LOG_NAME    "IoT_HFSM"
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


#define USE_PROVISIONING  // Comment out if Azure IoT Provisioning is not used.

static int azure_iot(hfsm* me, hfsm_event event);
static int idle(hfsm* me, hfsm_event event);
#ifdef USE_PROVISIONING
static int provisioning(hfsm* me, hfsm_event event);
#endif
static int hub(hfsm* me, hfsm_event event);

// Hardcoded AzureIoT hierarchy structure
static state_handler azure_iot_hfsm_get_parent(state_handler child_state)
{
   state_handler parent_state;

  if ((child_state == azure_iot))
  {
    parent_state = NULL;
  }
  else if (
#ifdef USE_PROVISIONING
      (child_state == provisioning) ||
#endif
      (child_state == hub) || (child_state == idle)
    )
  {
    parent_state = azure_iot;
  }
  else {
    // Unknown state.
    configASSERT(0);
    parent_state = NULL;
  }

  return parent_state;
}

// AzureIoT
static int azure_iot(hfsm* me, hfsm_event event)
{
  int ret = 0;
  az_iot_hfsm_type* thisiothfsm = (az_iot_hfsm_type*)me;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      LogInfo( ("AzureIoT: Entry") );
      thisiothfsm->_use_secondary_credentials = false;
      break;

    case HFSM_EXIT:
    case ERROR:
    case TIMEOUT:
    default:
      LogInfo( ("AzureIoT: PANIC!") );
      // Critical error: (memory corruption likely) reboot device.
      //TODO: az_iot_hfsm_pal_critical();
      configASSERT(0);
      break;
  }

  return ret;
}

static int idle(hfsm* me, hfsm_event event)
{
  int ret = 0;
  az_iot_hfsm_type* thisiothfsm = (az_iot_hfsm_type*)me;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      LogInfo( ("idle: Entry") );
      break;

    case HFSM_EXIT:
      LogInfo( ("idle: Exit") );
      break;
    
    case AZ_IOT_START:
#ifdef USE_PROVISIONING
      az_iot_hfsm_pal_provisioning_start(me, thisiothfsm->_provisioning_handle, thisiothfsm->_use_secondary_credentials);
      ret = hfsm_transition_substate(me, azure_iot, provisioning);
#else
      az_iot_hfsm_pal_hub_start();
      ret = hfsm_transition_substate(me, azure_iot, hub);
#endif
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

#ifdef USE_PROVISIONING
// AzureIoT/Provisioning
static int provisioning(hfsm* me, hfsm_event event)
{
  int ret = 0;
  az_iot_hfsm_type* thisiothfsm = (az_iot_hfsm_type*)me;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      LogInfo( ("provisioning: Entry") );
      thisiothfsm->_retry_count = 0;
      thisiothfsm->_start_seconds = az_hfsm_pal_timer_get_miliseconds();
      thisiothfsm->_timer_handle = az_iot_hfsm_pal_timer_create();
      break;

    case HFSM_EXIT:
      LogInfo( ("provisioning: Exit") );
      az_iot_hfsm_pal_timer_destroy(thisiothfsm->_timer_handle);
      break;

    case AZ_IOT_ERROR:
      // TODO: error_data = 

      thisiothfsm->_retry_count++;
      int elapsedSeconds = az_iot_hfsm_pal_timer_get_seconds() - thisiothfsm->_start_seconds;
      int retry_delay = 0; // TODO : calculate retry delay.

      // TODO: retriable error, start timers
      break;

    case TIMEOUT:
      thisiothfsm->_start_seconds = az_iot_hfsm_pal_timer_get_seconds();
      az_iot_hfsm_pal_provisioning_start(me, thisiothfsm->_provisioning_handle, thisiothfsm->_use_secondary_credentials);
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}
#endif //USE_PROVISIONING

// AzureIoT/Hub
static int hub(hfsm* me, hfsm_event event)
{
  int ret = 0;
  az_iot_hfsm_type* thisiothfsm = (az_iot_hfsm_type*)me;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      LogInfo( ("hub: Entry") );
      thisiothfsm->_retry_count = 0;
      thisiothfsm->_start_seconds = az_hfsm_pal_timer_get_miliseconds();
      thisiothfsm->_timer_handle = az_iot_hfsm_pal_timer_create(me);
      break;

    case HFSM_EXIT:
      LogInfo( ("hub: Exit") );
      az_iot_hfsm_pal_timer_destroy(thisiothfsm->_timer_handle);
      break;

    case AZ_IOT_ERROR:
      // TODO: error_data = 
      //TODO:  if retriable

      break;

    case TIMEOUT:
      thisiothfsm->_start_seconds = az_iot_hfsm_pal_timer_get_seconds();
      az_iot_hfsm_pal_hub_start(me, thisiothfsm->_iothub_handle, thisiothfsm->_use_secondary_credentials);
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

/**
 * @brief 
 * 
 * @param handle 
 * @return int 
 */
int az_iot_hfsm_initialize(az_iot_hfsm_type* handle)
{
  int ret = 0;
  ret = hfsm_init((hfsm*)(handle), azure_iot, azure_iot_hfsm_get_parent);
  
  // Transition to initial state.
  if (!ret)
  {
    ret = hfsm_transition_substate((hfsm*)(handle), azure_iot, idle);
  }

  return ret;
}

/**
 * @brief 
 * 
 * @param handle 
 * @param event 
 * @return int 
 */
int az_iot_hfsm_post_sync(az_iot_hfsm_type* handle, hfsm_event event)
{
  return hfsm_post_event((hfsm*)(&handle), event);
}
