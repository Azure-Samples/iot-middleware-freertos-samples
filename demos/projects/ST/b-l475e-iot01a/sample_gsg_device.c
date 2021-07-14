/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "stm32l475e_iot01.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_psensor.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01_magneto.h"
#include "stm32l475e_iot01_accelero.h"


/* Standard includes. */
#include <stdbool.h>
#include <stdint.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* Azure JSON includes */
#include "azure_iot_json_writer.h"

#define TELEMETRY_HUMIDITY          "humidity"
#define TELEMETRY_TEMPERATURE       "temperature"
#define TELEMETRY_PRESSURE          "pressure"
#define TELEMETRY_MAGNETOMETERX     "magnetometerX"
#define TELEMETRY_MAGNETOMETERY     "magnetometerY"
#define TELEMETRY_MAGNETOMETERZ     "magnetometerZ"
#define TELEMETRY_ACCELEROMETERX    "accelerometerX"
#define TELEMETRY_ACCELEROMETERY    "accelerometerY"
#define TELEMETRY_ACCELEROMETERZ    "accelerometerZ"
#define TELEMETRY_GYROSCOPEX        "gyroscopeX"
#define TELEMETRY_GYROSCOPEY        "gyroscopeY"
#define TELEMETRY_GYROSCOPEZ        "gyroscopeZ"

typedef enum TelemetryStateType_t
{
    eTelemetryStateTypeDefault,
    eTelemetryStateTypeMagnetometer,
    eTelemetryStateTypeAccelerometer,
    eTelemetryStateTypeGyroscope,
    eTelemetryStateTypeEnd,
} TelemetryStateType_t;

static TelemetryStateType_t telemetryState = eTelemetryStateTypeDefault;

const char * manufacturer_property_value = "STMicroelectronics";
const char * model_property_value = "B-L4S5I-IOT01A";
const char * software_version_property_value = "1.0.0";
const char * os_name_property_value = "FreeRTOS";
const char * processor_architecture_property_value = "Arm Cortex M4";
const char * processor_manufacturer_property_value = "STMicroelectronics";
const double total_storage_property_value = 8192;
const double total_memory_property_value = 768;

static void createTelemetryDevice( AzureIoTJSONWriter_t * xWriter )
{
    AzureIoTResult_t xResult;
    float temperature = BSP_TSENSOR_ReadTemp();
    float humidity = BSP_HSENSOR_ReadHumidity();
    float pressure = BSP_PSENSOR_ReadPressure();

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_TEMPERATURE, sizeof( TELEMETRY_TEMPERATURE ) - 1, temperature, 2 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_HUMIDITY, sizeof( TELEMETRY_HUMIDITY ) - 1, humidity, 2 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_PRESSURE, sizeof( TELEMETRY_PRESSURE ) - 1, pressure, 2 );
    configASSERT( xResult == eAzureIoTSuccess );
}

static void createTelemetryMagnetometer( AzureIoTJSONWriter_t * xWriter )
{
    AzureIoTResult_t xResult;
    int16_t magnetometer[ 3 ];

    BSP_MAGNETO_GetXYZ( magnetometer );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_MAGNETOMETERX, sizeof( TELEMETRY_MAGNETOMETERX ) - 1, magnetometer[ 0 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_MAGNETOMETERY, sizeof( TELEMETRY_MAGNETOMETERY ) - 1, magnetometer[ 1 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_MAGNETOMETERZ, sizeof( TELEMETRY_MAGNETOMETERZ ) - 1, magnetometer[ 2 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );
}

static void createTelemetryAccelerometer( AzureIoTJSONWriter_t * xWriter )
{
    AzureIoTResult_t xResult;
    int16_t accelerometer[ 3 ];

    BSP_ACCELERO_AccGetXYZ( accelerometer );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_ACCELEROMETERX, sizeof( TELEMETRY_ACCELEROMETERX ) - 1, accelerometer[ 0 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_ACCELEROMETERY, sizeof( TELEMETRY_ACCELEROMETERY ) - 1, accelerometer[ 1 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_ACCELEROMETERZ, sizeof( TELEMETRY_ACCELEROMETERZ ) - 1, accelerometer[ 2 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );
}

static void createTelemetryGyroscope( AzureIoTJSONWriter_t * xWriter )
{
    AzureIoTResult_t xResult;
    float gyroscope[ 3 ];

    BSP_GYRO_GetXYZ( gyroscope );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_GYROSCOPEX, sizeof( TELEMETRY_GYROSCOPEX ) - 1, gyroscope[ 0 ], 2 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_GYROSCOPEY, sizeof( TELEMETRY_GYROSCOPEY ) - 1, gyroscope[ 1 ], 2 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) TELEMETRY_GYROSCOPEZ, sizeof( TELEMETRY_GYROSCOPEZ ) - 1, gyroscope[ 2 ], 2 );
    configASSERT( xResult == eAzureIoTSuccess );
}

void setLedState( bool level )
{
    if( level )
    {
        BSP_LED_On( LED_GREEN );
    }
    else
    {
        BSP_LED_Off( LED_GREEN );
    }
}

uint32_t createTelemetry( uint8_t * pucTelemetryData,
                          uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    switch( telemetryState )
    {
        case eTelemetryStateTypeDefault:
            createTelemetryDevice( &xWriter );
            break;

        case eTelemetryStateTypeMagnetometer:
            createTelemetryMagnetometer( &xWriter );
            break;

        case eTelemetryStateTypeAccelerometer:
            createTelemetryAccelerometer( &xWriter );
            break;

        case eTelemetryStateTypeGyroscope:
            createTelemetryGyroscope( &xWriter );
            break;

        default:
            break;
    }

    telemetryState = ( telemetryState + 1 ) % eTelemetryStateTypeEnd;

    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    return lBytesWritten;
}
