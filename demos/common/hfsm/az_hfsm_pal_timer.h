/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm_pal_timer_freertos.c
 * @brief HFSM Platform Adaptation Layer for FreeRTOS
 */

#include <stdint.h>
#include <stdbool.h>

// az_iot_hfsm_pal_timer

/**
 * @brief 
 * 
 * @param me 
 * @return void* 
 */
void* az_hfsm_pal_timer_create(hfsm me);

/**
 * @brief 
 * 
 * @param me 
 * @param timer_handle 
 * @param seconds 
 * @param oneshot 
 * @return int 
 */
int az_hfsm_pal_timer_start(hfsm me, void* timer_handle, uint32_t seconds, bool oneshot);

/**
 * @brief 
 * 
 * @param me 
 * @param timer_handle 
 * @return int 
 */
int az_hfsm_pal_timer_stop(hfsm me, void* timer_handle);

/**
 * @brief 
 * 
 * @param me 
 * @param timer_handle 
 */
void az_hfsm_pal_timer_destroy(hfsm me, void* timer_handle);

inline void az_hfsm_

/**
 * @brief 
 * 
 * @return int32_t 
 */
int32_t az_hfsm_pal_timer_get_miliseconds();
