/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "sample_azure_iot_pnp_data_if.h"
#include "azure_iot_flash_platform.h"

/* Standard includes. */
#include <string.h>
#include <stdio.h>

#include "azure_iot_adu_client.h"

/* Azure JSON includes */
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

#include "azure_iot_jws.h"

/* FreeRTOS */
/* This task provides taskDISABLE_INTERRUPTS, used by configASSERT */
#include "FreeRTOS.h"
#include "task.h"

/*-----------------------------------------------------------*/

#define sampleazureiotUPDATE_HANDLER                      "microsoft/swupdate:1"

/**
 * @brief Command values
 */
#define sampleazureiotCOMMAND_MAX_MIN_REPORT              "getMaxMinReport"
#define sampleazureiotCOMMAND_MAX_TEMP                    "maxTemp"
#define sampleazureiotCOMMAND_MIN_TEMP                    "minTemp"
#define sampleazureiotCOMMAND_TEMP_VERSION                "avgTemp"
#define sampleazureiotCOMMAND_START_TIME                  "startTime"
#define sampleazureiotCOMMAND_END_TIME                    "endTime"
#define sampleazureiotCOMMAND_EMPTY_PAYLOAD               "{}"
#define sampleazureiotCOMMAND_FAKE_END_TIME               "2023-01-10T10:00:00Z"

/**
 * @brief Device values
 */
#define sampleazureiotDEFAULT_START_TEMP_COUNT            1
#define sampleazureiotDEFAULT_START_TEMP_CELSIUS          22.0
#define sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS         2

/**
 * @brief Property Values
 */
#define sampleazureiotPROPERTY_STATUS_SUCCESS             200
#define sampleazureiotPROPERTY_SUCCESS                    "success"
#define sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT    "targetTemperature"
#define sampleazureiotPROPERTY_MAX_TEMPERATURE_TEXT       "maxTempSinceLastReboot"

/**
 * @brief Telemetry values
 */
#define sampleazureiotTELEMETRY_NAME                      "temperature"

/**
 *@brief The Telemetry message published in this example.
 */
#define sampleazureiotMESSAGE                             "{\"" sampleazureiotTELEMETRY_NAME "\":%0.2f}"

/**
 * @brief Buffer for ADU to copy values into.
 *
 */
static uint8_t ucADUScratchBuffer[ azureiotjwsSCRATCH_BUFFER_SIZE ];

/* Device values */
static double xDeviceCurrentTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
static double xDeviceMaximumTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
static double xDeviceMinimumTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
static double xDeviceTemperatureSummation = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
static uint32_t ulDeviceTemperatureCount = sampleazureiotDEFAULT_START_TEMP_COUNT;
static double xDeviceAverageTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;

/* Command buffers */
static uint8_t ucCommandStartTimeValueBuffer[ 32 ];
/*-----------------------------------------------------------*/

