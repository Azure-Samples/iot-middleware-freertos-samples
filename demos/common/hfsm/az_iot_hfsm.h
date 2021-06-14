#ifndef AZ_IOT_HFSM_H
#define AZ_IOT_HFSM_H

#include "az_hfsm.h"

// Devices should always be provisioned with at least 2 credentials to prevent them
// from loosing connectivity with the cloud and firmware update systems.
#define CREDENTIAL_COUNT 2

// AzIoTHFSM-specific events.
typedef enum
{
  ERROR = HFSM_EVENT(1),
  TIMEOUT = HFSM_EVENT(2),
  AZ_IOT_PROVISIONING_START = HFSM_EVENT(3),
  AZ_IOT_HUB_START = HFSM_EVENT(4),
} az_iot_hsm_event_type;

// AZIoTHFSM-specific event data
typedef struct
{
    az_span mqtt_user_name;
    az_span password;
    void* client_certificate_information;
} az_iot_provisioning_start_data;

typedef struct
{
    az_span mqtt_user_name;
    az_span password;
    void* client_certificate_information;
} az_iot_hub_start_data;

// Platform Adaptation Layer (PAL)

// az_iot_hsm_pal_timer
void* az_iot_hsm_pal_timer_create();
void az_iot_hsm_pal_timer_start(void* timer_handle, int32_t seconds);
void az_iot_hsm_pal_timer_stop(void* timer_handle);
void az_iot_hsm_pal_timer_destroy(void* timer_handle);

// az_iot_hsm_pal_provisioning
void* az_iot_hsm_pal_provisioning_init();
void az_iot_hsm_pal_provisioning_start(void* provisioning_handle);
void az_iot_hsm_pal_provisioning_deinit(void* provisioning_handle);

// az_iot_hsm_pal_hub
void* az_iot_hsm_pal_hub_init();
void az_iot_hsm_pal_hub_start(void* hub_handle);
void az_iot_hsm_pal_hub_deinit(void* hub_handle);

#endif //AZ_IOT_HFSM_H
