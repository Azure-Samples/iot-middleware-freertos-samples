/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_freertos_esp32_pnp_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Azure Provisioning/IoT Hub library includes */
#include <azure/iot/az_iot_adu_ota.h>
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"

#include "sample_azure_iot_pnp_data_if.h"
/*-----------------------------------------------------------*/

#define INDEFINITE_TIME                            ( ( time_t ) - 1 )
#define lengthof( x )                              ( sizeof(x) - 1 )
// This macro helps remove quotes around a string.
// That is achieved by skipping the first char in the string, and reducing the length by 2 chars.
#define UNQUOTE_STRING( x )                        ( x + 1)
#define UNQUOTED_STRING_LENGTH( n )                ( n - 2 )
/*-----------------------------------------------------------*/

static const char *TAG = "sample_azureiotkit";
/*-----------------------------------------------------------*/

/**
 * @brief Device Info Values
 */
#define sampleazureiotkitDEVICE_INFORMATION_NAME                 ( "deviceInformation" )
#define sampleazureiotkitMANUFACTURER_PROPERTY_NAME              ( "manufacturer" )
#define sampleazureiotkitMODEL_PROPERTY_NAME                     ( "model" )
#define sampleazureiotkitSOFTWARE_VERSION_PROPERTY_NAME          ( "swVersion" )
#define sampleazureiotkitOS_NAME_PROPERTY_NAME                   ( "osName" )
#define sampleazureiotkitPROCESSOR_ARCHITECTURE_PROPERTY_NAME    ( "processorArchitecture" )
#define sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_NAME    ( "processorManufacturer" )
#define sampleazureiotkitTOTAL_STORAGE_PROPERTY_NAME             ( "totalStorage" )
#define sampleazureiotkitTOTAL_MEMORY_PROPERTY_NAME              ( "totalMemory" )

#define sampleazureiotkitMANUFACTURER_PROPERTY_VALUE             ( "ESPRESSIF" )
#define sampleazureiotkitMODEL_PROPERTY_VALUE                    ( "ESP32-Azure-IoT-Kit" )
#define sampleazureiotkitVERSION_PROPERTY_VALUE                  ( "1.0.0" )
#define sampleazureiotkitOS_NAME_PROPERTY_VALUE                  ( "FreeRTOS" )
#define sampleazureiotkitARCHITECTURE_PROPERTY_VALUE             ( "ESP32 WROVER-B" )
#define sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_VALUE   ( "ESPRESSIF" )
// The next couple properties are in KiloBytes.
#define sampleazureiotkitTOTAL_STORAGE_PROPERTY_VALUE            4096
#define sampleazureiotkitTOTAL_MEMORY_PROPERTY_VALUE             8192

/**
 * @brief Telemetry Values
 */
#define sampleazureiotTELEMETRY_TEMPERATURE        ( "temperature" )
#define sampleazureiotTELEMETRY_HUMIDITY           ( "humidity" )
#define sampleazureiotTELEMETRY_LIGHT              ( "light" )
#define sampleazureiotTELEMETRY_PRESSURE           ( "pressure" )
#define sampleazureiotTELEMETRY_ALTITUDE           ( "altitude" )
#define sampleazureiotTELEMETRY_MAGNETOMETERX      ( "magnetometerX" )
#define sampleazureiotTELEMETRY_MAGNETOMETERY      ( "magnetometerY" )
#define sampleazureiotTELEMETRY_MAGNETOMETERZ      ( "magnetometerZ" )
#define sampleazureiotTELEMETRY_PITCH              ( "pitch" )
#define sampleazureiotTELEMETRY_ROLL               ( "roll" )
#define sampleazureiotTELEMETRY_ACCELEROMETERX     ( "accelerometerX" )
#define sampleazureiotTELEMETRY_ACCELEROMETERY     ( "accelerometerY" )
#define sampleazureiotTELEMETRY_ACCELEROMETERZ     ( "accelerometerZ" )

/**
 * @brief Command Values
 */
