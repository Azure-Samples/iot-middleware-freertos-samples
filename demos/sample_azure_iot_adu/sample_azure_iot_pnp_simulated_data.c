/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "sample_azure_iot_pnp_data_if.h"

/* Standard includes. */
#include <string.h>
#include <stdio.h>

#include "azure_iot_adu_client.h"

/* Azure JSON includes */
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

#include "sample_adu_jws.h"

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

/**
 * @brief Buffer for ADU to copy values into.
 *
 */
static uint8_t ucADUScratchBuffer[ jwsSCRATCH_BUFFER_SIZE ];

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
    if( ( xADUDeviceProperties.xCurrentUpdateId.ulVersionLength ==
          pxAduUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ 0 ].ulInstalledCriteriaLength ) &&
        ( strncmp(
              ( const char * ) xADUDeviceProperties.xCurrentUpdateId.ucVersion,
              ( const char * ) pxAduUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ 0 ].pucInstalledCriteria,
              ( size_t ) xADUDeviceProperties.xCurrentUpdateId.ulVersionLength ) == 0 ) )
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
    if( prvDoesInstalledCriteriaMatchCurrentVersion( pxAduUpdateRequest ) )
    {
        LogInfo( ( "[ADU] Rejecting update request (installed criteria matches current version)" ) );
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
               pxMessage->ulPayloadLength, ( char * ) pxMessage->pvMessagePayload ) );

    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );

    if( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", xAzIoTResult ) );
        *pulWritablePropertyResponseBufferLength = 0;
        return;
    }

    xAzIoTResult = AzureIoTHubClientProperties_GetPropertiesVersion( &xAzureIoTHubClient, &xJsonReader, pxMessage->xMessageType, &ulPropertyVersion );

    if( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTHubClientProperties_GetPropertiesVersion failed: result 0x%08x", xAzIoTResult ) );
        *pulWritablePropertyResponseBufferLength = 0;
        return;
    }

    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );

    if( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", xAzIoTResult ) );
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
        LogInfo( ( "Properties component name: %.*s", ulComponentNameLength, pucComponentName ) );

        if( AzureIoTADUClient_IsADUComponent( &xAzureIoTADUClient, pucComponentName, ulComponentNameLength ) )
        {
            AzureIoTADURequestDecision_t xRequestDecision;

            xAzIoTResult = AzureIoTADUClient_ParseRequest(
                &xAzureIoTADUClient,
                &xJsonReader,
                &xAzureIoTAduUpdateRequest,
                ucAduContextBuffer,
                ADU_CONTEXT_BUFFER_SIZE );

            if( xAzIoTResult != eAzureIoTSuccess )
            {
                LogError( ( "AzureIoTADUClient_ParseRequest failed: result 0x%08x", xAzIoTResult ) );
                *pulWritablePropertyResponseBufferLength = 0;
                return;
            }

            if( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionApplyDownload )
            {
                LogInfo( ( "Verifying JWS Manifest" ) );
                xAzIoTResult = JWS_ManifestAuthenticate( xAzureIoTAduUpdateRequest.pucUpdateManifest,
                                                         xAzureIoTAduUpdateRequest.ulUpdateManifestLength,
                                                         xAzureIoTAduUpdateRequest.pucUpdateManifestSignature,
                                                         xAzureIoTAduUpdateRequest.ulUpdateManifestSignatureLength,
                                                         ucADUScratchBuffer,
                                                         sizeof( ucADUScratchBuffer ) );

                if( xAzIoTResult != eAzureIoTSuccess )
                {
                    LogError( ( "JWS_ManifestAuthenticate failed: JWS was not validated successfully: result 0x%08x", xAzIoTResult ) );
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
                    LogError( ( "AzureIoTADUClient_GetResponse failed: result 0x%08x", xAzIoTResult ) );
                    return;
                }

                if( xRequestDecision == eAzureIoTADURequestDecisionAccept )
                {
                    xProcessUpdateRequest = true;
                }
            }
            else if( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionCancel )
            {
                LogInfo( ( "ADU manifest received: action cancelled" ) );
            }
            else
            {
                LogInfo( ( "ADU manifest received: action unknown" ) );
            }
        }
        else
        {
            LogInfo( ( "Component not ADU: %.*s", ulComponentNameLength, pucComponentName ) );
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
