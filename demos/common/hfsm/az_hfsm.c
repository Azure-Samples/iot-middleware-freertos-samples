/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm.c
 * @brief Hierarchical Finite State Machine implementation.
 */

#include "FreeRTOS.h"
#include "az_hfsm.h"

const hfsm_event hfsm_entry_event = { HFSM_ENTRY, NULL };
const hfsm_event hfsm_exit_event = { HFSM_EXIT, NULL };

/**
 * @brief Initializes the HFSM.
 * 
 * @param[in] h The HFSM handle.
 * @param initial_state The initial state for this HFSM.
 * @param get_parent_func The function describing the HFSM structure.
 * @return int 
 */
int hfsm_init(hfsm* h, state_handler initial_state, get_parent get_parent_func)
{
  configASSERT(h != NULL);
  h->current_state = initial_state;
  h->get_parent_func = get_parent_func;
  return h->current_state(h, hfsm_entry_event);
}

static int _hfsm_recursive_exit(
    hfsm* h,
    state_handler source_state)
{
  configASSERT(h != NULL);
  configASSERT(source_state != NULL);

  int ret = 0;
  // Super-state handler making a transition must exit all substates:
  while (source_state != h->current_state)
  {
    // The current state cannot be null while walking the hierarchy chain from an sub-state to the
    // super-state:
    configASSERT(h->current_state != NULL);

    ret = h->current_state(h, hfsm_exit_event);
    state_handler super_state = h->get_parent_func(h->current_state);
    configASSERT(super_state != NULL);

    if (ret)
    {
      break;
    }

    h->current_state = super_state;
  }

  return ret;
}

/**
 * @brief Transition to peer.
 * 
 * @details     Supported transitions limited to the following:
 *              - peer states (within the same top-level state).
 *              - super state transitioning to another peer state (all sub-states will exit).
 * @param[in] h The HFSM handle.
 * @param source_state The source state.
 * @param destination_state The destination state.
 * @return int Non-zero return on error.
 */
int hfsm_transition_peer(hfsm* h, state_handler source_state, state_handler destination_state)
{
  configASSERT(h != NULL);
  configASSERT(source_state != NULL);
  configASSERT(destination_state != NULL);

  int ret = 0;
  // Super-state handler making a transition must exit all inner states:
  ret = _hfsm_recursive_exit(h, source_state);

  if (source_state == h->current_state)
  {
    // Exit the source state.
    ret = h->current_state(h, hfsm_exit_event);
    if (!ret)
    {
      // Enter the destination state:
      h->current_state = destination_state;
      ret = h->current_state(h, hfsm_entry_event);
    }
  }

  return ret;
}

/**
 * @brief Transition to sub-state.
 * 
 * @details Supported transitions limited to the following:
 *          - peer state transitioning to first-level sub-state.
 *          - super state transitioning to another first-level sub-state.
 * @param h The HFSM handle.
 * @param source_state The source state.
 * @param destination_state The destination state.
 * @return int Non-zero return on error.
 */
int hfsm_transition_substate(
    hfsm* h,
    state_handler source_state,
    state_handler destination_state)
{
  configASSERT(h != NULL);
  configASSERT(h->current_state != NULL);
  configASSERT(source_state != NULL);
  configASSERT(destination_state != NULL);

  int ret;
  // Super-state handler making a transition must exit all inner states:
  ret = _hfsm_recursive_exit(h, source_state);

  if (source_state == h->current_state)
  {
    // Transitions to sub-states will not exit the super-state:
    h->current_state = destination_state;
    ret = h->current_state(h, hfsm_entry_event);
  }

  return ret;
}


/**
 * @brief Transition to super-state
 *
 * @details Supported transitions limited to 
 *          - state transitioning to first-level super-state.
 *          - super-state transitioning to its immediate super-state.
 * @param h The HFSM handle.
 * @param source_state The source state.
 * @param destination_state The destination state.
 * @return int Non-zero return on error.
 */
int hfsm_transition_superstate(
    hfsm* h,
    state_handler source_state,
    state_handler destination_state)
{
  configASSERT(h != NULL);
  configASSERT(h->current_state != NULL);
  configASSERT(source_state != NULL);
  configASSERT(destination_state != NULL);

  int ret;
  // Super-state handler making a transition must exit all inner states:
  ret = _hfsm_recursive_exit(h, source_state);

  if (source_state == h->current_state)
  {
    // Transitions to super states will exit the substate but not enter the superstate again:
    ret = h->current_state(h, hfsm_exit_event);
    h->current_state = destination_state;
  }

  return ret;
}

/**
 * @brief Post synchronous event.
 * 
 * @param h The HFSM handle.
 * @param event The posted event.
 * @return int Non-zero return on error.
 */
int hfsm_post_event(hfsm* h, hfsm_event event)
{
  configASSERT(h != NULL);
  int ret;

  ret = h->current_state(h, event);

  state_handler current = h->current_state;
  while (ret == HFSM_RET_HANDLE_BY_SUPERSTATE)
  {
    state_handler super = h->get_parent_func(current);
    configASSERT(super != NULL);
    current = super;
    ret = current(h, event);
  }

  return ret;
}