#define sampleazureiotCOMMAND_EMPTY_PAYLOAD        "{}"

/**
 * @brief Property Values
 */
#define sampleazureiotPROPERTY_STATUS_SUCCESS      200
#define sampleazureiotPROPERTY_SUCCESS             "success"
#define sampleazureiotPROPERTY_TELEMETRY_FREQUENCY ( "telemetryFrequencySecs" )

static int lTelemetryFrequencySecs = 2;

/*-----------------------------------------------------------*/
// ADU OTA
#define OTA_CONTEXT_BUFFER_SIZE 5000
static uint8_t uOtaContextBuffer[OTA_CONTEXT_BUFFER_SIZE];
static int32_t uOtaAgentState = AZ_IOT_ADU_OTA_AGENT_STATE_IDLE;
static bool xOtaAgentSendProperties = true;
static bool xOtaServiceActionReceived = false;



/*-----------------------------------------------------------*/

static int32_t lGenerateAduAgentPayload( uint8_t * pucPropertiesData,
                                         uint32_t ulPropertiesDataSize )
{
    az_span xPayload = az_span_create(pucPropertiesData, ulPropertiesDataSize);

    // TODO: remove this hardcoded part, read from manifest. 
    az_iot_adu_ota_step_result step_results[1];
    step_results[0].step_id = AZ_SPAN_FROM_STR("step1");
    step_results[0].result_code = 700;
    step_results[0].extended_result_code = 0;
    step_results[0].result_details = AZ_SPAN_FROM_STR("");

    az_iot_adu_ota_install_result last_install_result;
    last_install_result.result_code = 700;
    last_install_result.extended_result_code = 0;
    last_install_result.result_details = AZ_SPAN_FROM_STR("");
    last_install_result.step_results_count = 1;
    last_install_result.step_results = step_results;

    if (az_result_failed(
        az_iot_adu_ota_get_properties_payload(
            &xAzureIoTHubClient._internal.xAzureIoTHubClientCore,
            &xOtaDeviceInformation,
            uOtaAgentState,
            pxOtaLastWorkflow,
            pxOtaLastWorkflow != NULL && xOtaLastInstalledVersion == 1.1 ? NULL : &last_install_result,
            xPayload,
            &xPayload)))
    {
        return 0;
    }

    return az_span_size(xPayload);
}
/*-----------------------------------------------------------*/

/**
 * @brief Command message callback handler
 */
uint32_t ulSampleHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                                uint32_t * pulResponseStatus,
                                uint8_t * pucCommandResponsePayloadBuffer,
                                uint32_t ulCommandResponsePayloadBufferSize)
{
    ESP_LOGI( TAG, "Command payload : %.*s \r\n",
              pxMessage->ulPayloadLength,
              ( const char * ) pxMessage->pvMessagePayload );

    uint32_t ulCommandResponsePayloadLength = lengthof( sampleazureiotCOMMAND_EMPTY_PAYLOAD );
    *pulResponseStatus = AZ_IOT_STATUS_NOT_FOUND;
    configASSERT( ulCommandResponsePayloadBufferSize >= ulCommandResponsePayloadLength );
    (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );

    return ulCommandResponsePayloadLength;
}
/*-----------------------------------------------------------*/

uint32_t ulSampleCreateTelemetry( uint8_t * pucTelemetryData,
                                  uint32_t ulTelemetryDataLength )
{
    (void)pucTelemetryData;
    (void)ulTelemetryDataLength;
    int32_t lBytesWritten = 0;
    return lBytesWritten;
}
/*-----------------------------------------------------------*/


/**
 * @brief Acknowledges the update of Telemetry Frequency property.
 */
