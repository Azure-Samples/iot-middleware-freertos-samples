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

uint32_t ulSampleCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                                 uint32_t ulPropertiesDataSize )
{
    /* No reported properties to send if length is zero. */
    uint32_t lBytesWritten = 0;
    return lBytesWritten;
}
/*-----------------------------------------------------------*/
