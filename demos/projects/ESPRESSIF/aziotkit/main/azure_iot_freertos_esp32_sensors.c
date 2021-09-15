// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"

#include "sample_azure_iot_pnp.h"
#include "led.h"
#include "sensor_manager.h"
/*-----------------------------------------------------------*/

#define INDEFINITE_TIME                            ( ( time_t ) -1 )
/*-----------------------------------------------------------*/

static const char *TAG = "sample_azureiotkit";
/*-----------------------------------------------------------*/

/**
 * @brief Device Info Values
 */
#define sampleazureiotgsgDEVICE_INFORMATION_NAME                 ( "deviceInformation" )
#define sampleazureiotgsgMANUFACTURER_PROPERTY_NAME              ( "manufacturer" )
#define sampleazureiotgsgMODEL_PROPERTY_NAME                     ( "model" )
#define sampleazureiotgsgSOFTWARE_VERSION_PROPERTY_NAME          ( "swVersion" )
#define sampleazureiotgsgOS_NAME_PROPERTY_NAME                   ( "osName" )
#define sampleazureiotgsgPROCESSOR_ARCHITECTURE_PROPERTY_NAME    ( "processorArchitecture" )
#define sampleazureiotgsgPROCESSOR_MANUFACTURER_PROPERTY_NAME    ( "processorManufacturer" )
#define sampleazureiotgsgTOTAL_STORAGE_PROPERTY_NAME             ( "totalStorage" )
#define sampleazureiotgsgTOTAL_MEMORY_PROPERTY_NAME              ( "totalMemory" )

const char * pcManufacturerPropertyValue = "ESPRESSIF";
const char * pcModelPropertyValue = "ESP32 Azure IoT Kit";
const char * pcSoftwareVersionPropertyValue = "1.0.0";
const char * pcOsNamePropertyValue = "FreeRTOS";
const char * pcProcessorArchitecturePropertyValue = "ESP32 WROVER-B";
const char * pcProcessorManufacturerPropertyValue = "ESPRESSIF";
const double xTotalStoragePropertyValue = 4153344;
const double xTotalMemoryPropertyValue = 8306688;

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

static time_t xLastTelemetrySendTime = INDEFINITE_TIME;

/**
 * @brief Command Values
 */
#define sampleazureiotCOMMAND_EMPTY_PAYLOAD        "{}"

static const char sampleazureiotCOMMAND_TOGGLE_LED1[] = "ToggleLed1";
static const char sampleazureiotCOMMAND_TOGGLE_LED2[] = "ToggleLed2";
static const char sampleazureiotCOMMAND_DISPLAY_TEXT[] = "DisplayText";

static bool xLed1State = false;
static bool xLed2State = false;

/**
 * @brief Property Values
 */
#define sampleazureiotPROPERTY_STATUS_SUCCESS      200
#define sampleazureiotPROPERTY_SUCCESS             "success"
#define sampleazureiotPROPERTY_TELEMETRY_FREQUENCY ( "telemetryFrequencySecs" )

static int lTelemetryFrequencySecs = 2;

static uint8_t ucPropertyPayloadBuffer[ 384 ];
/*-----------------------------------------------------------*/