static uint32_t prvGenerateAckForTelemetryFrequencyPropertyUpdate( uint8_t * pucPropertiesData,
                                                                   uint32_t ulPropertiesDataSize,
                                                                   uint32_t ulVersion )
{
    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xAzIoTResult = AzureIoTJSONWriter_Init( &xWriter, pucPropertiesData, ulPropertiesDataSize );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xAzureIoTHubClient,
                                                                           &xWriter,
                                                                           ( const uint8_t * ) sampleazureiotPROPERTY_TELEMETRY_FREQUENCY,
                                                                           lengthof( sampleazureiotPROPERTY_TELEMETRY_FREQUENCY ),
                                                                           sampleazureiotPROPERTY_STATUS_SUCCESS,
                                                                           ulVersion,
                                                                           ( const uint8_t * ) sampleazureiotPROPERTY_SUCCESS,
                                                                           lengthof( sampleazureiotPROPERTY_SUCCESS ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendInt32( &xWriter, lTelemetryFrequencySecs );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_BuilderEndResponseStatus( &xAzureIoTHubClient, &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );
    configASSERT( lBytesWritten > 0 );

    return ( uint32_t ) lBytesWritten;
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
 * @brief Handler for writable properties updates.
 */
void vHandleWritableProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                uint8_t * pucWritablePropertyResponseBuffer, 
                                uint32_t ulWritablePropertyResponseBufferSize,
                                uint32_t *pulWritablePropertyResponseBufferLength )
{
    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONReader_t xJsonReader;
    const uint8_t * pucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    uint32_t ulPropertyVersion;
    az_result azres;

    // TODO: remove printfs
    printf( "Writable properties received: %.*s\r\n",
        pxMessage->ulPayloadLength, ( char * ) pxMessage->pvMessagePayload );

    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    if ( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", xAzIoTResult ) );
        * pulWritablePropertyResponseBufferLength = 0;
        return;
    }

    xAzIoTResult = AzureIoTHubClientProperties_GetPropertiesVersion( &xAzureIoTHubClient, &xJsonReader, pxMessage->xMessageType, &ulPropertyVersion );
    if ( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTHubClientProperties_GetPropertiesVersion failed: result 0x%08x", xAzIoTResult ) );
        * pulWritablePropertyResponseBufferLength = 0;
        return;
    }

    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    if ( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", xAzIoTResult ) );
        * pulWritablePropertyResponseBufferLength = 0;
        return;
    }

    * pulWritablePropertyResponseBufferLength = 0;

    while( ( xAzIoTResult = AzureIoTHubClientProperties_GetNextComponentProperty( &xAzureIoTHubClient, &xJsonReader,
                                                                                  pxMessage->xMessageType, eAzureIoTHubClientPropertyWritable,
                                                                                  &pucComponentName, &ulComponentNameLength ) ) == eAzureIoTSuccess )
    {
        LogInfo( ( "Properties component name: %.*s", ulComponentNameLength, pucComponentName ) );

        // TODO: fix sign of pucComponentName in AzureIoTADUClient_IsADUComponent (should be uint8_t*)
        if ( AzureIoTADUClient_IsADUComponent( &xAzureIoTADUClient, ( const char * ) pucComponentName, ulComponentNameLength ) )
        {
            xAzIoTResult = AzureIoTADUClient_ADUProcessComponent(
                                &xAzureIoTADUClient,
                                &xJsonReader,
                                ulPropertyVersion,
                                pucWritablePropertyResponseBuffer,
                                ulWritablePropertyResponseBufferSize,
                                pulWritablePropertyResponseBufferLength );

            if ( xAzIoTResult != eAzureIoTSuccess )
            {
                LogError( ( "Failed updating ADU context." ) );
            }
        }
        else
        {
            LogInfo( ( "Component not ADU OTA: %.*s", ulComponentNameLength, pucComponentName ) );
            prvSkipPropertyAndValue( &xJsonReader );
        }
    }
}
/*-----------------------------------------------------------*/

#define TIME_UNDEFINED (time_t)-1
static time_t xOtaLastStepTime = TIME_UNDEFINED;
static double xOtaStepDelaySeconds = 3;

#define prvIsOtaStepCompleted( ) \
    ( xOtaLastStepTime == TIME_UNDEFINED || difftime( time(NULL), xOtaLastStepTime ) >= xOtaStepDelaySeconds )
#define prvSetOtaStepStartTime( ) \
    xOtaLastStepTime = time(NULL)

// TODO: [NOTE] the ADU logic was implemented here because this is a frequently called function from
//              the FreeRTOS middleware samples main task.  
//              Ref: https://github.com/Azure/iot-hub-device-update/blob/main/docs/agent-reference/goal-state-support.md
uint32_t ulSampleCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                                 uint32_t ulPropertiesDataSize )
{
    /* No reported properties to send if length is zero. */
    uint32_t lBytesWritten = 0;

    if ( xOtaAgentSendProperties )
    {
        lBytesWritten = lGenerateAduAgentPayload( pucPropertiesData, ulPropertiesDataSize );

        LogInfo( ( "[OTA] Agent properties payload=%.*s\r\n", lBytesWritten, pucPropertiesData ) );

        xOtaAgentSendProperties = false;

        if ( uOtaAgentState == AZ_IOT_ADU_OTA_AGENT_STATE_FAILED )
        {
            LogInfo( ( "[OTA] deployment failed. Setting agent to IDLE." ) );
            uOtaAgentState = AZ_IOT_ADU_OTA_AGENT_STATE_IDLE;
        }
    }
    else if ( xOtaServiceActionReceived )
    {
        if ( xOtaUpdateRequest.workflow.action == AZ_IOT_ADU_OTA_SERVICE_ACTION_APPLY_DEPLOYMENT )
        {
            if ( uOtaAgentState == AZ_IOT_ADU_OTA_AGENT_STATE_IDLE )
            {
                // TODO: check incoming version and compare with currently installed.
                LogInfo( ( "[OTA] starting deployment" ) );

                uOtaAgentState = AZ_IOT_ADU_OTA_AGENT_STATE_DEPLOYMENT_IN_PROGRESS;
                xOtaAgentSendProperties = true;
                prvSetOtaStepStartTime( );
            }
            else
            {
                LogError( ( "[OTA] deployment started, but agent is in failed state" ) );
                // TODO: Fail? (send AZ_IOT_ADU_OTA_AGENT_STATE_FAILED?)
            }
        }
        else if ( xOtaUpdateRequest.workflow.action == AZ_IOT_ADU_OTA_SERVICE_ACTION_CANCEL )
        {
            if ( uOtaAgentState == AZ_IOT_ADU_OTA_AGENT_STATE_DEPLOYMENT_IN_PROGRESS )
            {
                LogInfo( ( "[OTA] cancelling deployment" ) );
                uOtaAgentState = AZ_IOT_ADU_OTA_AGENT_STATE_IDLE;
                xOtaAgentSendProperties = true;
            }
            else
            {
                LogError( ( "[OTA] deployment cancelled, but agent is not in DIP state" ) );
                // TODO: Fail? (send AZ_IOT_ADU_OTA_AGENT_STATE_FAILED?)
                //       If already in FAILED state, should reset to IDLE?
            }
        }

        xOtaServiceActionReceived = false;
    }
    else if ( uOtaAgentState == AZ_IOT_ADU_OTA_AGENT_STATE_DEPLOYMENT_IN_PROGRESS )
    {
        /*
         * Download started
         * Download Succeeded
         * Install started
         * Install succeeded
         * Apply started
         * Apply succeeded
         * 
         * prvIsOtaStepCompleted simulates a delay to complete all the steps above.
         */
        if ( prvIsOtaStepCompleted( ) )
        {
            // Simulating increment of version...
            xOtaLastInstalledVersion += 0.1; // TODO: read from manifest.
            xOtaDeviceInformation.adu_version = xGetOtaLastInstalledVersion( );
            uOtaAgentState = AZ_IOT_ADU_OTA_AGENT_STATE_IDLE;
            xOtaAgentSendProperties = true;
            LogInfo( ( "[OTA] deployment completed" ) );
        }
    }

    return lBytesWritten;
}
/*-----------------------------------------------------------*/
