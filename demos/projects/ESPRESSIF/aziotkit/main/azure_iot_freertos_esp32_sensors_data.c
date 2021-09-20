/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_freertos_esp32_sensors_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"

#include "sample_azure_iot_pnp_data_if.h"
#include "sensor_manager.h"
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
#define sampleazureiotkitMODEL_PROPERTY_VALUE                    ( "ESP32 Azure IoT Kit" )
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
/*-----------------------------------------------------------*/

int32_t lGenerateDeviceInfo( uint8_t * pucPropertiesData,
                             uint32_t ulPropertiesDataSize )
{
    AzureIoTResult_t xAzIoTResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    /* Update reported property */
    xAzIoTResult = AzureIoTJSONWriter_Init( &xWriter, pucPropertiesData, ulPropertiesDataSize );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTHubClientProperties_BuilderBeginComponent( &xAzureIoTHubClient, &xWriter, ( const uint8_t * ) sampleazureiotkitDEVICE_INFORMATION_NAME, strlen( sampleazureiotkitDEVICE_INFORMATION_NAME ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitMANUFACTURER_PROPERTY_NAME, lengthof( sampleazureiotkitMANUFACTURER_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitMANUFACTURER_PROPERTY_VALUE, lengthof( sampleazureiotkitMANUFACTURER_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitMODEL_PROPERTY_NAME, lengthof( sampleazureiotkitMODEL_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitMODEL_PROPERTY_VALUE, lengthof( sampleazureiotkitMODEL_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitSOFTWARE_VERSION_PROPERTY_NAME, lengthof( sampleazureiotkitSOFTWARE_VERSION_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitVERSION_PROPERTY_VALUE, lengthof( sampleazureiotkitVERSION_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitOS_NAME_PROPERTY_NAME, lengthof( sampleazureiotkitOS_NAME_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitOS_NAME_PROPERTY_VALUE, lengthof( sampleazureiotkitOS_NAME_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitPROCESSOR_ARCHITECTURE_PROPERTY_NAME, lengthof( sampleazureiotkitPROCESSOR_ARCHITECTURE_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitARCHITECTURE_PROPERTY_VALUE, lengthof( sampleazureiotkitARCHITECTURE_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_NAME, lengthof( sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_NAME ),
                                                                     ( uint8_t * ) sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_VALUE, lengthof( sampleazureiotkitPROCESSOR_MANUFACTURER_PROPERTY_VALUE ) );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotkitTOTAL_STORAGE_PROPERTY_NAME, lengthof( sampleazureiotkitTOTAL_STORAGE_PROPERTY_NAME ),
                                                                     sampleazureiotkitTOTAL_STORAGE_PROPERTY_VALUE, 0 );
    configASSERT( xAzIoTResult == eAzureIoTSuccess );

    xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotkitTOTAL_MEMORY_PROPERTY_NAME, lengthof( sampleazureiotkitTOTAL_MEMORY_PROPERTY_NAME ),
                                                                     sampleazureiotkitTOTAL_MEMORY_PROPERTY_VALUE, 0 );
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
        led1_set_state( xLed1State ? LED_STATE_ON : LED_STATE_OFF );

        *pulResponseStatus = AZ_IOT_STATUS_OK;
        ulCommandResponsePayloadLength = lengthof( sampleazureiotCOMMAND_EMPTY_PAYLOAD );
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }
    else if ( strncmp( ( const char * ) pxMessage->pucCommandName, sampleazureiotCOMMAND_TOGGLE_LED2, pxMessage->usCommandNameLength ) == 0)
    {
        xLed2State = !xLed2State;
        led2_set_state( xLed2State ? LED_STATE_ON : LED_STATE_OFF );

        *pulResponseStatus = AZ_IOT_STATUS_OK;
        ulCommandResponsePayloadLength = lengthof( sampleazureiotCOMMAND_EMPTY_PAYLOAD );
        configASSERT( ulCommandResponsePayloadBufferSize >= ulCommandResponsePayloadLength );
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }
    else if ( strncmp( ( const char * ) pxMessage->pucCommandName, sampleazureiotCOMMAND_DISPLAY_TEXT, pxMessage->usCommandNameLength ) == 0)
    {
        oled_clean_screen();
        uint32_t ulStringLength = UNQUOTED_STRING_LENGTH( pxMessage->ulPayloadLength );
        
        oled_show_message( ( const uint8_t * ) UNQUOTE_STRING( pxMessage->pvMessagePayload ),
                            ulStringLength <= OLED_DISPLAY_MAX_STRING_LENGTH ? ulStringLength : OLED_DISPLAY_MAX_STRING_LENGTH );

        *pulResponseStatus = AZ_IOT_STATUS_OK;
        ulCommandResponsePayloadLength = lengthof( sampleazureiotCOMMAND_EMPTY_PAYLOAD );
        configASSERT( ulCommandResponsePayloadBufferSize >= ulCommandResponsePayloadLength );
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }
    else
    {
        *pulResponseStatus = AZ_IOT_STATUS_NOT_FOUND;
        ulCommandResponsePayloadLength = lengthof( sampleazureiotCOMMAND_EMPTY_PAYLOAD );
        configASSERT( ulCommandResponsePayloadBufferSize >= ulCommandResponsePayloadLength );
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
        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_TEMPERATURE, lengthof( sampleazureiotTELEMETRY_TEMPERATURE ), xTemperature, 2 );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_HUMIDITY, lengthof( sampleazureiotTELEMETRY_HUMIDITY ), xHumidity, 2 );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_LIGHT, lengthof( sampleazureiotTELEMETRY_LIGHT ), xLight, 2 );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        // Pressure, Altitude
        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_PRESSURE, lengthof( sampleazureiotTELEMETRY_PRESSURE ), xPressure, 2 );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_ALTITUDE, lengthof( sampleazureiotTELEMETRY_ALTITUDE ), xAltitude, 2 );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        // Magnetometer
        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_MAGNETOMETERX, lengthof( sampleazureiotTELEMETRY_MAGNETOMETERX ), lMagnetometerX );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_MAGNETOMETERY, lengthof( sampleazureiotTELEMETRY_MAGNETOMETERY ), lMagnetometerY );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_MAGNETOMETERZ, lengthof( sampleazureiotTELEMETRY_MAGNETOMETERZ ), lMagnetometerZ );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        // Pitch, Roll, Accelleration
        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_PITCH, lengthof( sampleazureiotTELEMETRY_PITCH ), lPitch );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_ROLL, lengthof( sampleazureiotTELEMETRY_ROLL ), lRoll );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_ACCELEROMETERX, lengthof( sampleazureiotTELEMETRY_ACCELEROMETERX ), lAccelerometerX );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_ACCELEROMETERY, lengthof( sampleazureiotTELEMETRY_ACCELEROMETERY ), lAccelerometerY );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        xAzIoTResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * )sampleazureiotTELEMETRY_ACCELEROMETERZ, lengthof( sampleazureiotTELEMETRY_ACCELEROMETERZ ), lAccelerometerZ );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        // Complete Json Content
        xAzIoTResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
        configASSERT( xAzIoTResult == eAzureIoTSuccess );

        lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );
        configASSERT( lBytesWritten > 0 );

        xLastTelemetrySendTime = xNow;
    }

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
                                                 lengthof( sampleazureiotPROPERTY_TELEMETRY_FREQUENCY ) ) )
        {
            xAzIoTResult = AzureIoTJSONReader_NextToken( &xJsonReader );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );

            xAzIoTResult = AzureIoTJSONReader_GetTokenInt32( &xJsonReader, &lTelemetryFrequencySecs );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );

            *pulWritablePropertyResponseBufferLength = prvGenerateAckForTelemetryFrequencyPropertyUpdate(
                                                            pucWritablePropertyResponseBuffer,
                                                            ulWritablePropertyResponseBufferSize,
                                                            ulPropertyVersion );

            ESP_LOGI( TAG, "Telemetry frequency set to once every %d seconds.\r\n", lTelemetryFrequencySecs );

            xAzIoTResult = AzureIoTJSONReader_NextToken( &xJsonReader );
            configASSERT( xAzIoTResult == eAzureIoTSuccess );
        }
        else
        {
            LogInfo( ( "Unknown property arrived: skipping over it." ) );

            /* Unknown property arrived. We have to skip over the property and value to continue iterating. */
            prvSkipPropertyAndValue( &xJsonReader );
        }
    }

    if( xAzIoTResult != eAzureIoTErrorEndOfProperties )
    {
        LogError( ( "There was an error parsing the properties: result 0x%08x", xAzIoTResult ) );
    }
    else
    {
        LogInfo( ( "Successfully parsed properties" ) );
    }
}
/*-----------------------------------------------------------*/

static bool xUpdateDeviceProperties = true;

uint32_t ulSampleCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                                 uint32_t ulPropertiesDataSize )
{
    /* No reported properties to send if length is zero. */
    uint32_t lBytesWritten = 0;

    if ( xUpdateDeviceProperties )
    {
        lBytesWritten = lGenerateDeviceInfo( pucPropertiesData, ulPropertiesDataSize );
        configASSERT( lBytesWritten > 0 );
        xUpdateDeviceProperties = false;
    }

    return lBytesWritten;
}
/*-----------------------------------------------------------*/
