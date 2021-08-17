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

#define samplegsgdeviceTELEMETRY_HUMIDITY          ( "humidity" )
#define samplegsgdeviceTELEMETRY_TEMPERATURE       ( "temperature" )
#define samplegsgdeviceTELEMETRY_PRESSURE          ( "pressure" )
#define samplegsgdeviceTELEMETRY_MAGNETOMETERX     ( "magnetometerX" )
#define samplegsgdeviceTELEMETRY_MAGNETOMETERY     ( "magnetometerY" )
#define samplegsgdeviceTELEMETRY_MAGNETOMETERZ     ( "magnetometerZ" )
#define samplegsgdeviceTELEMETRY_ACCELEROMETERX    ( "accelerometerX" )
#define samplegsgdeviceTELEMETRY_ACCELEROMETERY    ( "accelerometerY" )
#define samplegsgdeviceTELEMETRY_ACCELEROMETERZ    ( "accelerometerZ" )
#define samplegsgdeviceTELEMETRY_GYROSCOPEX        ( "gyroscopeX" )
#define samplegsgdeviceTELEMETRY_GYROSCOPEY        ( "gyroscopeY" )
#define samplegsgdeviceTELEMETRY_GYROSCOPEZ        ( "gyroscopeZ" )

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
    float xTemperature = BSP_TSENSOR_ReadTemp();
    float xHumidity = BSP_HSENSOR_ReadHumidity();
    float xPressure = BSP_PSENSOR_ReadPressure();

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_TEMPERATURE, sizeof( samplegsgdeviceTELEMETRY_TEMPERATURE ) - 1, xTemperature, 2 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_HUMIDITY, sizeof( samplegsgdeviceTELEMETRY_HUMIDITY ) - 1, xHumidity, 2 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_PRESSURE, sizeof( samplegsgdeviceTELEMETRY_PRESSURE ) - 1, xPressure, 2 );
    configASSERT( xResult == eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void prvCreateTelemetryMagnetometer( AzureIoTJSONWriter_t * xWriter )
{
    AzureIoTResult_t xResult;
    int16_t usMagnetometer[ 3 ];

    BSP_MAGNETO_GetXYZ( usMagnetometer );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_MAGNETOMETERX, sizeof( samplegsgdeviceTELEMETRY_MAGNETOMETERX ) - 1, usMagnetometer[ 0 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_MAGNETOMETERY, sizeof( samplegsgdeviceTELEMETRY_MAGNETOMETERY ) - 1, usMagnetometer[ 1 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_MAGNETOMETERZ, sizeof( samplegsgdeviceTELEMETRY_MAGNETOMETERZ ) - 1, usMagnetometer[ 2 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void prvCreateTelemetryAccelerometer( AzureIoTJSONWriter_t * xWriter )
{
    AzureIoTResult_t xResult;
    int16_t usAccelerometer[ 3 ];

    BSP_ACCELERO_AccGetXYZ( usAccelerometer );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_ACCELEROMETERX, sizeof( samplegsgdeviceTELEMETRY_ACCELEROMETERX ) - 1, usAccelerometer[ 0 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_ACCELEROMETERY, sizeof( samplegsgdeviceTELEMETRY_ACCELEROMETERY ) - 1, usAccelerometer[ 1 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_ACCELEROMETERZ, sizeof( samplegsgdeviceTELEMETRY_ACCELEROMETERZ ) - 1, usAccelerometer[ 2 ], 0 );
    configASSERT( xResult == eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

static void prvCreateTelemetryGyroscope( AzureIoTJSONWriter_t * xWriter )
{
    AzureIoTResult_t xResult;
    float xGyroscope[ 3 ];

    BSP_GYRO_GetXYZ( xGyroscope );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_GYROSCOPEX, sizeof( samplegsgdeviceTELEMETRY_GYROSCOPEX ) - 1, xGyroscope[ 0 ], 2 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_GYROSCOPEY, sizeof( samplegsgdeviceTELEMETRY_GYROSCOPEY ) - 1, xGyroscope[ 1 ], 2 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( xWriter, ( uint8_t * ) samplegsgdeviceTELEMETRY_GYROSCOPEZ, sizeof( samplegsgdeviceTELEMETRY_GYROSCOPEZ ) - 1, xGyroscope[ 2 ], 2 );
    configASSERT( xResult == eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

void vSetLedState( bool xLevel )
{
    if( xLevel )
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

    return ( uint32_t ) lBytesWritten;
}
/*-----------------------------------------------------------*/
