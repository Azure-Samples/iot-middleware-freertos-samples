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

#include "az_hfsm_pal_timer.h"

// az_iot_hfsm_pal_timer
void* az_hfsm_pal_timer_create(hfsm me)
{

}

int az_hfsm_pal_timer_start(hfsm me, void* timer_handle, int32_t seconds, bool oneshot)
{

}

int az_hfsm_pal_timer_stop(hfsm me, void* timer_handle)
{

}

void az_hfsm_pal_timer_destroy(hfsm me, void* timer_handle)
{

}

uint64_t az_hfsm_pal_timer_get_miliseconds()
{
    return ullGetUnixTime() / 1000;
}

/**
 * @brief 
 * 
 * @param me The calling HFSM object.
 * @param provisioning_handle 
 * @return int 
 */
int az_iot_hfsm_pal_provisioning_start(hfsm* caller, void* provisioning_handle, bool use_secondary_credentials)
{
    (void) caller;
    LogInfo( ("Provisioning not implemented") );
    return 0;
}

/**
 * @brief 
 * 
 * @param me The calling HFSM object.
 * @param hub_handle 
 * @return int 
 */
int az_iot_hfsm_pal_hub_start(hfsm* caller, void* hub_handle, bool use_secondary_credentials)
{
    (void) caller;
    LogInfo( ("Hub not implemented") );
    return 0;
}

/**
 * @brief Critical error. This function should not return.
 * 
 * @param me The calling HFSM object.
 */
void az_iot_hfsm_pal_critical(hfsm* caller, az_iot_hfsm_event_data_error error)
{
    (void) caller;
    LogInfo( ("Hub not implemented") );
}

typedef enum
{
    AZ_IOT_HFSM_PAL_DO_PROVISIONING,
    AZ_IOT_HFSM_PAL_DO_IOT_HUB,
    AZ_IOT_HFSM_PAL_DO_SLEEP
} AZ_IOT_HFSM_PAL_EVENTS;

static AZ_IOT_HFSM_PAL_EVENTS next_operation;

// The retry Hierarchical Finite State Machine object.
static az_iot_hfsm_type prvIoTHfsm;

/**
 * @brief 
 * 
 */
void az_iot_hfsm_pal_freertos_sync_initialize()
{
    configASSERT( !az_iot_hfsm_initialize(&prvIoTHfsm) );
    
}

/**
 * @brief Message pump for syncronous operations.
 * 
 */
void az_iot_hfsm_pal_freertos_sync_do_work()
{
    hfsm_event az_iot_hfsm_event_iot_start = { AZ_IOT_START, NULL };
    az_iot_hfsm_post_sync(&prvIoTHfsm, az_iot_hfsm_event_iot_start);

    az_iot_hfsm_event_data_error unknown_error = { false, false, false, AZ_IOT_STATUS_UNKNOWN };

    for ( ; ; )
    {
        switch (next_operation)
        {
            case DO_PROVISIONING:
                
                break;

            case DO_IOT_HUB:
                break;

            case DO_SLEEP:
                break;

            default:
                az_iot_hfsm_pal_critical((hfsm*)(&prvIoTHfsm), unknown_error);
        }
    }

}
