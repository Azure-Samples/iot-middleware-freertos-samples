/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_freertos_esp32_pnp_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Azure Provisioning/IoT Hub library includes */
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

// TODO: rename and re-organize

/* Define the ADU agent component name.  */
#define NX_AZURE_IOT_ADU_AGENT_COMPONENT_NAME                           "deviceUpdate"

/* Define the ADU agent interface ID.  */
#define NX_AZURE_IOT_ADU_AGENT_INTERFACE_ID                             "dtmi:azure:iot:deviceUpdate;1"

/* Define the compatibility.  */
#define NX_AZURE_IOT_ADU_AGENT_COMPATIBILITY                            "manufacturer,model"

/* Define the ADU agent property name "agent" and sub property names.  */
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_AGENT                      "agent"

#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_DEVICEPROPERTIES           "deviceProperties"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_MANUFACTURER               "manufacturer"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_MODEL                      "model"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_INTERFACE_ID               "interfaceId"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_ADU_VERSION                "aduVer"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_DO_VERSION                 "doVer"

#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_COMPAT_PROPERTY_NAMES      "compatPropertyNames"

#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_INSTALLED_CONTENT_ID       "installedUpdateId"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_PROVIDER                   "provider"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_NAME                       "name"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_VERSION                    "version"

#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_LAST_INSTALL_RESULT        "lastInstallResult"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_RESULT_CODE                "resultCode"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_EXTENDED_RESULT_CODE       "extendedResultCode"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_RESULT_DETAILS             "resultDetails"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_STEP_RESULTS               "stepResults"

#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_STATE                      "state"

/* Define the ADU agent property name "service" and sub property names.  */
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_SERVICE                    "service"

#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_WORKFLOW                   "workflow"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_ACTION                     "action"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_ID                         "id"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_RETRY_TIMESTAMP            "retryTimestamp"

#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_UPDATE_MANIFEST            "updateManifest"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_UPDATE_MANIFEST_SIGNATURE  "updateManifestSignature"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_FILEURLS                   "fileUrls"

#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_MANIFEST_VERSION           "manifestVersion"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_UPDATE_ID                  "updateId"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_COMPATIBILITY              "compatibility"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_DEVICE_MANUFACTURER        "deviceManufacturer"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_DEVICE_MODEL               "deviceModel"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_GROUP                      "group"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_INSTRUCTIONS               "instructions"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_STEPS                      "steps"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_TYPE                       "type"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_HANDLE                     "handler"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_FILES                      "files"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_DETACHED_MANIFEST_FILED    "detachedManifestFileId"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_INSTALLED_CRITERIA         "installedCriteria"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_FILE_NAME                  "fileName"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_SIZE_IN_BYTES              "sizeInBytes"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_HASHES                     "hashes"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_SHA256                     "sha256"
#define NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_CREATED_DATE_TIME          "createdDateTime"

/*-----------------------------------------------------------*/

