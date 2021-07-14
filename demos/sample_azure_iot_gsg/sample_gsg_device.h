/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef __TASK_GSG_DEVICE_H
#define __TASK_GSG_DEVICE_H

#include <stdbool.h>
#include <stdint.h>

extern const char * pcManufacturerPropertyValue;
extern const char * pcModelPropertyValue;
extern const char * pcSoftwareVersionPropertyValue;
extern const char * pcOsNamePropertyValue;
extern const char * pcProcessorArchitecturePropertyValue;
extern const char * pcProcessorManufacturerPropertyValue;
extern const double pxTotalStoragePropertyValue;
extern const double pxTotalMemoryPropertyValue;

void setLedState( bool level );

uint32_t createTelemetry( uint8_t * pucTelemetryData,
                          uint32_t ulTelemetryDataLength );

#endif /* ifndef __TASK_GSG_DEVICE_H */