/* ADU.200702.R */
static uint8_t ucAzureIoTADURootKeyId200702[ 13 ] = "ADU.200702.R";
static uint8_t ucAzureIoTADURootKeyN200702[ 385 ]
    =
    {
    0x00, 0xd5, 0x42, 0x2e, 0xaf, 0x11, 0x54, 0xa3, 0x50, 0x65, 0x87, 0xa2, 0x4d, 0x5b, 0xba,
    0x1a, 0xfb, 0xa9, 0x32, 0xdf, 0xe9, 0x99, 0x5f, 0x05, 0x45, 0xc8, 0xaf, 0xbd, 0x35, 0x1d,
    0x89, 0xe8, 0x27, 0x27, 0x58, 0xa3, 0xa8, 0xee, 0xc5, 0xc5, 0x1e, 0x4f, 0xf7, 0x92, 0xa6,
    0x12, 0x06, 0x7d, 0x3d, 0x7d, 0xb0, 0x07, 0xf6, 0x2c, 0x7f, 0xde, 0x6d, 0x2a, 0xf5, 0xbc,
    0x49, 0xbc, 0x15, 0xef, 0xf0, 0x81, 0xcb, 0x3f, 0x88, 0x4f, 0x27, 0x1d, 0x88, 0x71, 0x28,
    0x60, 0x08, 0xb6, 0x19, 0xd2, 0xd2, 0x39, 0xd0, 0x05, 0x1f, 0x3c, 0x76, 0x86, 0x71, 0xbb,
    0x59, 0x58, 0xbc, 0xb1, 0x88, 0x7b, 0xab, 0x56, 0x28, 0xbf, 0x31, 0x73, 0x44, 0x32, 0x10,
    0xfd, 0x3d, 0xd3, 0x96, 0x5c, 0xff, 0x4e, 0x5c, 0xb3, 0x6b, 0xff, 0x8b, 0x84, 0x9b, 0x8b,
    0x80, 0xb8, 0x49, 0xd0, 0x7d, 0xfa, 0xd6, 0x40, 0x58, 0x76, 0x4d, 0xc0, 0x72, 0x27, 0x75,
    0xcb, 0x9a, 0x2f, 0x9b, 0xb4, 0x9f, 0x0f, 0x25, 0xf1, 0x1c, 0xc5, 0x1b, 0x0b, 0x5a, 0x30,
    0x7d, 0x2f, 0xb8, 0xef, 0xa7, 0x26, 0x58, 0x53, 0xaf, 0xd5, 0x1d, 0x55, 0x01, 0x51, 0x0d,
    0xe9, 0x1b, 0xa2, 0x0f, 0x3f, 0xd7, 0xe9, 0x1d, 0x20, 0x41, 0xa6, 0xe6, 0x14, 0x0a, 0xae,
    0xfe, 0xf2, 0x1c, 0x2a, 0xd6, 0xe4, 0x04, 0x7b, 0xf6, 0x14, 0x7e, 0xec, 0x0f, 0x97, 0x83,
    0xfa, 0x58, 0xfa, 0x81, 0x36, 0x21, 0xb9, 0xa3, 0x2b, 0xfa, 0xd9, 0x61, 0x0b, 0x1a, 0x94,
    0xf7, 0xc1, 0xbe, 0x7f, 0x40, 0x14, 0x4a, 0xc9, 0xfa, 0x35, 0x7f, 0xef, 0x66, 0x70, 0x00,
    0xb1, 0xfd, 0xdb, 0xd7, 0x61, 0x0d, 0x3b, 0x58, 0x74, 0x67, 0x94, 0x89, 0x75, 0x76, 0x96,
    0x7c, 0x91, 0x87, 0xd2, 0x8e, 0x11, 0x97, 0xee, 0x7b, 0x87, 0x6c, 0x9a, 0x2f, 0x45, 0xd8,
    0x65, 0x3f, 0x52, 0x70, 0x98, 0x2a, 0xcb, 0xc8, 0x04, 0x63, 0xf5, 0xc9, 0x47, 0xcf, 0x70,
    0xf4, 0xed, 0x64, 0xa7, 0x74, 0xa5, 0x23, 0x8f, 0xb6, 0xed, 0xf7, 0x1c, 0xd3, 0xb0, 0x1c,
    0x64, 0x57, 0x12, 0x5a, 0xa9, 0x81, 0x84, 0x1f, 0xa0, 0xe7, 0x50, 0x19, 0x96, 0xb4, 0x82,
    0xb1, 0xac, 0x48, 0xe3, 0xe1, 0x32, 0x82, 0xcb, 0x40, 0x1f, 0xac, 0xc4, 0x59, 0xbc, 0x10,
    0x34, 0x51, 0x82, 0xf9, 0x28, 0x8d, 0xa8, 0x1e, 0x9b, 0xf5, 0x79, 0x45, 0x75, 0xb2, 0xdc,
    0x9a, 0x11, 0x43, 0x08, 0xbe, 0x61, 0xcc, 0x9a, 0xc4, 0xcb, 0x77, 0x36, 0xff, 0x83, 0xdd,
    0xa8, 0x71, 0x4f, 0x51, 0x8e, 0x0e, 0x7b, 0x4d, 0xfa, 0x79, 0x98, 0x8d, 0xbe, 0xfc, 0x82,
    0x7e, 0x40, 0x48, 0xa9, 0x12, 0x01, 0xa8, 0xd9, 0x7e, 0xf3, 0xa5, 0x1b, 0xf1, 0xfb, 0x90,
    0x77, 0x3e, 0x40, 0x87, 0x18, 0xc9, 0xab, 0xd9, 0xf7, 0x79
    };
