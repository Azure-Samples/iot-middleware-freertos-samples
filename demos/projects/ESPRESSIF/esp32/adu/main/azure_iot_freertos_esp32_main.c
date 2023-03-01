/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

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

#include "sample_azure_iot_pnp_data_if.h"

/* Azure Device Update */
#include <azure/iot/az_iot_adu_client.h>
/*-----------------------------------------------------------*/

#define NR_OF_IP_ADDRESSES_TO_WAIT_FOR     1

#if CONFIG_SAMPLE_IOT_WIFI_SCAN_METHOD_FAST
    #define SAMPLE_IOT_WIFI_SCAN_METHOD    WIFI_FAST_SCAN
#elif CONFIG_SAMPLE_IOT_WIFI_SCAN_METHOD_ALL_CHANNEL
    #define SAMPLE_IOT_WIFI_SCAN_METHOD    WIFI_ALL_CHANNEL_SCAN
#endif

#if CONFIG_SAMPLE_IOT_WIFI_CONNECT_AP_BY_SIGNAL
    #define SAMPLE_IOT_WIFI_CONNECT_AP_SORT_METHOD    WIFI_CONNECT_AP_BY_SIGNAL
#elif CONFIG_SAMPLE_IOT_WIFI_CONNECT_AP_BY_SECURITY
    #define SAMPLE_IOT_WIFI_CONNECT_AP_SORT_METHOD    WIFI_CONNECT_AP_BY_SECURITY
#endif

#if CONFIG_SAMPLE_IOT_WIFI_AUTH_OPEN
    #define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD    WIFI_AUTH_OPEN
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WEP
    #define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD    WIFI_AUTH_WEP
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA_PSK
    #define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD    WIFI_AUTH_WPA_PSK
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA2_PSK
    #define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD    WIFI_AUTH_WPA2_PSK
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA_WPA2_PSK
    #define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD    WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA2_ENTERPRISE
    #define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD    WIFI_AUTH_WPA2_ENTERPRISE
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA3_PSK
    #define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD    WIFI_AUTH_WPA3_PSK
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WPA2_WPA3_PSK
    #define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD    WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_SAMPLE_IOT_WIFI_AUTH_WAPI_PSK
    #define SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD    WIFI_AUTH_WAPI_PSK
#endif /* if CONFIG_SAMPLE_IOT_WIFI_AUTH_OPEN */

#define INDEFINITE_TIME                                 ( ( time_t ) -1 )

#define SNTP_SERVER_FQDN                                "pool.ntp.org"

/*-----------------------------------------------------------*/

static const char * TAG = "sample_azureiotkit";

static bool xTimeInitialized = false;

static SemaphoreHandle_t xSemphGetIpAddrs;
static esp_ip4_addr_t xIpAddress;

/*-----------------------------------------------------------*/

extern void vStartDemoTask( void );
/*-----------------------------------------------------------*/

/**
 * @brief Checks the netif description if it contains specified prefix.
 * All netifs created within common connect component are prefixed with the module TAG,
 * so it returns true if the specified netif is owned by this module
 */
static bool prvIsOurNetif( const char * pcPrefix,
                           esp_netif_t * pxNetif )
{
    return strncmp( pcPrefix, esp_netif_get_desc( pxNetif ), strlen( pcPrefix ) - 1 ) == 0;
}
/*-----------------------------------------------------------*/

static void prvOnGotIpAddress( void * pvArg,
                               esp_event_base_t xEventBase,
                               int32_t lEventId,
                               void * pvEventData )
{
    ip_event_got_ip_t * pxEvent = ( ip_event_got_ip_t * ) pvEventData;

    if( !prvIsOurNetif( TAG, pxEvent->esp_netif ) )
    {
        ESP_LOGW( TAG, "Got IPv4 from another interface \"%s\": ignored",
                  esp_netif_get_desc( pxEvent->esp_netif ) );
        return;
    }

    ESP_LOGI( TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR,
              esp_netif_get_desc( pxEvent->esp_netif ), IP2STR( &pxEvent->ip_info.ip ) );
    memcpy( &xIpAddress, &pxEvent->ip_info.ip, sizeof( xIpAddress ) );
    xSemaphoreGive( xSemphGetIpAddrs );
}
/*-----------------------------------------------------------*/

