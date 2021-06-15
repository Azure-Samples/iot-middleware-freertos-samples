/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm_pal_freertos.c
 * @brief HFSM Platform Adaptation Layer for FreeRTOS
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

int32_t az_hfsm_pal_timer_get_seconds()
{
    
}
