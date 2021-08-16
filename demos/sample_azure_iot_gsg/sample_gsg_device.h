/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef __TASK_GSG_DEVICE_H
#define __TASK_GSG_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

extern const char * pcModelId;

extern const char * pcManufacturerPropertyValue;
extern const char * pcModelPropertyValue;
extern const char * pcSoftwareVersionPropertyValue;
extern const char * pcOsNamePropertyValue;
extern const char * pcProcessorArchitecturePropertyValue;
extern const char * pcProcessorManufacturerPropertyValue;
extern const double xTotalStoragePropertyValue;
extern const double xTotalMemoryPropertyValue;

void vSetLedState( bool level );

uint32_t ulCreateTelemetry( uint8_t * pucTelemetryData,
                          uint32_t ulTelemetryDataLength );

#endif /* ifndef __TASK_GSG_DEVICE_H */
