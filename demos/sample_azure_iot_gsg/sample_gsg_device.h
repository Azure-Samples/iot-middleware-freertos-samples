/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef __TASK_GSG_DEVICE_H
#define __TASK_GSG_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

extern const char * manufacturer_property_value;
extern const char * model_property_value;
extern const char * software_version_property_value;
extern const char * os_name_property_value;
extern const char * processor_architecture_property_value;
extern const char * processor_manufacturer_property_value;
extern const double total_storage_property_value;
extern const double total_memory_property_value;

void setLedState( bool level );

uint32_t createTelemetry( uint8_t * pucTelemetryData,
                          uint32_t ulTelemetryDataLength );

#endif /* ifndef __TASK_GSG_DEVICE_H */
