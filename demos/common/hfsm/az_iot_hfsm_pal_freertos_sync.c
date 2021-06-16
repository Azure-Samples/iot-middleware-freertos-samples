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


int (*next_operation)(void);

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
    int ret = 0;

    hfsm_event az_iot_hfsm_event_iot_start = { AZ_IOT_START, NULL };
    az_iot_hfsm_post_sync(&prvIoTHfsm, az_iot_hfsm_event_iot_start);

    az_iot_hfsm_event_data_error unknown_error = { false, false, false, AZ_IOT_STATUS_UNKNOWN };

    for ( ; ; )
    {
        ret = next_operation();
    }

}

// az_iot_hfsm_pal_timer
void* az_hfsm_pal_timer_create(hfsm me)
{
    (void) me;
    // NOOP.
}

static uint32_t delay_miliseconds
static int do_timer_operation()
{
    vTaskDelay (pdMS_TO_TICKS(delay_miliseconds));
    return az_iot_hfsm_post_sync(&prvIoTHfsm, hfsm_timeout_event);
}

int az_hfsm_pal_timer_start(hfsm me, void* timer_handle, uint32_t miliseconds, bool oneshot)
{
    (void) me;
    (void) timer_handle;
    configAssert (oneshot == true);
    next_operation = &do_timer_operation;
    return 0;
}

int az_hfsm_pal_timer_stop(hfsm me, void* timer_handle)
{
    (void) me;
    (void) timer_handle;
    // NOOP.
    return 0;
}

void az_hfsm_pal_timer_destroy(hfsm me, void* timer_handle)
{
    (void) me;
    (void) timer_handle;
    // NOOP.
}

uint64_t ullGetUnixTime( void );

uint64_t az_hfsm_pal_timer_get_miliseconds()
{
    return ullGetUnixTime();
}

static int do_provisioning()
{
    uint32_t ret;
    ret = prvDeviceProvisioningRun();
    //AZ_IOT_PROVISIONING_DONE or AZ_IOT_ERROR
    //return az_iot_hfsm_post_sync(&prvIoTHfsm, hfsm_);
}

int az_iot_hfsm_pal_provisioning_start(hfsm* caller, void* provisioning_handle, bool use_secondary_credentials)
{
    (void) caller;
    next_operation = &;
    return 0;
}

static int do_hub()
{
    uint32_t ret;
    ret = prvIoTHubRun();
    //AZ_IOT_PROVISIONING_DONE or AZ_IOT_ERROR
    //return az_iot_hfsm_post_sync(&prvIoTHfsm, hfsm_);
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
    next_operation = &prvIoTHubRun;
    return 0;
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
void az_iot_hfsm_pal_critical(hfsm* caller, az_iot_hfsm_event_data_error error)
{
    (void) caller;
    Error_Handler();
}
