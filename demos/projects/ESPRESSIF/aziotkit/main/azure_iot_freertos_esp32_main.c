// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"

#include "sample_azure_iot_pnp.h"
#include "led.h"
#include "sensor_manager.h"

/*-----------------------------------------------------------*/

#define NR_OF_IP_ADDRESSES_TO_WAIT_FOR 1

#if CONFIG_SAMPLE_IOT_WIFI_SCAN_METHOD_FAST
#define SAMPLE_IOT_WIFI_SCAN_METHOD WIFI_FAST_SCAN
#elif CONFIG_SAMPLE_IOT_WIFI_SCAN_METHOD_ALL_CHANNEL
#define SAMPLE_IOT_WIFI_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#endif

#if CONFIG_SAMPLE_IOT_WIFI_CONNECT_AP_BY_SIGNAL
#define SAMPLE_IOT_WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#elif CONFIG_SAMPLE_IOT_WIFI_CONNECT_AP_BY_SECURITY
#define SAMPLE_IOT_WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SECURITY
#endif

#if CONFIG_SAMPLE_IOT_WIFI_AUTH_OPEN
#define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WEP
#define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA_PSK
#define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA2_PSK
#define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA_WPA2_PSK
#define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA2_ENTERPRISE
#define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_ENTERPRISE
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA3_PSK
#define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA2_WPA3_PSK
#define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WAPI_PSK
#define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define INDEFINITE_TIME                            ( ( time_t ) -1 )

#define SNTP_SERVER_FQDN                           "pool.ntp.org"

/*-----------------------------------------------------------*/

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

#define sampleazureiotPROPERTY_STATUS_SUCCESS      200
#define sampleazureiotPROPERTY_SUCCESS             "success"
#define sampleazureiotPROPERTY_TELEMETRY_FREQUENCY ( "telemetryFrequencySecs" )

static int lTelemetryFrequencySecs = 2;
static time_t xLastTelemetrySendTime = INDEFINITE_TIME;

static bool xUpdateDeviceProperties = true;

static uint8_t ucPropertyPayloadBuffer[ 384 ];

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

/*-----------------------------------------------------------*/

static const char *TAG = "sample_azureiot";

static bool xTimeInitialized = false;

static xSemaphoreHandle xSemphGetIpAddrs;
static esp_ip4_addr_t xIpAddress;

/*-----------------------------------------------------------*/

extern void vStartDemoTask( void );

/*-----------------------------------------------------------*/

/**
 * @brief Checks the netif description if it contains specified prefix.
 * All netifs created within common connect component are prefixed with the module TAG,
 * so it returns true if the specified netif is owned by this module
 */
static bool prvIsOurNetif( const char * pcPrefix, esp_netif_t * pxNetif )
{
    return strncmp( pcPrefix, esp_netif_get_desc( pxNetif ), strlen( pcPrefix ) - 1) == 0;
}

/*-----------------------------------------------------------*/

static void prvOnGotIpAddress( void * pvArg, esp_event_base_t xEventBase,
                               int32_t lEventId, void * pvEventData)
{
    ip_event_got_ip_t * pxEvent = ( ip_event_got_ip_t * )pvEventData;

    if ( !prvIsOurNetif( TAG, pxEvent->esp_netif ) )
    {
        ESP_LOGW( TAG, "Got IPv4 from another interface \"%s\": ignored",
                  esp_netif_get_desc( pxEvent->esp_netif ) );
        return;
    }
    ESP_LOGI( TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR,
              esp_netif_get_desc( pxEvent->esp_netif ), IP2STR( &pxEvent->ip_info.ip ) );
    memcpy( &xIpAddress, &pxEvent->ip_info.ip, sizeof( xIpAddress ));
    xSemaphoreGive( xSemphGetIpAddrs );
}
/*-----------------------------------------------------------*/

static void prvOnWifiDisconnect( void * pvArg, esp_event_base_t xEventBase,
                                 int32_t lEventId, void * pvEventData)
{
    ESP_LOGI( TAG, "Wi-Fi disconnected, trying to reconnect..." );
    esp_err_t xError = esp_wifi_connect();

    if ( xError == ESP_ERR_WIFI_NOT_STARTED )
    {
        return;
    }

    ESP_ERROR_CHECK( xError );
}
/*-----------------------------------------------------------*/

static esp_netif_t * prvGetExampleNetifFromDesc( const char * pcDesc )
{
    esp_netif_t * pxNetif = NULL;
    char * pcExpectedDesc;
    asprintf( &pcExpectedDesc, "%s: %s", TAG, pcDesc );
    while ( ( pxNetif = esp_netif_next( pxNetif ) ) != NULL )
    {
        if ( strcmp( esp_netif_get_desc( pxNetif ), pcExpectedDesc ) == 0 )
        {
            free( pcExpectedDesc );
            return pxNetif;
        }
    }
    free( pcExpectedDesc );
    return pxNetif;
}
/*-----------------------------------------------------------*/