static int32_t lGenerateAduAgentPayload( uint8_t * pucPropertiesData,
                                         uint32_t ulPropertiesDataSize )
{
    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    int32_t azure_iot_adu_agent_state = 0;
    const char * azure_iot_adu_device_manufacturer = "ESPRESSIF";
    const char * azure_iot_adu_device_model = "ESP32-Azure-IoT-Kit";
    const char * installed_update_id = "{\"provider\":\"ESPRESSIF\",\"Name\":\"ESP32-Azure-IoT-Kit\",\"Version\":\"1.0\"}";

    /* Update reported property */
    xAzIoTResult = AzureIoTJSONWriter_Init( &xWriter, pucPropertiesData, ulPropertiesDataSize );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

        /* Fill the ADU agent component name.  */
        xAzIoTResult = AzureIoTHubClientProperties_BuilderBeginComponent(
            &xAzureIoTHubClient, &xWriter, ( const uint8_t * ) NX_AZURE_IOT_ADU_AGENT_COMPONENT_NAME, strlen( NX_AZURE_IOT_ADU_AGENT_COMPONENT_NAME ) );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

            /* Fill the agent property name.  */
            xAzIoTResult = AzureIoTJSONWriter_AppendPropertyName(&xWriter, ( uint8_t * ) NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_AGENT, lengthof( NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_AGENT ));
            configASSERT( xAzIoTResult == eAzureIoTSuccess );

            xAzIoTResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );

                /* Fill the state.   */
                xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_STATE, lengthof( NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_STATE ),
                                                                                azure_iot_adu_agent_state );
                configASSERT( xAzIoTResult == eAzureIoTSuccess );

                /* Fill the workflow.  */
                /* Append retry timestamp in workflow if existed.  */
                /* Fill installed update id.  */
                xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_INSTALLED_CONTENT_ID, lengthof( NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_INSTALLED_CONTENT_ID ),
                                                                                ( uint8_t * ) installed_update_id, strlen( installed_update_id ) );
                configASSERT( xAzIoTResult == eAzureIoTSuccess );                

                /* Fill the deviceProperties.  */
                xAzIoTResult = AzureIoTJSONWriter_AppendPropertyName(&xWriter, ( uint8_t * ) NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_DEVICEPROPERTIES, lengthof( NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_DEVICEPROPERTIES ));
                configASSERT( xAzIoTResult == eAzureIoTSuccess );

                xAzIoTResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
                configASSERT( xAzIoTResult == eAzureIoTSuccess );

                    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_MANUFACTURER, lengthof( NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_MANUFACTURER ),
                                                                                    ( uint8_t * ) azure_iot_adu_device_manufacturer, strlen( azure_iot_adu_device_manufacturer ) );
                    configASSERT( xAzIoTResult == eAzureIoTSuccess );

                    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_MODEL, lengthof( NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_MODEL ),
                                                                                    ( uint8_t * ) azure_iot_adu_device_model, strlen( azure_iot_adu_device_model ) );
                    configASSERT( xAzIoTResult == eAzureIoTSuccess );

                    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_INTERFACE_ID, lengthof( NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_INTERFACE_ID ),
                                                                                    ( uint8_t * ) NX_AZURE_IOT_ADU_AGENT_INTERFACE_ID, lengthof( NX_AZURE_IOT_ADU_AGENT_INTERFACE_ID ) );
                    configASSERT( xAzIoTResult == eAzureIoTSuccess );

                xAzIoTResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
                configASSERT( xAzIoTResult == eAzureIoTSuccess );

                /* Fill the comatability property.  */
                xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_COMPAT_PROPERTY_NAMES, lengthof( NX_AZURE_IOT_ADU_AGENT_PROPERTY_NAME_COMPAT_PROPERTY_NAMES ),
                                                                                ( uint8_t * ) NX_AZURE_IOT_ADU_AGENT_COMPATIBILITY, lengthof( NX_AZURE_IOT_ADU_AGENT_COMPATIBILITY ) );
                configASSERT( xAzIoTResult == eAzureIoTSuccess );

                /* Fill the last install result.  */


            xAzIoTResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTHubClientProperties_BuilderEndComponent( &xAzureIoTHubClient, &xWriter );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the device properties JSON" ) );
        lBytesWritten = 0;
    }

    return lBytesWritten;
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
                                uint32_t * pulWritablePropertyResponseBufferLength )
{

    * pulWritablePropertyResponseBufferLength = 0;
    printf( "Writable properties received: %.*s\r\n",
        pxMessage->ulPayloadLength, ( char * ) pxMessage->pvMessagePayload );
}
/*-----------------------------------------------------------*/

static bool xSendProperties = true;

uint32_t ulSampleCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                                 uint32_t ulPropertiesDataSize )
{
    /* No reported properties to send if length is zero. */
    uint32_t lBytesWritten = 0;

    if ( xSendProperties )
    {
        lBytesWritten = lGenerateAduAgentPayload( pucPropertiesData, ulPropertiesDataSize );

        printf( "otaPayload=%.*s\r\n", lBytesWritten, pucPropertiesData );

        xSendProperties = false;
    }

    return lBytesWritten;
}
/*-----------------------------------------------------------*/
