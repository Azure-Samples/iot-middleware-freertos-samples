/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm_sync_adapter.h
 * @brief Adapter for HFSM syncrhonous operation.
 * 
 * @details This adapter provides a way to convert from syncrhonous IoT Provisioning and Hub
 *          operations to asyncrhonous HFSM events. The application must implement the PAL functions.
 *          A single Provisioning + Hub client is supported in syncrhonous mode.
 */

#ifndef _az_IOT_HFSM_SYNC_ADAPTER_H
#define _az_IOT_HFSM_SYNC_ADAPTER_H

#include "az_iot_hfsm.h"

int32_t az_iot_hfsm_sync_adapter_sync_initialize();
void az_iot_hfsm_sync_adapter_sync_do_work();

// PAL:
void az_iot_hfsm_sync_adapter_pal_set_credentials( bool use_secondary );
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
az_iot_hfsm_event_data_error az_iot_hfsm_sync_adapter_pal_run_provisioning( );
#endif
az_iot_hfsm_event_data_error az_iot_hfsm_sync_adapter_pal_run_hub( );
void az_iot_hfsm_sync_adapter_sleep( int32_t milliseconds );

#endif //_az_IOT_HFSM_SYNC_ADAPTER_H
