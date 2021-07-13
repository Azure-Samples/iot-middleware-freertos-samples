/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#include "stm32l475e_iot01.h"

#include <stdbool.h>
#include <stdint.h>

#define TELEMETRY_TEMPERATURE "temperature"

typedef enum TELEMETRY_STATE_ENUM
{
    TELEMETRY_STATE_DEFAULT,
    TELEMETRY_STATE_MAGNETOMETER,
    TELEMETRY_STATE_ACCELEROMETER,
    TELEMETRY_STATE_GYROSCOPE,
    TELEMETRY_STATE_END
} TELEMETRY_STATE;

static TELEMETRY_STATE telemetryState = TELEMETRY_STATE_DEFAULT;

const char* manufacturer_property_value = "STMicroelectronics";
const char* model_property_value = "B-L4S5I-IOT01A";
const char* software_version_property_value = "1.0.0";
const char* os_name_property_value = "FreeRTOS";
const char* processor_architecture_property_value = "Arm Cortex M4";
const char* processor_manufacturer_property_value = "STMicroelectronics";
const double total_storage_property_value = 8192;
const double total_memory_property_value = 768;

static uint32_t createTelemetryDevice(uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength)
{
//    return snprintf( ( char * ) pucTelemetryData, ulTelemetryDataLength, "{sampleazureiotMESSAGE, TELEMETRY_TEMPERATURE, xDeviceCurrentTemperature );
    return 0;
}

static uint32_t createTelemetryMagnetometer(uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength)
{
    return 0;
}

static uint32_t createTelemetryAccelerometer(uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength)
{
    return 0;
}

static uint32_t createTelemetryGyroscope(uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength)
{
    return 0;
}

void setLedState(bool level)
{
    if (level)
    {
        BSP_LED_On(LED_GREEN);
    }
    else
    {
        BSP_LED_Off(LED_GREEN);
    }
}

uint32_t createTelemetry(uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength)
{
    uint32_t ulActualLength = 0;

    switch (telemetryState)
    {
        case TELEMETRY_STATE_DEFAULT:
            ulActualLength = createTelemetryDevice(pucTelemetryData, ulTelemetryDataLength);
            break;

        case TELEMETRY_STATE_MAGNETOMETER:
            ulActualLength = createTelemetryMagnetometer(pucTelemetryData, ulTelemetryDataLength);
            break;

        case TELEMETRY_STATE_ACCELEROMETER:
            ulActualLength = createTelemetryAccelerometer(pucTelemetryData, ulTelemetryDataLength);
            break;

        case TELEMETRY_STATE_GYROSCOPE:
            ulActualLength = createTelemetryGyroscope(pucTelemetryData, ulTelemetryDataLength);
            break;

        default:
            break;
    }

    telemetryState = (telemetryState + 1) % TELEMETRY_STATE_END;

    return ulActualLength;
}
