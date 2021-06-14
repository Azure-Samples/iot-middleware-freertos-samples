/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm.h
 * @brief Hierarchical Finite State Machine implementation.
 */

#ifndef AZ_HFSM_H
#define AZ_HFSM_H

#include "FreeRTOS.h"

/**
 * @brief hfsm event type.
 *
 */
typedef enum
{
  HFSM_ENTRY = 1,
  HFSM_EXIT = 2,
  HFSM_ERROR = 3,
  HFSM_TIMEOUT = 4,
  HFSM_EVENT_BASE = 10,
} hfsm_event_type;

#define HFSM_EVENT(id) ((int32_t)(HFSM_EVENT_BASE + id))

struct hfsm_event
{
  hfsm_event_type type;
  void* data;
};

typedef struct hfsm_event hfsm_event;

extern const hfsm_event hfsm_entry_event;
extern const hfsm_event hfsm_exit_event;

typedef struct hfsm hfsm;
typedef int (*state_handler)(hfsm* me, hfsm_event event);
typedef state_handler (*get_parent)(state_handler child_state);

struct hfsm
{
  state_handler current_state;
  get_parent get_parent_func;
};

#define HFSM_RET_HANDLE_BY_SUPERSTATE -1

/**
 * @brief Initializes the HFSM.
 * 
 * @param[in] h The HFSM handle.
 * @param initial_state The initial state for this HFSM.
 * @param get_parent_func The function describing the HFSM structure.
 * @return int 
 */
int hfsm_init(
    hfsm* h,
    state_handler initial_state,
    get_parent get_parent_func);


int hfsm_transition_peer(
    hfsm* h,
    state_handler source_state,
    state_handler destination_state);

int hfsm_transition_substate(
    hfsm* h,
    state_handler source_state,
    state_handler destination_state);


int hfsm_transition_superstate(
    hfsm* h,
    state_handler source_state,
    state_handler destination_state);

int hfsm_post_event(hfsm* h, hfsm_event event);

#endif //AZ_HFSM_H
