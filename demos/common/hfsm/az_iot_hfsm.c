
#include <stdint.h>
#include <assert.h>
#include <azure/az_core.h>

#include <az_iot_hfsm.h>

static hfsm az_iot_hfsm;

#define USE_PROVISIONING  // Comment out if Azure IoT Provisioning is not used.

static int azure_iot(hfsm* me, hfsm_event event);
#ifdef USE_PROVISIONING
static int provisioning(hfsm* me, hfsm_event event);
static int retry_provisioning(hfsm* me, hfsm_event event);
#endif
static int hub(hfsm* me, hfsm_event event);
static int retry_hub(hfsm* me, hfsm_event event);

static int credential_idx;

// Hardcoded AzureIoT hierarchy structure
static state_handler azure_iot_hfsm_get_parent(state_handler child_state)
{
   state_handler parent_state;

  if ((child_state == azure_iot))
  {
    parent_state = NULL;
  }
  else if (
#ifdef USE_PROVISIONING
      (child_state == provisioning) || (child_state == retry_provisioning) ||
#endif
      (child_state == hub) || (child_state == retry_hub) 
    )
  {
    parent_state = azure_iot;
  }
  else {
    // Unknown state.
    assert(false);
    parent_state = NULL;
  }

  return parent_state;
}

// AzureIoT
static int azure_iot(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      credential_idx = 0;
      break;

    case HFSM_EXIT:
      break;

#ifdef USE_PROVISIONING
    case AZ_IOT_PROVISIONING_START:
      ret = hfsm_transition_substate(me, azure_iot, provisioning);
      break;
#else
    case AZ_IOT_HUB_START:
      ret = hfsm_transition_substate(me, azure_iot, hub);
      break;
#endif    

    case ERROR:
    case TIMEOUT:
    default:
      // Critical error: (memory corruption likely) reboot device.
      assert(0);
      break;
  }

  return ret;
}

#ifdef USE_PROVISIONING
// AzureIoT/Provisioning
static int provisioning(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      // ret = init_provisioning(credential_idx);
      break;

    case HFSM_EXIT:
      // ret = deinit_provisioning();
      break;

    case AZ_IOT_PROVISIONING_START:
      // ret = start_provisioning();
      break;

    case AZ_IOT_HUB_START:
      // TODO: pass-through the hfsm_event event:
      ret = hfsm_transition_peer(me, provisioning, hub);
      break;

    case ERROR:
    case TIMEOUT:
      // TODO: pass-through the hfsm_event event:
      ret = hfsm_transition_peer(me, provisioning, retry_provisioning);
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoT/RetryProvisioning
static int retry_provisioning(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      break;

    case HFSM_EXIT:
      break;

    case AZ_IOT_PROVISIONING_START:
      ret = hfsm_transition_peer(me, retry_provisioning, provisioning);
      break;

    case ERROR:
      // TODO:

      break;

    case TIMEOUT:
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}
#endif //USE_PROVISIONING

// AzureIoT/Hub
static int hub(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      break;

    case HFSM_EXIT:
      break;

    case ERROR:
    case TIMEOUT:
      // TODO: pass-through the hfsm_event event:
      ret = hfsm_transition_peer(me, hub, retry_hub);
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoT/RetryHub
static int retry_hub(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      break;

    case HFSM_EXIT:
      break;

    case AZ_IOT_HUB_START:
      // TODO: pass-through the hfsm_event event:
      ret = hfsm_transition_peer(me, hub, retry_hub);
      break;

    default:
      ret = HFSM_RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}
