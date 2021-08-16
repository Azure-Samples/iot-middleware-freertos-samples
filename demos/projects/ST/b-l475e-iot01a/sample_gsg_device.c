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

#define TELEMETRY_HUMIDITY          ( "humidity" )
#define TELEMETRY_TEMPERATURE       ( "temperature" )
#define TELEMETRY_PRESSURE          ( "pressure" )
#define TELEMETRY_MAGNETOMETERX     ( "magnetometerX" )
#define TELEMETRY_MAGNETOMETERY     ( "magnetometerY" )
#define TELEMETRY_MAGNETOMETERZ     ( "magnetometerZ" )
#define TELEMETRY_ACCELEROMETERX    ( "accelerometerX" )
#define TELEMETRY_ACCELEROMETERY    ( "accelerometerY" )
#define TELEMETRY_ACCELEROMETERZ    ( "accelerometerZ" )
#define TELEMETRY_GYROSCOPEX        ( "gyroscopeX" )
#define TELEMETRY_GYROSCOPEY        ( "gyroscopeY" )
#define TELEMETRY_GYROSCOPEZ        ( "gyroscopeZ" )

const char * pcModelId = "dtmi:azureiot:devkit:freertos:gsgstml475;2";

const char * pcManufacturerPropertyValue = "STMicroelectronics";
const char * pcModelPropertyValue = "B-L475E-IOT01A";
const char * pcSoftwareVersionPropertyValue = "1.0.0";
const char * pcOsNamePropertyValue = "FreeRTOS";
const char * pcProcessorArchitecturePropertyValue = "Arm Cortex M4";
const char * pcProcessorManufacturerPropertyValue = "STMicroelectronics";
const double xTotalStoragePropertyValue = 8192;
const double xTotalMemoryPropertyValue = 768;

typedef enum TelemetryStateType_t
{
    eTelemetryStateTypeDefault,
    eTelemetryStateTypeMagnetometer,
    eTelemetryStateTypeAccelerometer,
    eTelemetryStateTypeGyroscope,
    eTelemetryStateTypeEnd,
} TelemetryStateType_t;

static TelemetryStateType_t xTelemetryState = eTelemetryStateTypeDefault;
/*-----------------------------------------------------------*/

static void prvCreateTelemetryDevice( AzureIoTJSONWriter_t * xWriter )
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
/*-----------------------------------------------------------*/

static void prvCreateTelemetryMagnetometer( AzureIoTJSONWriter_t * xWriter )
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
/*-----------------------------------------------------------*/

static void prvCreateTelemetryAccelerometer( AzureIoTJSONWriter_t * xWriter )
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
/*-----------------------------------------------------------*/

static void prvCreateTelemetryGyroscope( AzureIoTJSONWriter_t * xWriter )
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
/*-----------------------------------------------------------*/

void vSetLedState( bool level )
{
    if( level )
    {
        LogInfo( ( "LED is turned ON" ) );
        BSP_LED_On( LED_GREEN );
    }
    else
    {
        LogInfo( ( "LED is turned OFF" ) );
        BSP_LED_Off( LED_GREEN );
    }
}
/*-----------------------------------------------------------*/

uint32_t ulCreateTelemetry( uint8_t * pucTelemetryData,
                            uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    switch( xTelemetryState )
    {
        case eTelemetryStateTypeDefault:
            prvCreateTelemetryDevice( &xWriter );
            break;

        case eTelemetryStateTypeMagnetometer:
            prvCreateTelemetryMagnetometer( &xWriter );
            break;

        case eTelemetryStateTypeAccelerometer:
            prvCreateTelemetryAccelerometer( &xWriter );
            break;

        case eTelemetryStateTypeGyroscope:
            prvCreateTelemetryGyroscope( &xWriter );
            break;

        default:
            break;
    }

    xTelemetryState = ( xTelemetryState + 1 ) % eTelemetryStateTypeEnd;

    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    LogInfo( ( "Telemetry message sent %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return lBytesWritten;
}
/*-----------------------------------------------------------*/