int32_t lGenerateDeviceInfo( uint8_t * pucPropertiesData,
                             uint32_t ulPropertiesDataSize )
{
    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    /* Update reported property */
    xAzIoTResult = AzureIoTJSONWriter_Init( &xWriter, ucPropertyPayloadBuffer, sizeof( ucPropertyPayloadBuffer ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_BuilderBeginComponent( &xAzureIoTHubClient, &xWriter, ( const uint8_t * ) sampleazureiotgsgDEVICE_INFORMATION_NAME, strlen( sampleazureiotgsgDEVICE_INFORMATION_NAME ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotgsgMANUFACTURER_PROPERTY_NAME, sizeof( sampleazureiotgsgMANUFACTURER_PROPERTY_NAME ) - 1,
                                                                     ( uint8_t * ) pcManufacturerPropertyValue, strlen( pcManufacturerPropertyValue ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotgsgMODEL_PROPERTY_NAME, sizeof( sampleazureiotgsgMODEL_PROPERTY_NAME ) - 1,
                                                                     ( uint8_t * ) pcModelPropertyValue, strlen( pcModelPropertyValue ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotgsgSOFTWARE_VERSION_PROPERTY_NAME, sizeof( sampleazureiotgsgSOFTWARE_VERSION_PROPERTY_NAME ) - 1,
                                                                     ( uint8_t * ) pcSoftwareVersionPropertyValue, strlen( pcSoftwareVersionPropertyValue ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotgsgOS_NAME_PROPERTY_NAME, sizeof( sampleazureiotgsgOS_NAME_PROPERTY_NAME ) - 1,
                                                                     ( uint8_t * ) pcOsNamePropertyValue, strlen( pcOsNamePropertyValue ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotgsgPROCESSOR_ARCHITECTURE_PROPERTY_NAME, sizeof( sampleazureiotgsgPROCESSOR_ARCHITECTURE_PROPERTY_NAME ) - 1,
                                                                     ( uint8_t * ) pcProcessorArchitecturePropertyValue, strlen( pcProcessorArchitecturePropertyValue ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotgsgPROCESSOR_MANUFACTURER_PROPERTY_NAME, sizeof( sampleazureiotgsgPROCESSOR_MANUFACTURER_PROPERTY_NAME ) - 1,
                                                                     ( uint8_t * ) pcProcessorManufacturerPropertyValue, strlen( pcProcessorManufacturerPropertyValue ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotgsgTOTAL_STORAGE_PROPERTY_NAME, sizeof( sampleazureiotgsgTOTAL_STORAGE_PROPERTY_NAME ) - 1,
                                                                     xTotalStoragePropertyValue, 0 );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotgsgTOTAL_MEMORY_PROPERTY_NAME, sizeof( sampleazureiotgsgTOTAL_MEMORY_PROPERTY_NAME ) - 1,
                                                                     xTotalMemoryPropertyValue, 0 );
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
    uint32_t ulCommandResponsePayloadLength;

    ESP_LOGI( TAG, "Command payload : %.*s \r\n",
              pxMessage->ulPayloadLength,
              ( const char * ) pxMessage->pvMessagePayload );

    if ( strncmp( ( const char * ) pxMessage->pucCommandName, sampleazureiotCOMMAND_TOGGLE_LED1, pxMessage->usCommandNameLength ) == 0)
    {
        xLed1State = !xLed1State;
        toggle_wifi_led( xLed1State ? LED_ON : LED_OFF );

        *pulResponseStatus = AZ_IOT_STATUS_OK;
        ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }
    else if ( strncmp( ( const char * ) pxMessage->pucCommandName, sampleazureiotCOMMAND_TOGGLE_LED2, pxMessage->usCommandNameLength ) == 0)
    {
        xLed2State = !xLed2State;
        toggle_azure_led( xLed2State ? LED_ON : LED_OFF );

        *pulResponseStatus = AZ_IOT_STATUS_OK;
        ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }
    else if ( strncmp( ( const char * ) pxMessage->pucCommandName, sampleazureiotCOMMAND_DISPLAY_TEXT, pxMessage->usCommandNameLength ) == 0)
    {
        oled_clean_screen();
        oled_show_message( ( const uint8_t * ) pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );

        *pulResponseStatus = AZ_IOT_STATUS_OK;
        ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }
    else
    {
        *pulResponseStatus = AZ_IOT_STATUS_NOT_FOUND;
        ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }

    return ulCommandResponsePayloadLength;
}
/*-----------------------------------------------------------*/

uint32_t ulSampleCreateTelemetry( uint8_t * pucTelemetryData,
                                  uint32_t ulTelemetryDataLength )
{
    int32_t lBytesWritten = 0;
    time_t xNow = time( NULL );

    if ( xNow == INDEFINITE_TIME )
    {
        ESP_LOGE( TAG, "Failed obtaining current time.\r\n" );
    }

    if ( xLastTelemetrySendTime == INDEFINITE_TIME || xNow == INDEFINITE_TIME ||
         difftime( xNow , xLastTelemetrySendTime ) > lTelemetryFrequencySecs )
    {
        AzureIoTResult_t xAzIoTResult;
        AzureIoTJSONWriter_t xWriter;

        float xPressure;
        float xAltitude;
        int lMagnetometerX;
        int lMagnetometerY;
        int lMagnetometerZ;
        int lPitch;
        int lRoll;
        int lAccelerometerX;
        int lAccelerometerY;
        int lAccelerometerZ;

        // Collect sensor data
        float xTemperature = get_temperature();
        float xHumidity = get_humidity();
        float xLight = get_ambientLight();
        get_pressure_altitude( &xPressure, &xAltitude );
        get_magnetometer( &lMagnetometerX, &lMagnetometerY, &lMagnetometerZ );
        get_pitch_roll_accel( &lPitch, &lRoll, &lAccelerometerX, &lAccelerometerY, &lAccelerometerZ );

        // Initialize Json Writer
        xAzIoTResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );
        
        // Temperature, Humidity, Light Intensity
        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_TEMPERATURE, sizeof( sampleazureiotTELEMETRY_TEMPERATURE ) - 1, xTemperature, 2 );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_HUMIDITY, sizeof( sampleazureiotTELEMETRY_HUMIDITY ) - 1, xHumidity, 2 );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_LIGHT, sizeof( sampleazureiotTELEMETRY_LIGHT ) - 1, xLight, 2 );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        // Pressure, Altitude
        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_PRESSURE, sizeof( sampleazureiotTELEMETRY_PRESSURE ) - 1, xPressure, 2 );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_ALTITUDE, sizeof( sampleazureiotTELEMETRY_ALTITUDE ) - 1, xAltitude, 2 );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        // Magnetometer
        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_MAGNETOMETERX, sizeof( sampleazureiotTELEMETRY_MAGNETOMETERX ) - 1, lMagnetometerX );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_MAGNETOMETERY, sizeof( sampleazureiotTELEMETRY_MAGNETOMETERY ) - 1, lMagnetometerY );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_MAGNETOMETERZ, sizeof( sampleazureiotTELEMETRY_MAGNETOMETERZ ) - 1, lMagnetometerZ );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        // Pitch, Roll, Accelleration
        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_PITCH, sizeof( sampleazureiotTELEMETRY_PITCH ) - 1, lPitch );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_ROLL, sizeof( sampleazureiotTELEMETRY_ROLL ) - 1, lRoll );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_ACCELEROMETERX, sizeof( sampleazureiotTELEMETRY_ACCELEROMETERX ) - 1, lAccelerometerX );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_ACCELEROMETERY, sizeof( sampleazureiotTELEMETRY_ACCELEROMETERY ) - 1, lAccelerometerY );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_ACCELEROMETERZ, sizeof( sampleazureiotTELEMETRY_ACCELEROMETERZ ) - 1, lAccelerometerZ );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        // Complete Json Content
        xAzIoTResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

        xLastTelemetrySendTime = xNow;
    }

    return lBytesWritten;
}
/*-----------------------------------------------------------*/


