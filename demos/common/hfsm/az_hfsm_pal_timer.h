/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm_pal_timer_freertos.c
 * @brief HFSM Platform Adaptation Layer for FreeRTOS
 */

#include <stdint.h>
#include <stdbool.h>

#include "az_hfsm.h"

// az_iot_hfsm_pal_timer

/**
 * @brief 
 * 
 * @param me 
 * @return void* 
 */
void* az_hfsm_pal_timer_create(hfsm* src);

/**
 * @brief 
 * 
 * @param me 
 * @param timer_handle 
 * @param seconds 
 * @param oneshot 
 * @return int 
 */
int az_hfsm_pal_timer_start(hfsm* src, void* timer_handle, int32_t milliseconds, bool oneshot);

/**
 * @brief 
 * 
 * @param me 
 * @param timer_handle 
 * @return int 
 */
int az_hfsm_pal_timer_stop(hfsm* src, void* timer_handle);

/**
 * @brief 
 * 
 * @param me 
 * @param timer_handle 
 */
void az_hfsm_pal_timer_destroy(hfsm* src, void* timer_handle);

inline int az_hfsm_pal_timer_notify(hfsm* dst)
{
    return hfsm_post_event(dst, hfsm_timeout_event);
}

uint64_t az_hfsm_pal_timer_get_miliseconds();