static esp_netif_t * prvWifiStart( void )
{
    char * pcDesc;
    wifi_init_config_t xWifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &xWifiInitConfig ) );

    esp_netif_inherent_config_t xEspNetifConfig = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // Prefix the interface description with the module TAG
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    asprintf( &pcDesc, "%s: %s", TAG, xEspNetifConfig.if_desc );
    xEspNetifConfig.if_desc = pcDesc;
    xEspNetifConfig.route_prio = 128;
    esp_netif_t *netif = esp_netif_create_wifi( WIFI_IF_STA, &xEspNetifConfig );
    free( pcDesc );
    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK( esp_event_handler_register( WIFI_EVENT,
                     WIFI_EVENT_STA_DISCONNECTED, &prvOnWifiDisconnect, NULL ) );
    ESP_ERROR_CHECK( esp_event_handler_register( IP_EVENT,
                     IP_EVENT_STA_GOT_IP, &prvOnGotIpAddress, NULL ) );
#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
    ESP_ERROR_CHECK( esp_event_handler_register( WIFI_EVENT,
                     WIFI_EVENT_STA_CONNECTED, &on_wifi_connect, netif ) );
    ESP_ERROR_CHECK( esp_event_handler_register( IP_EVENT,
                     IP_EVENT_GOT_IP6, &prvOnGotIpAddressv6, NULL ) );
#endif

    ESP_ERROR_CHECK( esp_wifi_set_storage( WIFI_STORAGE_RAM ) );

    wifi_config_t xWifiConfig =
    {
        .sta =
        {
            .ssid = CONFIG_SAMPLE_IOT_WIFI_SSID,
            .password = CONFIG_SAMPLE_IOT_WIFI_PASSWORD,
            .scan_method = SAMPLE_IOT_WIFI_SCAN_METHOD,
            .sort_method = SAMPLE_IOT_WIFI_CONNECT_AP_SORT_METHOD,
            .threshold.rssi = CONFIG_SAMPLE_IOT_WIFI_SCAN_RSSI_THRESHOLD,
            .threshold.authmode = SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };
    ESP_LOGI( TAG, "Connecting to %s...", xWifiConfig.sta.ssid );
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_STA, &xWifiConfig ) );
    ESP_ERROR_CHECK( esp_wifi_start( ) );
    esp_wifi_connect();
    return netif;
}

/*-----------------------------------------------------------*/

static void prvWifiStop( void )
{
    esp_netif_t * pxWifiNetif = prvGetExampleNetifFromDesc( "sta" );

    ESP_ERROR_CHECK( esp_event_handler_unregister( WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &prvOnWifiDisconnect ) );
    ESP_ERROR_CHECK( esp_event_handler_unregister( IP_EVENT, IP_EVENT_STA_GOT_IP, &prvOnGotIpAddress ) );
#ifdef CONFIG_EXAMPLE_CONNECT_IPV6
    ESP_ERROR_CHECK( esp_event_handler_unregister( IP_EVENT, IP_EVENT_GOT_IP6, &prvOnGotIpAddressv6 ) );
    ESP_ERROR_CHECK( esp_event_handler_unregister( WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connect ) );
#endif
    esp_err_t err = esp_wifi_stop();
    if ( err == ESP_ERR_WIFI_NOT_INIT )
    {
        return;
    }
    ESP_ERROR_CHECK( err );
    ESP_ERROR_CHECK( esp_wifi_deinit( ) );
    ESP_ERROR_CHECK( esp_wifi_clear_default_wifi_driver_and_handlers( pxWifiNetif ) );
    esp_netif_destroy( pxWifiNetif );
}

/*-----------------------------------------------------------*/

static esp_err_t prvConnectNetwork( void )
{
    if ( xSemphGetIpAddrs != NULL )
    {
        return ESP_ERR_INVALID_STATE;
    }

    ( void ) prvWifiStart( );

    /* create semaphore if at least one interface is active */
    xSemphGetIpAddrs = xSemaphoreCreateCounting( NR_OF_IP_ADDRESSES_TO_WAIT_FOR, 0 );

    ESP_ERROR_CHECK( esp_register_shutdown_handler( &prvWifiStop ) );
    ESP_LOGI( TAG, "Waiting for IP(s)" );
    for ( int lCounter = 0; lCounter < NR_OF_IP_ADDRESSES_TO_WAIT_FOR; ++lCounter )
    {
        xSemaphoreTake( xSemphGetIpAddrs, portMAX_DELAY );
    }
    // iterate over active interfaces, and print out IPs of "our" netifs
    esp_netif_t * pxNetif = NULL;
    esp_netif_ip_info_t xIpInfo;
    for ( int lCounter = 0; lCounter < esp_netif_get_nr_of_ifs( ); ++lCounter )
    {
        pxNetif = esp_netif_next( pxNetif );
        if ( prvIsOurNetif( TAG, pxNetif ) )
        {
            ESP_LOGI( TAG, "Connected to %s", esp_netif_get_desc( pxNetif ) );

            ESP_ERROR_CHECK( esp_netif_get_ip_info( pxNetif, &xIpInfo ) );

            ESP_LOGI( TAG, "- IPv4 address: " IPSTR, IP2STR( &xIpInfo.ip ) );
        }
    }

    return ESP_OK;
}
/*-----------------------------------------------------------*/