/**
 * @brief Acknowledges the update of Telemetry Frequency property.
 */
static void prvAckTelemetryFrequencyPropertyUpdate( uint8_t * pucPropertiesData, uint32_t ulPropertiesDataSize, uint32_t ulVersion )
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
                                                                           sizeof( sampleazureiotPROPERTY_TELEMETRY_FREQUENCY ) - 1,
                                                                           sampleazureiotPROPERTY_STATUS_SUCCESS,
                                                                           ulVersion,
                                                                           ( const uint8_t * ) sampleazureiotPROPERTY_SUCCESS,
                                                                           sizeof( sampleazureiotPROPERTY_SUCCESS ) - 1 );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendInt32( &xWriter, lTelemetryFrequencySecs );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_BuilderEndResponseStatus( &xAzureIoTHubClient, &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );
    configASSERT( lBytesWritten > 0 );
}
/*-----------------------------------------------------------*/

/**
 * @brief Handler for writable properties updates.
 */
void vSampleHandleWritablePropertiesUpdate( AzureIoTHubClientPropertiesResponse_t * pxMessage )
{
    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONReader_t xJsonReader;
    const uint8_t * pucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    uint32_t ulPropertyVersion;

    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_GetPropertiesVersion( &xAzureIoTHubClient, &xJsonReader, pxMessage->xMessageType, &ulPropertyVersion );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    /* Reset JSON reader to the beginning */
    xAzIoTResult = AzureIoTJSONReader_Init( &xJsonReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    while( ( xAzIoTResult = AzureIoTHubClientProperties_GetNextComponentProperty( &xAzureIoTHubClient, &xJsonReader,
                                                                                  pxMessage->xMessageType, eAzureIoTHubClientPropertyWritable,
                                                                                  &pucComponentName, &ulComponentNameLength ) ) == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJsonReader,
                                                 ( const uint8_t * ) sampleazureiotPROPERTY_TELEMETRY_FREQUENCY,
                                                 sizeof( sampleazureiotPROPERTY_TELEMETRY_FREQUENCY ) - 1 ) )
        {
            xAzIoTResult = AzureIoTJSONReader_NextToken( &xJsonReader );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );

            xAzIoTResult = AzureIoTJSONReader_GetTokenInt32( &xJsonReader, &lTelemetryFrequencySecs );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );

            prvAckTelemetryFrequencyPropertyUpdate( ucPropertyPayloadBuffer, sizeof( ucPropertyPayloadBuffer ), ulPropertyVersion);

            ESP_LOGI( TAG, "Telemetry frequency set to once every %d seconds.\r\n", lTelemetryFrequencySecs );

            xAzIoTResult = AzureIoTJSONReader_NextToken( &xJsonReader );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );
        }
    }
}
/*-----------------------------------------------------------*/
