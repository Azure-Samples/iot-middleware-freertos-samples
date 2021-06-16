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
#include "az_hfsm_pal_timer.h"
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

const hfsm_event hfsm_event_az_iot_start = { AZ_IOT_START, NULL };
const hfsm_event hfsm_event_az_iot_provisioning_done = { AZ_IOT_PROVISIONING_DONE, NULL };

static int azure_iot(hfsm* me, hfsm_event event);
static int idle(hfsm* me, hfsm_event event);
static int provisioning(hfsm* me, hfsm_event event);
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
      (child_state == provisioning) ||
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
  int32_t operation_msec;

  az_iot_hfsm_type* this_iothfsm = (az_iot_hfsm_type*)me;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      LogInfo( ("AzureIoT: Entry") );
      this_iothfsm->_use_secondary_credentials = false;
      break;

    case AZ_IOT_ERROR:
      LogInfo( ("AzureIoT: AZ_IOT_ERROR") );
      operation_msec = az_hfsm_pal_timer_get_miliseconds() - this_iothfsm->_start_time_msec;

      this_iothfsm->_retry_attempt++;
      az_iot_hfsm_event_data_error* error_data = (az_iot_hfsm_event_data_error*)(event.data);

      bool should_retry = false;
      if (error_data->type == AZ_IOT_ERROR_TYPE_SERVICE)
      {
        should_retry = az_iot_status_retriable(error_data->iot_status);
      }
      else if (error_data->type == AZ_IOT_ERROR_TYPE_NETWORK)
      {
        should_retry = true;
      }

      if (should_retry)
      {
        int random_jitter_msec = az_iot_hfsm_pal_get_random_jitter_msec(me);

        int retry_delay_msec = az_iot_calculate_retry_delay(
          operation_msec, 
          this_iothfsm->_retry_attempt,
          MIN_RETRY_DELAY_MSEC,
          MAX_RETRY_DELAY_MSEC,
          random_jitter_msec);

        ret = az_hfsm_pal_timer_start(me, this_iothfsm->_timer_handle, retry_delay_msec, true);
      }
      else
      {
        this_iothfsm->_use_secondary_credentials = !this_iothfsm->_use_secondary_credentials;
        this_iothfsm->_start_time_msec = az_hfsm_pal_timer_get_miliseconds();
        ret = hfsm_post_event(this_iothfsm->_provisioning_hfsm, hfsm_event_az_iot_start);
        ret = hfsm_transition_substate(me, azure_iot, provisioning);
      }
      break;

    case HFSM_EXIT:
    case HFSM_ERROR:
    case HFSM_TIMEOUT:
    default:
      LogInfo( ("AzureIoT: PANIC!") );
      az_iot_hfsm_pal_critical(me);
      configASSERT(0); // Should never reach here.
      break;
  }

  return ret;
}

static int idle(hfsm* me, hfsm_event event)
{
  int ret = 0;
  az_iot_hfsm_type* this_iothfsm = (az_iot_hfsm_type*)me;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      LogInfo( ("idle: Entry") );
      break;

    case HFSM_EXIT:
      LogInfo( ("idle: Exit") );
      break;
    
    case AZ_IOT_START:
      LogInfo( ("idle: AZ_IOT_START") );
      ret = hfsm_post_event(this_iothfsm->_provisioning_hfsm, hfsm_event_az_iot_start);
      configASSERT(!ret);
      ret = hfsm_transition_peer(me, idle, provisioning);
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoT/Provisioning
static int provisioning(hfsm* me, hfsm_event event)
{
  int ret = 0;
  az_iot_hfsm_type* this_iothfsm = (az_iot_hfsm_type*)me;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      LogInfo( ("provisioning: Entry") );
      this_iothfsm->_retry_attempt = 0;
      this_iothfsm->_start_time_msec = az_hfsm_pal_timer_get_miliseconds();
      this_iothfsm->_timer_handle = az_hfsm_pal_timer_create(me);
      break;

    case HFSM_EXIT:
      LogInfo( ("provisioning: Exit") );
      az_hfsm_pal_timer_destroy(me, this_iothfsm->_timer_handle);
      break;

    case HFSM_TIMEOUT:
      LogInfo( ("provisioning: Timeout") );
      this_iothfsm->_start_time_msec = az_hfsm_pal_timer_get_miliseconds();
      ret = hfsm_post_event(this_iothfsm->_provisioning_hfsm, hfsm_event_az_iot_start);
      break;
    
    case AZ_IOT_PROVISIONING_DONE:
      LogInfo( ("provisioning: Done") );
      ret = hfsm_post_event(this_iothfsm->_iothub_hfsm, hfsm_event_az_iot_start);
      ret = hfsm_transition_peer(me, provisioning, hub);
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoT/Hub
static int hub(hfsm* me, hfsm_event event)
{
  int ret = 0;
  az_iot_hfsm_type* this_iothfsm = (az_iot_hfsm_type*)me;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      LogInfo( ("hub: Entry") );
      this_iothfsm->_retry_attempt = 0;
      this_iothfsm->_start_time_msec = az_hfsm_pal_timer_get_miliseconds();
      this_iothfsm->_timer_handle = az_hfsm_pal_timer_create(me);
      break;

    case HFSM_EXIT:
      LogInfo( ("hub: Exit") );
      az_hfsm_pal_timer_destroy(me, this_iothfsm->_timer_handle);
      break;

    case HFSM_TIMEOUT:
      LogInfo( ("hub: Timeout") );
      this_iothfsm->_start_time_msec = az_hfsm_pal_timer_get_miliseconds();
      ret = hfsm_post_event(this_iothfsm->_iothub_hfsm, hfsm_event_az_iot_start);
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

/**
 * @brief 
 * 
 * @param iot_hfsm 
 * @param provisioning_hfsm 
 * @param hub_hfsm 
 * @return int 
 */
int az_iot_hfsm_initialize(az_iot_hfsm_type* iot_hfsm, hfsm* provisioning_hfsm, hfsm* hub_hfsm)
{
  int ret = 0;

  iot_hfsm->_provisioning_hfsm = provisioning_hfsm;
  iot_hfsm->_iothub_hfsm = hub_hfsm;
  ret = hfsm_init((hfsm*)(iot_hfsm), azure_iot, azure_iot_hfsm_get_parent);

  // Transition to initial state.
  if (!ret)
  {
    ret = hfsm_transition_substate((hfsm*)(iot_hfsm), azure_iot, idle);
  }

  return ret;
}