/**
 * @brief Callback to confirm time update through NTP.
 */
static void prvTimeSyncNotificationCallback( struct timeval * pxTimeVal )
{
    ( void ) pxTimeVal;
    ESP_LOGI( TAG, "Notification of a time synchronization event" );
    xTimeInitialized = true;
}
/*-----------------------------------------------------------*/

/**
 * @brief Updates the device time using NTP.
 */
static void prvInitializeTime()
{
    sntp_setoperatingmode( SNTP_OPMODE_POLL );
    sntp_setservername( 0, SNTP_SERVER_FQDN );
    sntp_set_time_sync_notification_cb( prvTimeSyncNotificationCallback );
    sntp_init( );

    ESP_LOGI( TAG, "Waiting for time synchronization with SNTP server" );

    while ( !xTimeInitialized )
    {
        vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }
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
static void prvHandleWritablePropertiesUpdate( AzureIoTHubClientPropertiesResponse_t * pxMessage )
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

/**
 * @brief Property message callback handler
 */
void vHandleProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                        void * pvContext )
{
    ( void ) pvContext;

    ESP_LOGI( TAG, "Property document payload : %.*s \r\n",
              pxMessage->ulPayloadLength,
              ( const char * ) pxMessage->pvMessagePayload );

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubPropertiesRequestedMessage:
            ESP_LOGI( TAG, "Device property document GET received\r\n" );

            break;
        case eAzureIoTHubPropertiesWritablePropertyMessage:
            ESP_LOGI( TAG, "Device writeable property received\r\n" );

            prvHandleWritablePropertiesUpdate(pxMessage);

            break;

        case eAzureIoTHubPropertiesReportedResponseMessage:
            ESP_LOGI( TAG, "Device reported property response received\r\n" );
            break;

        default:
            ESP_LOGE( TAG, "Unknown property message: 0x%08x\r\n", pxMessage->xMessageType );
            configASSERT( false );
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Command message callback handler
 */
uint32_t ulHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
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
        toggle_wifi_led( xLed1State );

        *pulResponseStatus = 200;
        ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }
    else if ( strncmp( ( const char * ) pxMessage->pucCommandName, sampleazureiotCOMMAND_TOGGLE_LED2, pxMessage->usCommandNameLength ) == 0)
    {
        xLed2State = !xLed2State;
        toggle_azure_led( xLed2State );

        *pulResponseStatus = 200;
        ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }
    else if ( strncmp( ( const char * ) pxMessage->pucCommandName, sampleazureiotCOMMAND_DISPLAY_TEXT, pxMessage->usCommandNameLength ) == 0)
    {
        oled_clean_screen();
        oled_show_message( ( const uint8_t * ) pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );

        *pulResponseStatus = 200;
        ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }
    else
    {
        *pulResponseStatus = 404;
        ulCommandResponsePayloadLength = sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1;
        (void)memcpy( pucCommandResponsePayloadBuffer, sampleazureiotCOMMAND_EMPTY_PAYLOAD, ulCommandResponsePayloadLength );
    }

    return ulCommandResponsePayloadLength;
}

/*-----------------------------------------------------------*/

/**
 * @brief Implements the sample interface for generating Telemetry payload.
 */
uint32_t ulCreateTelemetry( uint8_t * pucTelemetryData,
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


static int32_t prvGenerateDeviceInfo( uint8_t * pucPropertiesData,
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
 * @brief Implements the sample interface for generating reported properties payload.
 */
uint32_t ulCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                           uint32_t ulPropertiesDataSize )
{
    uint32_t lBytesWritten = 0;

    if ( xUpdateDeviceProperties )
    {
        lBytesWritten = prvGenerateDeviceInfo( pucPropertiesData, ulPropertiesDataSize );

        xUpdateDeviceProperties = false;
    }

    return lBytesWritten;
}

/*-----------------------------------------------------------*/

uint64_t ullGetUnixTime( void )
{
    time_t now = time(NULL);

    if ( now == INDEFINITE_TIME )
    {
        ESP_LOGE(TAG, "Failed obtaining current time.\r\n");
    }

    return now;
}

/*-----------------------------------------------------------*/

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init( ) );
    ESP_ERROR_CHECK( esp_netif_init( ) );
    ESP_ERROR_CHECK( esp_event_loop_create_default( ) );

    //Allow other core to finish initialization
    vTaskDelay( pdMS_TO_TICKS( 100 ) );

    initialize_sensors( );

    ( void ) prvConnectNetwork( );

    prvInitializeTime( );

    vStartDemoTask( );
}
/*-----------------------------------------------------------*/
