/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "sample_azure_iot_pnp_data_if.h"

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Azure JSON includes */
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

/* FreeRTOS */
/* This task provides taskDISABLE_INTERRUPTS, used by configASSERT */
#include "FreeRTOS.h"
#include "task.h"

/*
 * TODO: In future improvement, compare sampleazureiotMODEL_ID macro definition
 *       and make sure that it is "dtmi:com:example:Thermostat;1",
 *       and fail compilation otherwise.
 */

/*-----------------------------------------------------------*/

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

static AzureIoTResult_t prvSkipPropertyAndValue( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTResult_t xResult;

    if( ( xResult = AzureIoTJSONReader_NextToken( pxReader ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error getting next token: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONReader_SkipChildren( pxReader ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error skipping children: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONReader_NextToken( pxReader ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error getting next token: result 0x%08x", xResult ) );
    }

    return xResult;
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
 * @returns bytes written or -1 if there's an error
 */
static uint32_t prvGetNewMaxTemp( double xUpdatedTemperature,
                                  uint8_t * ucReportedPropertyPayloadBuffer,
                                  uint32_t ulReportedPropertyPayloadBufferSize )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten = -1;

    /* Initialize the JSON writer with the buffer to which we will write the payload with the new temperature. */
    if( ( xResult = AzureIoTJSONWriter_Init( &xWriter, ucReportedPropertyPayloadBuffer, ulReportedPropertyPayloadBufferSize ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error initializing JSON writer: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error appending begin object: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( const uint8_t * ) sampleazureiotPROPERTY_MAX_TEMPERATURE_TEXT,
                                                                sizeof( sampleazureiotPROPERTY_MAX_TEMPERATURE_TEXT ) - 1 ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error appending max temperature property name: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendDouble( &xWriter, xUpdatedTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error appending temperature: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error appending end object: result 0x%08x", xResult ) );
    }
    else if( ( lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter ) ) < 1 )
    {
        LogError( ( "Error getting bytes used. Bytes written: 0x%d", lBytesWritten ) );
    }

    return lBytesWritten;
}
/*-----------------------------------------------------------*/

/**
 * @brief Generate an update for the device's target temperature property online,
 *        acknowledging the update from the IoT Hub.
 */
static uint32_t prvGenerateAckForIncomingTemperature( double xUpdatedTemperature,
                                                      uint32_t ulVersion,
                                                      uint8_t * pucResponseBuffer,
                                                      uint32_t ulResponseBufferSize )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten = -1;

    /* Building the acknowledgement payload for the temperature property to signal we successfully received and accept it. */
    if( ( xResult = AzureIoTJSONWriter_Init( &xWriter, pucResponseBuffer, ulResponseBufferSize ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error initializing JSON writer: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error appending begin object: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xAzureIoTHubClient,
                                                                                 &xWriter,
                                                                                 ( const uint8_t * ) sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT,
                                                                                 sizeof( sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT ) - 1,
                                                                                 sampleazureiotPROPERTY_STATUS_SUCCESS,
                                                                                 ulVersion,
                                                                                 ( const uint8_t * ) sampleazureiotPROPERTY_SUCCESS,
                                                                                 sizeof( sampleazureiotPROPERTY_SUCCESS ) - 1 ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error beginning response: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendDouble( &xWriter, xUpdatedTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error appending temperature: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTHubClientProperties_BuilderEndResponseStatus( &xAzureIoTHubClient,
                                                                               &xWriter ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error ending response: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error appending end object: result 0x%08x", xResult ) );
    }
    else if( ( lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter ) ) < 1 )
    {
        LogError( ( "Error getting bytes used. Bytes written: 0x%d", lBytesWritten ) );
    }

    return ( uint32_t ) lBytesWritten;
}
/*-----------------------------------------------------------*/

/**
 * @brief Properties callback handler
 */
static AzureIoTResult_t prvProcessProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                              AzureIoTHubClientPropertyType_t xPropertyType,
                                              double * pxOutTemperature,
                                              uint32_t * ulOutVersion )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONReader_t xReader;
    const uint8_t * pucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;

    *pxOutTemperature = 0.0;

    if( ( xResult = AzureIoTJSONReader_Init( &xReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error initializing the JSON reader: result 0x%08x", xResult ) );
    }
    else if( ( xResult = AzureIoTHubClientProperties_GetPropertiesVersion( &xAzureIoTHubClient, &xReader, pxMessage->xMessageType, ulOutVersion ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error getting the property version: result 0x%08x", xResult ) );
    }
    /* Reset JSON reader to the beginning */
    else if( ( xResult = AzureIoTJSONReader_Init( &xReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error initializing the JSON reader: result 0x%08x", xResult ) );
    }
    else
    {
        while( ( xResult = AzureIoTHubClientProperties_GetNextComponentProperty( &xAzureIoTHubClient, &xReader,
                                                                                 pxMessage->xMessageType, xPropertyType,
                                                                                 &pucComponentName, &ulComponentNameLength ) ) == eAzureIoTSuccess )
        {
            if( ulComponentNameLength > 0 )
            {
                LogInfo( ( "Unknown component name received" ) );

                /* Unknown component name arrived (there are none for this device).
                 * We have to skip over the property and value to continue iterating */
                xResult = prvSkipPropertyAndValue( &xReader );

                if( xResult != eAzureIoTSuccess )
                {
                    break;
                }
            }
            else if( AzureIoTJSONReader_TokenIsTextEqual( &xReader,
                                                          ( const uint8_t * ) sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT,
                                                          sizeof( sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT ) - 1 ) )
            {
                if( ( xResult = AzureIoTJSONReader_NextToken( &xReader ) ) != eAzureIoTSuccess )
                {
                    LogError( ( "Error getting next token: result 0x%08x", xResult ) );
                }
                /* Get desired temperature */
                else if( ( xResult = AzureIoTJSONReader_GetTokenDouble( &xReader, pxOutTemperature ) ) != eAzureIoTSuccess )
                {
                    LogError( ( "Error getting the property version: result 0x%08x", xResult ) );
                }
                else if( ( xResult = AzureIoTJSONReader_NextToken( &xReader ) ) != eAzureIoTSuccess )
                {
                    LogError( ( "Error getting next token: result 0x%08x", xResult ) );
                }

                if( xResult != eAzureIoTSuccess )
                {
                    break;
                }
            }
            else
            {
                LogInfo( ( "Unknown property arrived: skipping over it." ) );

                /* Unknown property arrived. We have to skip over the property and value to continue iterating. */
                xResult = prvSkipPropertyAndValue( &xReader );

                if( xResult != eAzureIoTSuccess )
                {
                    break;
                }
            }
        }

        if( xResult != eAzureIoTErrorEndOfProperties )
        {
            LogError( ( "There was an error parsing the properties: result 0x%08x", xResult ) );
        }
        else
        {
            LogInfo( ( "Successfully parsed properties" ) );
            xResult = eAzureIoTSuccess;
        }
    }

    return xResult;
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
    AzureIoTResult_t xResult;
    double xIncomingTemperature;
    uint32_t ulVersion;
    bool xWasMaxTemperatureChanged = false;

    xResult = prvProcessProperties( pxMessage, eAzureIoTHubClientPropertyWritable, &xIncomingTemperature, &ulVersion );

    if( xResult == eAzureIoTSuccess )
    {
        prvUpdateLocalProperties( xIncomingTemperature, ulVersion, &xWasMaxTemperatureChanged );
        *pulWritablePropertyResponseBufferLength = prvGenerateAckForIncomingTemperature(
            xIncomingTemperature,
            ulVersion,
            pucWritablePropertyResponseBuffer,
            ulWritablePropertyResponseBufferSize );
    }
    else
    {
        LogError( ( "There was an error processing incoming properties: result 0x%08x", xResult ) );
    }
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
    AzureIoTResult_t xResult;
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONWriter_t xWriter;
    int32_t lCommandNameLength;
    int32_t ulCommandResponsePayloadLength;

    LogInfo( ( "Command payload : %.*s \r\n",
               ( int16_t ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );

    lCommandNameLength = sizeof( sampleazureiotCOMMAND_MAX_MIN_REPORT ) - 1;

    if( ( lCommandNameLength == pxMessage->usCommandNameLength ) &&
        ( strncmp( sampleazureiotCOMMAND_MAX_MIN_REPORT, ( const char * ) pxMessage->pucCommandName, lCommandNameLength ) == 0 ) )
    {
        /* Is for max min report */

        /*Initialize the reader from which we pull the "since". */
        if( ( xResult = AzureIoTJSONReader_Init( &xReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Error initializing the JSON reader: result 0x%08x", xResult ) );
        }
        /* Initialize the JSON writer with a buffer to which we will write the response payload. */
        else if( ( xResult = AzureIoTJSONWriter_Init( &xWriter, pucCommandResponsePayloadBuffer, ulCommandResponsePayloadBufferSize ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Error initializing the JSON writer: result 0x%08x", xResult ) );
        }
        /* Read from the writer the "since" value and use it to construct the response payload in the writer. */
        else if( ( xResult = prvInvokeMaxMinCommand( &xReader, &xWriter ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Error generating command payload: result 0x%08x", xResult ) );
        }
        else
        {
            ulCommandResponsePayloadLength = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

            *pulResponseStatus = AZ_IOT_STATUS_OK;
        }

        if( xResult != eAzureIoTSuccess )
        {
            *pulResponseStatus = 501;
            ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;
            configASSERT( ulCommandResponsePayloadBufferSize >= ulCommandResponsePayloadLength );
            ( void ) memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
        }
    }
    else
    {
        /* Not for max min report (not for this device) */
        LogInfo( ( "Received command is not for this device: %.*s",
                   pxMessage->usCommandNameLength,
                   pxMessage->pucCommandName ) );

        *pulResponseStatus = AZ_IOT_STATUS_NOT_FOUND;
        ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;
        configASSERT( ulCommandResponsePayloadBufferSize >= ulCommandResponsePayloadLength );
        ( void ) memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }

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

/**
 * @brief Implements the sample interface for generating reported properties payload.
 */
uint32_t ulCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                           uint32_t ulPropertiesDataSize )
{
    return prvGetNewMaxTemp( xDeviceCurrentTemperature, pucPropertiesData, ulPropertiesDataSize );
}

/*-----------------------------------------------------------*/