static uint8_t ucAzureIoTADURootKeyE200702[ 3 ] = { 0x01, 0x00, 0x01 };

/* ADU.200703.R */
static uint8_t ucAzureIoTADURootKeyId200703[ 13 ] = "ADU.200703.R";
static uint8_t ucAzureIoTADURootKeyN200703[ 385 ] =
{
    0x00, 0xb2, 0xa3, 0xb2, 0x74, 0x16, 0xfa, 0xbb, 0x20, 0xf9, 0x52, 0x76, 0xe6, 0x27, 0x3e,
    0x80, 0x41, 0xc6, 0xfe, 0xcf, 0x30, 0xf9, 0xc8, 0x96, 0xf5, 0x59, 0x0a, 0xaa, 0x81, 0xe7,
    0x51, 0x83, 0x8a, 0xc4, 0xf5, 0x17, 0x3a, 0x2f, 0x2a, 0xe6, 0x57, 0xd4, 0x71, 0xce, 0x8a,
    0x3d, 0xef, 0x9a, 0x55, 0x76, 0x3e, 0x99, 0xe2, 0xc2, 0xae, 0x4c, 0xee, 0x2d, 0xb8, 0x78,
    0xf5, 0xa2, 0x4e, 0x28, 0xf2, 0x9c, 0x4e, 0x39, 0x65, 0xbc, 0xec, 0xe4, 0x0d, 0xe5, 0xe3,
    0x38, 0xa8, 0x59, 0xab, 0x08, 0xa4, 0x1b, 0xb4, 0xf4, 0xa0, 0x52, 0xa3, 0x38, 0xb3, 0x46,
    0x21, 0x13, 0xcc, 0x3c, 0x68, 0x06, 0xde, 0xfe, 0x00, 0xa6, 0x92, 0x6e, 0xde, 0x4c, 0x47,
    0x10, 0xd6, 0x1c, 0x9c, 0x24, 0xf5, 0xcd, 0x70, 0xe1, 0xf5, 0x6a, 0x7c, 0x68, 0x13, 0x1d,
    0xe1, 0xc5, 0xf6, 0xa8, 0x4f, 0x21, 0x9f, 0x86, 0x7c, 0x44, 0xc5, 0x8a, 0x99, 0x1c, 0xc5,
    0xd3, 0x06, 0x9b, 0x5a, 0x71, 0x9d, 0x09, 0x1c, 0xc3, 0x64, 0x31, 0x6a, 0xc5, 0x17, 0x95,
    0x1d, 0x5d, 0x2a, 0xf1, 0x55, 0xc7, 0x66, 0xd4, 0xe8, 0xf5, 0xd9, 0xa9, 0x5b, 0x8c, 0xa2,
    0x6c, 0x62, 0x60, 0x05, 0x37, 0xd7, 0x32, 0xb0, 0x73, 0xcb, 0xf7, 0x4b, 0x36, 0x27, 0x24,
    0x21, 0x8c, 0x38, 0x0a, 0xb8, 0x18, 0xfe, 0xf5, 0x15, 0x60, 0x35, 0x8b, 0x35, 0xef, 0x1e,
    0x0f, 0x88, 0xa6, 0x13, 0x8d, 0x7b, 0x7d, 0xef, 0xb3, 0xe7, 0xb0, 0xc9, 0xa6, 0x1c, 0x70,
    0x7b, 0xcc, 0xf2, 0x29, 0x8b, 0x87, 0xf7, 0xbd, 0x9d, 0xb6, 0x88, 0x6f, 0xac, 0x73, 0xff,
    0x72, 0xf2, 0xef, 0x48, 0x27, 0x96, 0x72, 0x86, 0x06, 0xa2, 0x5c, 0xe3, 0x7d, 0xce, 0xb0,
    0x9e, 0xe5, 0xc2, 0xd9, 0x4e, 0xc4, 0xf3, 0x7f, 0x78, 0x07, 0x4b, 0x65, 0x88, 0x45, 0x0c,
    0x11, 0xe5, 0x96, 0x56, 0x34, 0x88, 0x2d, 0x16, 0x0e, 0x59, 0x42, 0xd2, 0xf7, 0xd9, 0xed,
    0x1d, 0xed, 0xc9, 0x37, 0x77, 0x44, 0x7e, 0xe3, 0x84, 0x36, 0x9f, 0x58, 0x13, 0xef, 0x6f,
    0xe4, 0xc3, 0x44, 0xd4, 0x77, 0x06, 0x8a, 0xcf, 0x5b, 0xc8, 0x80, 0x1c, 0xa2, 0x98, 0x65,
    0x0b, 0x35, 0xdc, 0x73, 0xc8, 0x69, 0xd0, 0x5e, 0xe8, 0x25, 0x43, 0x9e, 0xf6, 0xd8, 0xab,
    0x05, 0xaf, 0x51, 0x29, 0x23, 0x55, 0x40, 0x58, 0x10, 0xea, 0xb8, 0xe2, 0xcd, 0x5d, 0x79,
    0xcc, 0xec, 0xdf, 0xb4, 0x5b, 0x98, 0xc7, 0xfa, 0xe3, 0xd2, 0x6c, 0x26, 0xce, 0x2e, 0x2c,
    0x56, 0xe0, 0xcf, 0x8d, 0xee, 0xfd, 0x93, 0x12, 0x2f, 0x00, 0x49, 0x8d, 0x1c, 0x82, 0x38,
    0x56, 0xa6, 0x5d, 0x79, 0x44, 0x4a, 0x1a, 0xf3, 0xdc, 0x16, 0x10, 0xb3, 0xc1, 0x2d, 0x27,
    0x11, 0xfe, 0x1b, 0x98, 0x05, 0xe4, 0xa3, 0x60, 0x31, 0x99
};
static uint8_t ucAzureIoTADURootKeyE200703[ 3 ] = { 0x01, 0x00, 0x01 };

