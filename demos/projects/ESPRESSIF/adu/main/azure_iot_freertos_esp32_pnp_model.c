/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_freertos_esp32_pnp_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Azure Provisioning/IoT Hub library includes */
#include <azure/iot/az_iot_adu.h>
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"

#include "jws.h"

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
    ( void ) memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );

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

static bool prvIsUpdateAlreadyInstalled(
    const AzureIoTADUUpdateRequest_t * pxAduUpdateRequest )
{
    if ( pxAduUpdateRequest->xUpdateManifest.xUpdateId.ulNameLength == 
         xADUDeviceInformation.xCurrentUpdateId.ulNameLength &&
         strncmp( ( const char * ) pxAduUpdateRequest->xUpdateManifest.xUpdateId.pucName,
                  ( const char * ) xADUDeviceInformation.xCurrentUpdateId.ucName,
                  ( size_t ) xADUDeviceInformation.xCurrentUpdateId.ulNameLength ) == 0 &&
         pxAduUpdateRequest->xUpdateManifest.xUpdateId.ulProviderLength == 
         xADUDeviceInformation.xCurrentUpdateId.ulProviderLength &&
         strncmp( ( const char * ) pxAduUpdateRequest->xUpdateManifest.xUpdateId.pucProvider,
                  ( const char * ) xADUDeviceInformation.xCurrentUpdateId.ucProvider,
                  ( size_t ) xADUDeviceInformation.xCurrentUpdateId.ulProviderLength ) == 0 &&
         pxAduUpdateRequest->xUpdateManifest.xUpdateId.ulVersionLength == 
         xADUDeviceInformation.xCurrentUpdateId.ulVersionLength &&
         strncmp( ( const char * ) pxAduUpdateRequest->xUpdateManifest.xUpdateId.pucVersion,
                  ( const char * ) xADUDeviceInformation.xCurrentUpdateId.ucVersion,
                  ( size_t ) xADUDeviceInformation.xCurrentUpdateId.ulVersionLength ) == 0 )
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
static AzureIoTADURequestDecision_t prvUserDecideShouldStartUpdate(
    AzureIoTADUUpdateRequest_t * pxAduUpdateRequest )
{
    if ( prvIsUpdateAlreadyInstalled( pxAduUpdateRequest ) )
    {
        LogInfo( ( "[ADU] Rejecting update request (current version is up-to-date)" ) );
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

    LogInfo( ( "Writable properties received: %.*s\r\n",
        pxMessage->ulPayloadLength, ( char * ) pxMessage->pvMessagePayload ) );

    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    if ( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTJSONReader_Init failed: result 0x%08x", xAzIoTResult ) );
        *pulWritablePropertyResponseBufferLength = 0;
        return;
    }

    xAzIoTResult = AzureIoTHubClientProperties_GetPropertiesVersion( &xAzureIoTHubClient, &xJsonReader, pxMessage->xMessageType, &ulPropertyVersion );
    if ( xAzIoTResult != eAzureIoTSuccess )
    {
        LogError( ( "AzureIoTHubClientProperties_GetPropertiesVersion failed: result 0x%08x", xAzIoTResult ) );
        *pulWritablePropertyResponseBufferLength = 0;
        return;
    }

    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    if ( xAzIoTResult != eAzureIoTSuccess )
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

        // TODO: fix sign of pucComponentName in AzureIoTADUClient_IsADUComponent (should be uint8_t*)
        if ( AzureIoTADUClient_IsADUComponent( ( const char * ) pucComponentName, ulComponentNameLength ) )
        {
            AzureIoTADURequestDecision_t xRequestDecision;

            xAzIoTResult = AzureIoTADUClient_ParseRequest(
                                &xJsonReader,
                                &xAzureIoTAduUpdateRequest,
                                ucAduContextBuffer,
                                ADU_CONTEXT_BUFFER_SIZE );

            if ( xAzIoTResult != eAzureIoTSuccess )
            {
                LogError( ( "AzureIoTADUClient_ParseRequest failed: result 0x%08x", xAzIoTResult ) );
                *pulWritablePropertyResponseBufferLength = 0;
                return;
            }

            LogInfo( ( "Verifying JWS Manifest" ) );
            uint32_t ulJWSVerify = JWS_ManifestAuthenticate( (char*)xAzureIoTAduUpdateRequest.pucUpdateManifest,
                                                       xAzureIoTAduUpdateRequest.ulUpdateManifestLength,
                                                       (char*)xAzureIoTAduUpdateRequest.pucUpdateManifestSignature,
                                                       xAzureIoTAduUpdateRequest.ulUpdateManifestSignatureLength);
            if (ulJWSVerify != 0)
            {
              LogError( ( "JWS_ManifestAuthenticate failed: JWS was not validated successfully" ) );
              return;
            }

            xRequestDecision = prvUserDecideShouldStartUpdate( &xAzureIoTAduUpdateRequest );

            xAzIoTResult = AzureIoTADUClient_SendResponse(
                                &xAzureIoTHubClient,
                                xRequestDecision,
                                ulPropertyVersion,
                                pucWritablePropertyResponseBuffer,
                                ulWritablePropertyResponseBufferSize,
                                NULL );

            if ( xAzIoTResult != eAzureIoTSuccess )
            {
                LogError( ( "AzureIoTADUClient_GetResponse failed: result 0x%08x", xAzIoTResult ) );
                return;
            }

            if ( xRequestDecision == eAzureIoTADURequestDecisionAccept )
            {
                xProcessUpdateRequest = true;
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

uint32_t ulSampleCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                                 uint32_t ulPropertiesDataSize )
{
    /* No reported properties to send if length is zero. */
    uint32_t lBytesWritten = 0;
    return lBytesWritten;
}
/*-----------------------------------------------------------*/
