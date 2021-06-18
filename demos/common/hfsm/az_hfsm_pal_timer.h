/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm_pal_timer.h
 * @brief Azure HFSM event based Timer PAL layer.
 */

#ifndef _az_IOT_HFSM_PAL_TIMER_H
#define _az_IOT_HFSM_PAL_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include <azure/az_core.h>

#include "az_hfsm.h"

void* az_hfsm_pal_timer_create(az_hfsm* src);
int32_t az_hfsm_pal_timer_start(az_hfsm* src, void* timer_handle, int32_t milliseconds, bool oneshot);
int32_t az_hfsm_pal_timer_stop(az_hfsm* src, void* timer_handle);
void az_hfsm_pal_timer_destroy(az_hfsm* src, void* timer_handle);

AZ_INLINE int32_t az_hfsm_pal_timer_notify(az_hfsm* dst)
{
    return az_hfsm_post_event(dst, az_hfsm_timeout_event);
}

uint64_t az_hfsm_pal_timer_get_milliseconds();

#endif //_az_IOT_HFSM_PAL_TIMER_H