static AzureIoTJWS_RootKey_t xADURootKeys[] =
{
    {
        .pucRootKeyId = ucAzureIoTADURootKeyId200703,
        .ulRootKeyIdLength = sizeof( ucAzureIoTADURootKeyId200703 ) - 1,
        .pucRootKeyN = ucAzureIoTADURootKeyN200703,
        .ulRootKeyNLength = sizeof( ucAzureIoTADURootKeyN200703 ),
        .pucRootKeyExponent = ucAzureIoTADURootKeyE200703,
        .ulRootKeyExponentLength = sizeof( ucAzureIoTADURootKeyE200703 )
    },
    {
        .pucRootKeyId = ucAzureIoTADURootKeyId200702,
        .ulRootKeyIdLength = sizeof( ucAzureIoTADURootKeyId200702 ) - 1,
        .pucRootKeyN = ucAzureIoTADURootKeyN200702,
        .ulRootKeyNLength = sizeof( ucAzureIoTADURootKeyN200702 ),
        .pucRootKeyExponent = ucAzureIoTADURootKeyE200702,
        .ulRootKeyExponentLength = sizeof( ucAzureIoTADURootKeyE200702 )
    }
};

/*-----------------------------------------------------------*/

/**
 * @brief Generate max min payload.
 */
static AzureIoTResult_t prvInvokeMaxMinCommand( AzureIoTJSONReader_t * pxReader,
                                                AzureIoTJSONWriter_t * pxWriter )
{
    AzureIoTResult_t xResult;
    uint32_t ulSinceTimeLength;

    /* Get the start time */
    if( ( xResult = AzureIoTJSONReader_NextToken( pxReader ) )
        != eAzureIoTSuccess )
    {
        LogError( ( "Error getting next token: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONReader_GetTokenString( pxReader,
                                                            ucCommandStartTimeValueBuffer,
                                                            sizeof( ucCommandStartTimeValueBuffer ),
                                                            &ulSinceTimeLength ) )
             != eAzureIoTSuccess )
    {
        LogError( ( "Error getting token string: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendBeginObject( pxWriter ) )
             != eAzureIoTSuccess )
    {
        LogError( ( "Error appending begin object: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( pxWriter, ( const uint8_t * ) sampleazureiotCOMMAND_MAX_TEMP,
                                                                           sizeof( sampleazureiotCOMMAND_MAX_TEMP ) - 1,
                                                                           xDeviceMaximumTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS ) )
             != eAzureIoTSuccess )
    {
        LogError( ( "Error appending max temp: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( pxWriter, ( const uint8_t * ) sampleazureiotCOMMAND_MIN_TEMP,
                                                                           sizeof( sampleazureiotCOMMAND_MIN_TEMP ) - 1,
                                                                           xDeviceMinimumTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS ) )
             != eAzureIoTSuccess )
    {
        LogError( ( "Error appending min temp: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( pxWriter, ( const uint8_t * ) sampleazureiotCOMMAND_TEMP_VERSION,
                                                                           sizeof( sampleazureiotCOMMAND_TEMP_VERSION ) - 1,
                                                                           xDeviceAverageTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS ) )
             != eAzureIoTSuccess )
    {
        LogError( ( "Error appending average temp: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( pxWriter, ( const uint8_t * ) sampleazureiotCOMMAND_START_TIME,
                                                                           sizeof( sampleazureiotCOMMAND_START_TIME ) - 1,
                                                                           ucCommandStartTimeValueBuffer, ulSinceTimeLength ) )
             != eAzureIoTSuccess )
    {
        LogError( ( "Error appending start time: result 0x%08x", xResult ) );
    }
    /* Faking the end time to simplify dependencies on <time.h> */
    else if( ( xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( pxWriter, ( const uint8_t * ) sampleazureiotCOMMAND_END_TIME,
                                                                           sizeof( sampleazureiotCOMMAND_END_TIME ) - 1,
                                                                           ( const uint8_t * ) sampleazureiotCOMMAND_FAKE_END_TIME,
                                                                           sizeof( sampleazureiotCOMMAND_FAKE_END_TIME ) - 1 ) )
             != eAzureIoTSuccess )
    {
        LogError( ( "Error appending end time: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendEndObject( pxWriter ) )
             != eAzureIoTSuccess )
    {
        LogError( ( "Error appending end object: result 0x%08x", xResult ) );
    }

    return xResult;
}
/*-----------------------------------------------------------*/

static void prvSkipPropertyAndValue( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTResult_t xResult;

    xResult = AzureIoTJSONReader_NextToken( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONReader_SkipChildren( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONReader_NextToken( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

/**
 * @brief Update local device temperature values based on new requested temperature.
 */
static void prvUpdateLocalProperties( double xNewTemperatureValue,
                                      uint32_t ulPropertyVersion,
                                      bool * pxOutMaxTempChanged )
{
    *pxOutMaxTempChanged = false;
    xDeviceCurrentTemperature = xNewTemperatureValue;

    /* Update maximum or minimum temperatures. */
    if( xDeviceCurrentTemperature > xDeviceMaximumTemperature )
    {
        xDeviceMaximumTemperature = xDeviceCurrentTemperature;
        *pxOutMaxTempChanged = true;
    }
    else if( xDeviceCurrentTemperature < xDeviceMinimumTemperature )
    {
        xDeviceMinimumTemperature = xDeviceCurrentTemperature;
    }

    /* Calculate the new average temperature. */
    ulDeviceTemperatureCount++;
    xDeviceTemperatureSummation += xDeviceCurrentTemperature;
    xDeviceAverageTemperature = xDeviceTemperatureSummation / ulDeviceTemperatureCount;

    LogInfo( ( "Client updated desired temperature variables locally." ) );
    LogInfo( ( "Current Temperature: %2f", xDeviceCurrentTemperature ) );
    LogInfo( ( "Maximum Temperature: %2f", xDeviceMaximumTemperature ) );
    LogInfo( ( "Minimum Temperature: %2f", xDeviceMinimumTemperature ) );
    LogInfo( ( "Average Temperature: %2f", xDeviceAverageTemperature ) );
}
/*-----------------------------------------------------------*/

/**
 * @brief Gets the reported properties payload with the maximum temperature value.
 */
static uint32_t prvGetNewMaxTemp( double xUpdatedTemperature,
                                  uint8_t * ucReportedPropertyPayloadBuffer,
                                  uint32_t ulReportedPropertyPayloadBufferSize )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    /* Initialize the JSON writer with the buffer to which we will write the payload with the new temperature. */
    xResult = AzureIoTJSONWriter_Init( &xWriter, ucReportedPropertyPayloadBuffer, ulReportedPropertyPayloadBufferSize );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( const uint8_t * ) sampleazureiotPROPERTY_MAX_TEMPERATURE_TEXT,
                                                     sizeof( sampleazureiotPROPERTY_MAX_TEMPERATURE_TEXT ) - 1 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendDouble( &xWriter, xUpdatedTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );
    configASSERT( lBytesWritten > 0 );

    return lBytesWritten;
}
/*-----------------------------------------------------------*/

/**
 * @brief Verifies if the current image version matches the "installedCriteria" version in the
 *        installation step of the ADU Update Manifest.
 *
 * @param pxAduUpdateRequest Parsed update request, with the ADU update manifest.
 * @return true If the current image version matches the installedCriteria.
 * @return false If the current image version does not match the installedCriteria.
 */
static bool prvDoesInstalledCriteriaMatchCurrentVersion( const AzureIoTADUUpdateRequest_t * pxAduUpdateRequest )
{
    /*
     * In a production solution, each step should be validated against the version of
     * each component the update step applies to (matching through the `handler` name).
     */
    if( ( ( sizeof( democonfigADU_UPDATE_VERSION ) - 1 ) ==
          pxAduUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ 0 ].ulInstalledCriteriaLength ) &&
        ( strncmp(
              ( const char * ) democonfigADU_UPDATE_VERSION,
              ( const char * ) pxAduUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ 0 ].pucInstalledCriteria,
              ( size_t ) pxAduUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ 0 ].ulInstalledCriteriaLength ) == 0 ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Verifies that the handler is supported
 *
 * @param pxAduUpdateRequest Parsed update request, with the ADU update manifest.
 * @return true If the handler for the update step matches the supported handler.
 * @return false If the handler for the update step does not match the supported handler.
 */
static bool prvIsADUHandlerSupported( const AzureIoTADUUpdateRequest_t * pxAduUpdateRequest )
{
    if( ( ( sizeof( sampleazureiotUPDATE_HANDLER ) - 1 ) ==
          pxAduUpdateRequest->xUpdateManifest.xInstructions.pxSteps->ulHandlerLength ) &&
        ( strncmp(
              ( const char * ) sampleazureiotUPDATE_HANDLER,
              ( const char * ) pxAduUpdateRequest->xUpdateManifest.xInstructions.pxSteps->pucHandler,
              ( size_t ) pxAduUpdateRequest->xUpdateManifest.xInstructions.pxSteps->ulHandlerLength ) == 0 ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Sample function to decide if an update request should be accepted or rejected.
 *
 * @remark The user application can implement any logic to decide if an update request
 *         should be accepted or not. Factors would be if the device is currently busy,
 *         if it is within business hours, or any other factor the user would like to
 *         take into account. Rejected update requests get redelivered upon reconnection
 *         with the Azure IoT Hub.
 *
 * @param[in] pxAduUpdateRequest    The parsed update request.
 * @return An #AzureIoTADURequestDecision_t with the decision to accept or reject the update.
 */
static AzureIoTADURequestDecision_t prvUserDecideShouldStartUpdate( AzureIoTADUUpdateRequest_t * pxAduUpdateRequest )
{
    if( !prvIsADUHandlerSupported( pxAduUpdateRequest ) )
    {
        LogInfo( ( "[ADU] Rejecting update request (update handler not supported)" ) );
        return eAzureIoTADURequestDecisionReject;
    }

    if( prvDoesInstalledCriteriaMatchCurrentVersion( pxAduUpdateRequest ) )
    {
        LogInfo( ( "[ADU] Rejecting update request (installed criteria matches current version)" ) );
        return eAzureIoTADURequestDecisionReject;
    }
    else if( ( AzureIoTPlatform_GetSingleFlashBootBankSize() < pxAduUpdateRequest->xUpdateManifest.pxFiles[ 0 ].llSizeInBytes ) || ( pxAduUpdateRequest->xUpdateManifest.pxFiles[ 0 ].llSizeInBytes < 0 ) )
    {
        LogInfo( ( "[ADU] Rejecting update request (image size larger than flash bank size)" ) );
        return eAzureIoTADURequestDecisionReject;
    }
    else
    {
        LogInfo( ( "[ADU] Accepting update request" ) );
        return eAzureIoTADURequestDecisionAccept;
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Property message callback handler
 */
void vHandleWritableProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                uint8_t * pucWritablePropertyResponseBuffer,
                                uint32_t ulWritablePropertyResponseBufferSize,
                                uint32_t * pulWritablePropertyResponseBufferLength )
{
    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONReader_t xJsonReader;
    const uint8_t * pucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    uint32_t ulPropertyVersion;

    LogInfo( ( "Writable properties received: %.*s\r\n",
               ( int16_t ) pxMessage->ulPayloadLength, ( char * ) pxMessage->pvMessagePayload ) );

    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );

    if( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", ( uint16_t ) xAzIoTResult ) );
        *pulWritablePropertyResponseBufferLength = 0;
        return;
    }

    xAzIoTResult = AzureIoTHubClientProperties_GetPropertiesVersion( &xAzureIoTHubClient, &xJsonReader, pxMessage->xMessageType, &ulPropertyVersion );

    if( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTHubClientProperties_GetPropertiesVersion failed: result 0x%08x", ( uint16_t ) xAzIoTResult ) );
        *pulWritablePropertyResponseBufferLength = 0;
        return;
    }

    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );

    if( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", ( uint16_t ) xAzIoTResult ) );
        *pulWritablePropertyResponseBufferLength = 0;
        return;
    }

    /**
     * If the PnP component is for Azure Device Update, function
     * AzureIoTADUClient_SendResponse shall be used to publish back the
     * response for the ADU writable properties.
     * Thus, to prevent this callback to publish a response in duplicate,
     * pulWritablePropertyResponseBufferLength must be set to zero.
     */
    *pulWritablePropertyResponseBufferLength = 0;

    while( ( xAzIoTResult = AzureIoTHubClientProperties_GetNextComponentProperty( &xAzureIoTHubClient, &xJsonReader,
                                                                                  pxMessage->xMessageType, eAzureIoTHubClientPropertyWritable,
                                                                                  &pucComponentName, &ulComponentNameLength ) ) == eAzureIoTSuccess )
    {
        LogInfo( ( "Properties component name: %.*s", ( int16_t ) ulComponentNameLength, pucComponentName ) );

        if( AzureIoTADUClient_IsADUComponent( &xAzureIoTADUClient, pucComponentName, ulComponentNameLength ) )
        {
            AzureIoTADURequestDecision_t xRequestDecision;

            xAzIoTResult = AzureIoTADUClient_ParseRequest(
                &xAzureIoTADUClient,
                &xJsonReader,
                &xAzureIoTAduUpdateRequest );

            if( xAzIoTResult != eAzureIoTSuccess )
            {
                LogError( ( "AzureIoTADUClient_ParseRequest failed: result 0x%08x", ( uint16_t ) xAzIoTResult ) );
                *pulWritablePropertyResponseBufferLength = 0;
                return;
            }

            if( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionApplyDownload )
            {
                LogInfo( ( "Verifying JWS Manifest" ) );
                xAzIoTResult = AzureIoTJWS_ManifestAuthenticate( xAzureIoTAduUpdateRequest.pucUpdateManifest,
                                                                 xAzureIoTAduUpdateRequest.ulUpdateManifestLength,
                                                                 xAzureIoTAduUpdateRequest.pucUpdateManifestSignature,
                                                                 xAzureIoTAduUpdateRequest.ulUpdateManifestSignatureLength,
                                                                 &xADURootKeys[ 0 ],
                                                                 sizeof( xADURootKeys ) / sizeof( xADURootKeys[ 0 ] ),
                                                                 ucADUScratchBuffer,
                                                                 sizeof( ucADUScratchBuffer ) );

                if( xAzIoTResult != eAzureIoTSuccess )
                {
                    LogError( ( "AzureIoTJWS_ManifestAuthenticate failed: JWS was not validated successfully: result 0x%08x", ( uint16_t ) xAzIoTResult ) );
                    return;
                }

                xRequestDecision = prvUserDecideShouldStartUpdate( &xAzureIoTAduUpdateRequest );

                xAzIoTResult = AzureIoTADUClient_SendResponse(
                    &xAzureIoTADUClient,
                    &xAzureIoTHubClient,
                    xRequestDecision,
                    ulPropertyVersion,
                    pucWritablePropertyResponseBuffer,
                    ulWritablePropertyResponseBufferSize,
                    NULL );

                if( xAzIoTResult != eAzureIoTSuccess )
                {
                    LogError( ( "AzureIoTADUClient_GetResponse failed: result 0x%08x", ( uint16_t ) xAzIoTResult ) );
                    return;
                }

                if( xRequestDecision == eAzureIoTADURequestDecisionAccept )
                {
                    xProcessUpdateRequest = true;
                }
            }
            else if( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionCancel )
            {
                /*Nothing to do here but set process to "true", where we will then send state as "Idle" */
                xProcessUpdateRequest = true;

                LogInfo( ( "ADU manifest received: action cancelled" ) );
            }
            else
            {
                xProcessUpdateRequest = false;

                LogInfo( ( "ADU manifest received: action unknown" ) );
            }
        }
        else
        {
            LogInfo( ( "Component not ADU: %.*s", ( int16_t ) ulComponentNameLength, pucComponentName ) );
            prvSkipPropertyAndValue( &xJsonReader );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Implements the sample interface for generating reported properties payload.
 */
uint32_t ulCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                           uint32_t ulPropertiesDataSize )
{
    /* No reported properties to send if length is zero. */
    uint32_t lBytesWritten = 0;

    return lBytesWritten;
}
/*-----------------------------------------------------------*/

/**
 * @brief Command message callback handler
 */
uint32_t ulHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                          uint32_t * pulResponseStatus,
                          uint8_t * pucCommandResponsePayloadBuffer,
                          uint32_t ulCommandResponsePayloadBufferSize )
{
    uint32_t ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;

    *pulResponseStatus = AZ_IOT_STATUS_NOT_FOUND;
    configASSERT( ulCommandResponsePayloadBufferSize >= ulCommandResponsePayloadLength );
    ( void ) memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );

    return ulCommandResponsePayloadLength;
}
/*-----------------------------------------------------------*/

/**
 * @brief Implements the sample interface for generating Telemetry payload.
 */
uint32_t ulCreateTelemetry( uint8_t * pucTelemetryData,
                            uint32_t ulTelemetryDataSize,
                            uint32_t * ulTelemetryDataLength )
{
    int result = snprintf( ( char * ) pucTelemetryData, ulTelemetryDataSize,
                           sampleazureiotMESSAGE, xDeviceCurrentTemperature );

    if( ( result >= 0 ) && ( result < ulTelemetryDataSize ) )
    {
        *ulTelemetryDataLength = result;
        result = 0;
    }
    else
    {
        result = 1;
    }

    return result;
}

/*-----------------------------------------------------------*/