static void prvOnWifiDisconnect( void * pvArg,
                                 esp_event_base_t xEventBase,
                                 int32_t lEventId,
                                 void * pvEventData )
{
    ESP_LOGI( TAG, "Wi-Fi disconnected, trying to reconnect..." );
    esp_err_t xError = esp_wifi_connect();

    if( xError == ESP_ERR_WIFI_NOT_STARTED )
    {
        ESP_LOGE( TAG, "Failed connecting to Wi-Fi" );
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

    while( ( pxNetif = esp_netif_next( pxNetif ) ) != NULL )
    {
        if( strcmp( esp_netif_get_desc( pxNetif ), pcExpectedDesc ) == 0 )
        {
            break;
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
    /* Prefix the interface description with the module TAG */
    /* Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask) */
    asprintf( &pcDesc, "%s: %s", TAG, xEspNetifConfig.if_desc );
    xEspNetifConfig.if_desc = pcDesc;
    xEspNetifConfig.route_prio = 128;
    esp_netif_t * netif = esp_netif_create_wifi( WIFI_IF_STA, &xEspNetifConfig );
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
        .sta                    =
        {
            .ssid               = CONFIG_SAMPLE_IOT_WIFI_SSID,
            .password           = CONFIG_SAMPLE_IOT_WIFI_PASSWORD,
            .scan_method        = SAMPLE_IOT_WIFI_SCAN_METHOD,
            .sort_method        = SAMPLE_IOT_WIFI_CONNECT_AP_SORT_METHOD,
            .threshold.rssi     = CONFIG_SAMPLE_IOT_WIFI_SCAN_RSSI_THRESHOLD,
            .threshold.authmode = SAMPLE_IOT_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };
    ESP_LOGI( TAG, "Connecting to %s...", xWifiConfig.sta.ssid );
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_STA, &xWifiConfig ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
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

    if( err == ESP_ERR_WIFI_NOT_INIT )
    {
        return;
    }

    ESP_ERROR_CHECK( err );
    ESP_ERROR_CHECK( esp_wifi_deinit() );
    ESP_ERROR_CHECK( esp_wifi_clear_default_wifi_driver_and_handlers( pxWifiNetif ) );
    esp_netif_destroy( pxWifiNetif );
}
/*-----------------------------------------------------------*/

static esp_err_t prvConnectNetwork( void )
{
    if( xSemphGetIpAddrs != NULL )
    {
        return ESP_ERR_INVALID_STATE;
    }

    ( void ) prvWifiStart();

    /* create semaphore if at least one interface is active */
    xSemphGetIpAddrs = xSemaphoreCreateCounting( NR_OF_IP_ADDRESSES_TO_WAIT_FOR, 0 );

    ESP_ERROR_CHECK( esp_register_shutdown_handler( &prvWifiStop ) );
    ESP_LOGI( TAG, "Waiting for IP(s)" );

    for( int lCounter = 0; lCounter < NR_OF_IP_ADDRESSES_TO_WAIT_FOR; ++lCounter )
    {
        xSemaphoreTake( xSemphGetIpAddrs, portMAX_DELAY );
    }

    /* iterate over active interfaces, and print out IPs of "our" netifs */
    esp_netif_t * pxNetif = NULL;
    esp_netif_ip_info_t xIpInfo;

    for( int lCounter = 0; lCounter < esp_netif_get_nr_of_ifs(); ++lCounter )
    {
        pxNetif = esp_netif_next( pxNetif );

        if( prvIsOurNetif( TAG, pxNetif ) )
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
    sntp_init();

    ESP_LOGI( TAG, "Waiting for time synchronization with SNTP server" );

    while( !xTimeInitialized )
    {
        vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }
}
/*-----------------------------------------------------------*/

uint64_t ullGetUnixTime( void )
{
    time_t now = time( NULL );

    if( now == INDEFINITE_TIME )
    {
        ESP_LOGE( TAG, "Failed obtaining current time.\r\n" );
    }

    return now;
}
/*-----------------------------------------------------------*/

void app_main( void )
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK( esp_netif_init() );
    ESP_ERROR_CHECK( esp_event_loop_create_default() );

    /*Allow other core to finish initialization */
    vTaskDelay( pdMS_TO_TICKS( 100 ) );

    ( void ) prvConnectNetwork();

    prvInitializeTime();

    vStartDemoTask();
}
/*-----------------------------------------------------------*/
