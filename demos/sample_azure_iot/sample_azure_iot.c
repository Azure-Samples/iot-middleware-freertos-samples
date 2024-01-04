/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* Demo Specific Interface Functions. */
#include "azure_sample_connection.h"

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
#include "transport_tls_socket.h"

/* Crypto helper header. */
#include "azure_sample_crypto.h"

/* Azure JSON includes */
#include "azure_iot_json_writer.h"

// For IoT data reading
#include "system_data.h"

// Overriding the asserts to let IoT connectivity continue.
// @todo: Before restarting unsubscribing and TLS disconnect might not
//        need to be done because the assert might be because of
//        several software reasons (just not TLS/ socket disconnection).
// @todo: We may not need this in PCBA but it might be good to check
//        if the TLS issue happens with ethernet too.
#include "stm32l4xx_hal.h"
#undef configASSERT
#define configASSERT( x )                                                    \
    if( ( x ) == 0 )                                                         \
    {                                                                        \
        vLoggingPrintf( "[FATAL] [%s:%d] %s\r\n", __func__, __LINE__, # x ); \
        NVIC_SystemReset();                                                  \
    }

/*-----------------------------------------------------------*/

/* Compile time error for undefined configs. */
#if !defined( democonfigHOSTNAME ) && !defined( democonfigENABLE_DPS_SAMPLE )
    #error "Define the config democonfigHOSTNAME by following the instructions in file demo_config.h."
#endif

#if !defined( democonfigENDPOINT ) && defined( democonfigENABLE_DPS_SAMPLE )
    #error "Define the config dps endpoint by following the instructions in file demo_config.h."
#endif

#ifndef democonfigROOT_CA_PEM
    #error "Please define Root CA certificate of the IoT Hub(democonfigROOT_CA_PEM) in demo_config.h."
#endif

#if defined( democonfigDEVICE_SYMMETRIC_KEY ) && defined( democonfigCLIENT_CERTIFICATE_PEM )
    #error "Please define only one auth democonfigDEVICE_SYMMETRIC_KEY or democonfigCLIENT_CERTIFICATE_PEM in demo_config.h."
#endif

#if !defined( democonfigDEVICE_SYMMETRIC_KEY ) && !defined( democonfigCLIENT_CERTIFICATE_PEM )
    #error "Please define one auth democonfigDEVICE_SYMMETRIC_KEY or democonfigCLIENT_CERTIFICATE_PEM in demo_config.h."
#endif

/*-----------------------------------------------------------*/

/**
 * @brief The maximum number of retries for network operation with server.
 */
#define sampleazureiotRETRY_MAX_ATTEMPTS                      ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS              ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define sampleazureiotRETRY_BACKOFF_BASE_MS                   ( 500U )

/**
 * @brief Timeout for receiving CONNACK packet in milliseconds.
 */
#define sampleazureiotCONNACK_RECV_TIMEOUT_MS                 ( 10 * 1000U )

/**
 * @brief The Telemetry message published in this example.
 */
// #define sampleazureiotMESSAGE                                 "Hello World : %d !"
// #define sampleazureiotMESSAGE                                 "{\"temperature\":%f,\"humidity\":%f,\"co2\":%f}"

/**
 * @brief Telemetry Values
 */
// Primary objects
#define sampleazureiotTELEMETRY_OBJECT_SKID                       ( "skid" )
#define sampleazureiotTELEMETRY_OBJECT_UNITS                      ( "units" )
// #define sampleazureiotTELEMETRY_OBJECT_EXTERNAL                   ( "environment" )
// #define sampleazureiotTELEMETRY_OBJECT_STATE                      ( "state" )
// #define sampleazureiotTELEMETRY_OBJECT_ERROR_STATE                ( "error-state" )

// Secondary
#define sampleazureiotTELEMETRY_OBJECT_UNIT1                      ( "unit1" )
#define sampleazureiotTELEMETRY_OBJECT_UNIT2                      ( "unit2" )
#define sampleazureiotTELEMETRY_OBJECT_UNIT3                      ( "unit3" )
// #define sampleazureiotTELEMETRY_OBJECT_PRESSURE                   ( "pressure" )
// sensor objects
#define sampleazureiotTELEMETRY_OBJECT_O2                         ( "o2" )
#define sampleazureiotTELEMETRY_OBJECT_MASSFLOW                   ( "mass_flow" )
#define sampleazureiotTELEMETRY_OBJECT_CO2                        ( "co2" )
#define sampleazureiotTELEMETRY_OBJECT_TANK_PRESSURE              ( "tank_pressure" )
#define sampleazureiotTELEMETRY_OBJECT_TEMPERATURE                ( "temperature" )
#define sampleazureiotTELEMETRY_OBJECT_HUMIDITY                   ( "humidity" )
#define sampleazureiotTELEMETRY_OBJECT_VACUUM_SENSOR              ( "vacuum_sensor" )
#define sampleazureiotTELEMETRY_OBJECT_AMBIENT_HUMIDITY           ( "ambient_humidity" )
#define sampleazureiotTELEMETRY_OBJECT_AMBIENT_TEMPERATURE        ( "ambient_temperature" )
#define sampleazureiotTELEMETRY_OBJECT_PROPOTIONAL_VALVE_SENSOR   ( "proportional_valve_pressure" )

// Heaters and catridge objects
#define sampleazureiotTELEMETRY_OBJECT_HEATERS                    ( "heaters" )
#define sampleazureiotTELEMETRY_OBJECT_CARTRIDGE11                ( "cartridge1-1" )
#define sampleazureiotTELEMETRY_OBJECT_CARTRIDGE12                ( "cartridge1-2" )
#define sampleazureiotTELEMETRY_OBJECT_CARTRIDGE13                ( "cartridge1-3" )
#define sampleazureiotTELEMETRY_OBJECT_CARTRIDGE21                ( "cartridge2-1" )
#define sampleazureiotTELEMETRY_OBJECT_CARTRIDGE22                ( "cartridge2-2" )
#define sampleazureiotTELEMETRY_OBJECT_CARTRIDGE23                ( "cartridge2-3" )
#define sampleazureiotTELEMETRY_OBJECT_CARTRIDGE31                ( "cartridge3-1" )
#define sampleazureiotTELEMETRY_OBJECT_CARTRIDGE32                ( "cartridge3-2" )
#define sampleazureiotTELEMETRY_OBJECT_CARTRIDGE33                ( "cartridge3-3" )
// Status related
#define sampleazureiotTELEMETRY_OBJECT_COMPONENT_STATUS           ( "component_status" )
#define sampleazureiotTELEMETRY_OBJECT_SKID_STATUS                ( "skid_status" )
#define sampleazureiotTELEMETRY_OBJECT_UNIT_STATUS                ( "unit_status" )
#define sampleazureiotTELEMETRY_OBJECT_VALVE_STATUS               ( "valve_status" )

// Telemetry
#define sampleazureiotTELEMETRY_SKID_STATE                        ( "skid_state" )
#define sampleazureiotTELEMETRY_UNIT_STATE                        ( "unit_state" )
#define sampleazureiotTELEMETRY_STATUS                            ( "status" )
#define sampleazureiotTELEMETRY_AVG                               ( "avg" )
#define sampleazureiotTELEMETRY_MAX                               ( "max" )
#define sampleazureiotTELEMETRY_MIN                               ( "min" )
// components status
#define sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_BEFORE_WATER_TRAP             ( "two_way_gas_valve_before_water_trap" )
#define sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_IN_WATER_TRAP                 ( "two_way_gas_valve_in_water_trap" )
#define sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_AFTER_WATER_TRAP              ( "two_way_gas_valve_after_water_trap" )
#define sampleazureiotTELEMETRY_VACUUM_RELEASE_VALVE_IN_WATER_TRAP              ( "vacuum_release_valve_in_water_trap" )
#define sampleazureiotTELEMETRY_THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSER ( "three_way_vacuum_release_valve_before_condenser" )
#define sampleazureiotTELEMETRY_THREE_WAY_VALVE_AFTER_VACUUM_PUMP               ( "three_way_valve_after_vacuum_pump" )
#define sampleazureiotTELEMETRY_COMPRESSOR                                      ( "compressor" )
#define sampleazureiotTELEMETRY_VACUUM_PUMP                                     ( "vacuum_pump" )
#define sampleazureiotTELEMETRY_CONDENSER                                       ( "condenser" )
// Skid/Unit status
#define sampleazureiotTELEMETRY_ERROR_FLAG                                      ( "error_flag" )
#define sampleazureiotTELEMETRY_HALT_FLAG                                       ( "halt_flag" )
#define sampleazureiotTELEMETRY_RESET_FLAG                                      ( "reset_flag" )
#define sampleazureiotTELEMETRY_JUST_STARTED_FLAG                               ( "just_started_flag" )
#define sampleazureiotTELEMETRY_SETUP_STATE_SYNCHING_FLAG                       ( "setup_state_synching_flag" )
// Valve status
#define sampleazureiotTELEMETRY_FAN_STATUS                                      ( "fan_status" )
#define sampleazureiotTELEMETRY_BUTTERFLY_VALVE_1_STATUS                        ( "butterfly_valve_1_status" )
#define sampleazureiotTELEMETRY_BUTTERFLY_VALVE_2_STATUS                        ( "butterfly_valve_2_status" )

// #define sampleazureiotTELEMETRY_PRESSURE1                         ( "pressure1" )
// #define sampleazureiotTELEMETRY_PRESSURE2                         ( "pressure2" )
// #define sampleazureiotTELEMETRY_PRESSURE3                         ( "pressure3" )
// #define sampleazureiotTELEMETRY_MASS_FLOW                         ( "massflow" )
// #define sampleazureiotTELEMETRY_CO2                               ( "co2" )
// #define sampleazureiotTELEMETRY_O2                                ( "o2" )
// #define sampleazureiotTELEMETRY_TEMPERATURE                       ( "ambient_temperature" )
// #define sampleazureiotTELEMETRY_CARTRIDGE11                       ( "cartridge11" )
// #define sampleazureiotTELEMETRY_CARTRIDGE12                       ( "cartridge12" )
// #define sampleazureiotTELEMETRY_CARTRIDGE13                       ( "cartridge13" )
// #define sampleazureiotTELEMETRY_CARTRIDGE21                       ( "cartridge21" )
// #define sampleazureiotTELEMETRY_CARTRIDGE22                       ( "cartridge22" )
// #define sampleazureiotTELEMETRY_CARTRIDGE23                       ( "cartridge23" )
// #define sampleazureiotTELEMETRY_CARTRIDGE31                       ( "cartridge31" )
// #define sampleazureiotTELEMETRY_CARTRIDGE32                       ( "cartridge32" )
// #define sampleazureiotTELEMETRY_CARTRIDGE33                       ( "cartridge33" )
// #define sampleazureiotTELEMETRY_VACUUM                            ( "vacuum" )
// #define sampleazureiotTELEMETRY_VIBRATION                         ( "vibration" )
// #define sampleazureiotTELEMETRY_HUMIDITY                          ( "ambient_humidity" )
// #define sampleazureiotTELEMETRY_TANK_PRESSURE                     ( "tank_pressure" )
// #define sampleazureiotTELEMETRY_PWM_DUTY_CYCLE                    ( "pwm_duty_cycle" )

// #define sampleazureiotTELEMETRY_STATUS                     ( "status" )
// #define sampleazureiotTELEMETRY_ERROR_STATUS               ( "errorstatus" )

// For getting the length of the telemetry names
#define lengthof( x )                  ( sizeof( x ) - 1 )

/**
 * @brief  The content type of the Telemetry message published in this example.
 * @remark Message properties must be url-encoded.
 *         This message property is not required to send telemetry.
 */
// #define sampleazureiotMESSAGE_CONTENT_TYPE                    "text%2Fplain"
#define sampleazureiotMESSAGE_CONTENT_TYPE                    "application%2Fjson"

/**
 * @brief  The content encoding of the Telemetry message published in this example.
 * @remark Message properties must be url-encoded.
 *         This message property is not required to send telemetry.
 */
// #define sampleazureiotMESSAGE_CONTENT_ENCODING                "us-ascii"
#define sampleazureiotMESSAGE_CONTENT_ENCODING                "utf-8"

/**
 * @brief The reported property payload to send to IoT Hub
 */
#define sampleazureiotPROPERTY                                "{ \"PropertyIterationForCurrentConnection\": \"%d\" }"

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS     ( pdMS_TO_TICKS( 5000U ) )

/**
 * @brief Timeout for MQTT_ProcessLoop in milliseconds.
 */
#define sampleazureiotPROCESS_LOOP_TIMEOUT_MS                 ( 500U )

/**
 * @brief Delay (in ticks) between consecutive cycles of MQTT publish operations in a
 * demo iteration.
 *
 * Note that the process loop also has a timeout, so the total time between
 * publishes is the sum of the two delays.
 */
#define sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS           ( pdMS_TO_TICKS( 30000U ) )

/**
 * @brief Delay (in ticks) at start/ boot-up.
 * 
 * Note: We do this to allow SKID to send enough telemtry data. We deduct 10s as
 * the boot-up itself can take a bit of time.
*/
#define sampleazureiotDELAY_AT_START                          ( sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS - pdMS_TO_TICKS( 10000U ) )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS          ( 2000U )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotProvisioning_Registration_TIMEOUT_MS    ( 3 * 1000U )

/**
 * @brief Wait timeout for subscribe to finish.
 */
#define sampleazureiotSUBSCRIBE_TIMEOUT                       ( 10 * 1000U )
/*-----------------------------------------------------------*/

/**
 * @brief Unix time.
 *
 * @return Time in milliseconds.
 */
uint64_t ullGetUnixTime( void );
/*-----------------------------------------------------------*/

/* Define buffer for IoT Hub info.  */
#ifdef democonfigENABLE_DPS_SAMPLE
    static uint8_t ucSampleIotHubHostname[ 128 ];
    static uint8_t ucSampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
#endif /* democonfigENABLE_DPS_SAMPLE */

#define SCRATCH_BUFFER_LENGTH 2048
#define MINI_SCRATCH_BUFFER_LENGTH 1572
static uint8_t ucPropertyBuffer[ 80 ];
static uint8_t ucScratchBuffer[ SCRATCH_BUFFER_LENGTH ];

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    void * pParams;
};

static AzureIoTHubClient_t xAzureIoTHubClient;
/*-----------------------------------------------------------*/

#ifdef democonfigENABLE_DPS_SAMPLE

/**
 * @brief Gets the IoT Hub endpoint and deviceId from Provisioning service.
 *   This function will block for Provisioning service for result or return failure.
 *
 * @param[in] pXNetworkCredentials  Network credential used to connect to Provisioning service
 * @param[out] ppucIothubHostname  Pointer to uint8_t* IoT Hub hostname return from Provisioning Service
 * @param[in,out] pulIothubHostnameLength  Length of hostname
 * @param[out] ppucIothubDeviceId  Pointer to uint8_t* deviceId return from Provisioning Service
 * @param[in,out] pulIothubDeviceIdLength  Length of deviceId
 */
    static uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                      uint8_t ** ppucIothubHostname,
                                      uint32_t * pulIothubHostnameLength,
                                      uint8_t ** ppucIothubDeviceId,
                                      uint32_t * pulIothubDeviceIdLength );

#endif /* democonfigENABLE_DPS_SAMPLE */

/**
 * @brief The task used to demonstrate the MQTT API.
 *
 * @param[in] pvParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 */
static void prvAzureDemoTask( void * pvParameters );

/**
 * @brief Connect to endpoint with reconnection retries.
 *
 * If connection fails, retry is attempted after a timeout.
 * Timeout value will exponentially increase until maximum
 * timeout value is reached or the number of attempts are exhausted.
 *
 * @param pcHostName Hostname of the endpoint to connect to.
 * @param ulPort Endpoint port.
 * @param pxNetworkCredentials Pointer to Network credentials.
 * @param pxNetworkContext Point to Network context created.
 * @return uint32_t The status of the final connection attempt.
 */
static uint32_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                      uint32_t ulPort,
                                                      NetworkCredentials_t * pxNetworkCredentials,
                                                      NetworkContext_t * pxNetworkContext );
/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucMQTTMessageBuffer[ democonfigNETWORK_BUFFER_SIZE ];

/*-----------------------------------------------------------*/

/**
 * @brief Cloud message callback handler
 */
static void prvHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                                   void * pvContext )
{
    ( void ) pvContext;

    LogInfo( ( "Cloud message payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );
}
/*-----------------------------------------------------------*/

/**
 * @brief Command message callback handler
 */
static void prvHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                              void * pvContext )
{
    LogInfo( ( "Command payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );

    AzureIoTHubClient_t * xHandle = ( AzureIoTHubClient_t * ) pvContext;

    if( AzureIoTHubClient_SendCommandResponse( xHandle, pxMessage, 200,
                                               NULL, 0 ) != eAzureIoTSuccess )
    {
        LogInfo( ( "Error sending command response\r\n" ) );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Property mesage callback handler
 */
static void prvHandlePropertiesMessage( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                        void * pvContext )
{
    ( void ) pvContext;

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubPropertiesRequestedMessage:
            LogInfo( ( "Device property document GET received" ) );
            break;

        case eAzureIoTHubPropertiesReportedResponseMessage:
            LogInfo( ( "Device property reported property response received" ) );
            break;

        case eAzureIoTHubPropertiesWritablePropertyMessage:
            LogInfo( ( "Device property desired property received" ) );
            break;

        default:
            LogError( ( "Unknown property message" ) );
    }

    LogInfo( ( "Property document payload : %.*s \r\n",
               ( int ) pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );
}
/*-----------------------------------------------------------*/

/**
 * @brief Setup transport credentials.
 */
static uint32_t prvSetupNetworkCredentials( NetworkCredentials_t * pxNetworkCredentials )
{
    pxNetworkCredentials->xDisableSni = pdFALSE;
    /* Set the credentials for establishing a TLS connection. */
    pxNetworkCredentials->pucRootCa = ( const unsigned char * ) democonfigROOT_CA_PEM;
    pxNetworkCredentials->xRootCaSize = sizeof( democonfigROOT_CA_PEM );
    #ifdef democonfigCLIENT_CERTIFICATE_PEM
        pxNetworkCredentials->pucClientCert = ( const unsigned char * ) democonfigCLIENT_CERTIFICATE_PEM;
        pxNetworkCredentials->xClientCertSize = sizeof( democonfigCLIENT_CERTIFICATE_PEM );
        pxNetworkCredentials->pucPrivateKey = ( const unsigned char * ) democonfigCLIENT_PRIVATE_KEY_PEM;
        pxNetworkCredentials->xPrivateKeySize = sizeof( democonfigCLIENT_PRIVATE_KEY_PEM );
    #endif

    return 0;
}
/*-----------------------------------------------------------*/

// Scratch buffers, improve these later
static uint8_t ucScratchTempBuffer[ SCRATCH_BUFFER_LENGTH ];
static uint8_t ucScratchTempHalfBuffer[ MINI_SCRATCH_BUFFER_LENGTH ];
static uint8_t ucScratchTempHalfBuffer2[ MINI_SCRATCH_BUFFER_LENGTH ];

uint32_t prvCreateSkidComponentStatusTelemetry( SKID_iot_status_t skid_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_BEFORE_WATER_TRAP, lengthof( sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_BEFORE_WATER_TRAP ),
                                                                ( uint8_t * )valve_status_stringified[skid_data.two_way_gas_valve_before_water_trap], strlen(valve_status_stringified[skid_data.two_way_gas_valve_before_water_trap]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_IN_WATER_TRAP, lengthof( sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_IN_WATER_TRAP ),
                                                                ( uint8_t * )valve_status_stringified[skid_data.two_way_gas_valve_in_water_trap], strlen(valve_status_stringified[skid_data.two_way_gas_valve_in_water_trap]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_AFTER_WATER_TRAP, lengthof( sampleazureiotTELEMETRY_TWO_WAY_GAS_VALVE_AFTER_WATER_TRAP ),
                                                                ( uint8_t * )valve_status_stringified[skid_data.two_way_gas_valve_after_water_trap], strlen(valve_status_stringified[skid_data.two_way_gas_valve_after_water_trap]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_VACUUM_RELEASE_VALVE_IN_WATER_TRAP, lengthof( sampleazureiotTELEMETRY_VACUUM_RELEASE_VALVE_IN_WATER_TRAP ),
                                                                ( uint8_t * )valve_status_stringified[skid_data.vacuum_release_valve_in_water_trap], strlen(valve_status_stringified[skid_data.vacuum_release_valve_in_water_trap]));
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSER, lengthof( sampleazureiotTELEMETRY_THREE_WAY_VACUUM_RELEASE_VALVE_BEFORE_CONDENSER ),
                                                                ( uint8_t * )three_way_vacuum_release_valve_before_condensator_stringified[skid_data.three_way_vacuum_release_valve_before_condenser],
                                                                strlen(three_way_vacuum_release_valve_before_condensator_stringified[skid_data.three_way_vacuum_release_valve_before_condenser]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_THREE_WAY_VALVE_AFTER_VACUUM_PUMP, lengthof( sampleazureiotTELEMETRY_THREE_WAY_VALVE_AFTER_VACUUM_PUMP ),
                                                                ( uint8_t * )three_way_valve_after_vacuum_pump_stringified[skid_data.three_valve_after_vacuum_pump],
                                                                strlen(three_way_valve_after_vacuum_pump_stringified[skid_data.three_valve_after_vacuum_pump]));
    configASSERT( xResult == eAzureIoTSuccess )

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_COMPRESSOR, lengthof( sampleazureiotTELEMETRY_COMPRESSOR ),
                                                                ( uint8_t * )component_status_stringified[skid_data.compressor], strlen(component_status_stringified[skid_data.compressor]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_VACUUM_PUMP, lengthof( sampleazureiotTELEMETRY_VACUUM_PUMP ),
                                                                ( uint8_t * )component_status_stringified[skid_data.vacuum_pump], strlen(component_status_stringified[skid_data.vacuum_pump]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CONDENSER, lengthof( sampleazureiotTELEMETRY_CONDENSER ),
                                                                ( uint8_t * )component_status_stringified[skid_data.condenser], strlen(component_status_stringified[skid_data.condenser]));
    configASSERT( xResult == eAzureIoTSuccess );

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid component status data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnitComponentStatusTelemetry( UNIT_iot_status_t unit_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_FAN_STATUS, lengthof( sampleazureiotTELEMETRY_FAN_STATUS ),
                                                                ( uint8_t * )component_status_stringified[unit_data.fan_status], strlen(component_status_stringified[unit_data.fan_status]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_BUTTERFLY_VALVE_1_STATUS, lengthof( sampleazureiotTELEMETRY_BUTTERFLY_VALVE_1_STATUS ),
                                                                ( uint8_t * )valve_status_stringified[unit_data.butterfly_valve_1_status], strlen(valve_status_stringified[unit_data.butterfly_valve_1_status]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_BUTTERFLY_VALVE_2_STATUS, lengthof( sampleazureiotTELEMETRY_BUTTERFLY_VALVE_2_STATUS ),
                                                                ( uint8_t * )valve_status_stringified[unit_data.butterfly_valve_2_status], strlen(valve_status_stringified[unit_data.butterfly_valve_2_status]));
    configASSERT( xResult == eAzureIoTSuccess );

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid component status data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateSkidStatusTelemetry( SKID_iot_status_t skid_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_ERROR_FLAG, lengthof( sampleazureiotTELEMETRY_ERROR_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[skid_data.error_flag], strlen(flag_state_stringified[skid_data.error_flag]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_HALT_FLAG, lengthof( sampleazureiotTELEMETRY_HALT_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[skid_data.halt_flag], strlen(flag_state_stringified[skid_data.halt_flag]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_RESET_FLAG, lengthof( sampleazureiotTELEMETRY_RESET_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[skid_data.reset_flag], strlen(flag_state_stringified[skid_data.reset_flag]));
    configASSERT( xResult == eAzureIoTSuccess );

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid status data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnitStatusTelemetry( UNIT_iot_status_t unit_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_ERROR_FLAG, lengthof( sampleazureiotTELEMETRY_ERROR_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[unit_data.error_flag], strlen(flag_state_stringified[unit_data.error_flag]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_HALT_FLAG, lengthof( sampleazureiotTELEMETRY_HALT_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[unit_data.halt_flag], strlen(flag_state_stringified[unit_data.halt_flag]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_RESET_FLAG, lengthof( sampleazureiotTELEMETRY_RESET_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[unit_data.reset_flag], strlen(flag_state_stringified[unit_data.reset_flag]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_JUST_STARTED_FLAG, lengthof( sampleazureiotTELEMETRY_JUST_STARTED_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[unit_data.just_started_flag], strlen(flag_state_stringified[unit_data.just_started_flag]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SETUP_STATE_SYNCHING_FLAG, lengthof( sampleazureiotTELEMETRY_SETUP_STATE_SYNCHING_FLAG ),
                                                                ( uint8_t * )flag_state_stringified[unit_data.setup_state_synching_flag], strlen(flag_state_stringified[unit_data.setup_state_synching_flag]));
    configASSERT( xResult == eAzureIoTSuccess );

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Unit status data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateSkidSensorTelemetry( sensor_name_t name, SKID_iot_status_t skid_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    double sensor_avg = 0.0;
    double sensor_max = 0.0;
    double sensor_min = 0.0;

    // Get the desired sensor avg,max,min
    switch(name){
        // Skid sensors
        case SKID_O2:
            sensor_avg = skid_data.o2_sensor.avg;
            sensor_max = skid_data.o2_sensor.max;
            sensor_min = skid_data.o2_sensor.min;
        break;
        case SKID_MASS_FLOW:
            sensor_avg = skid_data.mass_flow.avg;
            sensor_max = skid_data.mass_flow.max;
            sensor_min = skid_data.mass_flow.min;
        break;
        case SKID_CO2:
            sensor_avg = skid_data.co2_sensor.avg;
            sensor_max = skid_data.co2_sensor.max;
            sensor_min = skid_data.co2_sensor.min;
        break;
        case SKID_TANK_PRESSURE:
            sensor_avg = skid_data.tank_pressure.avg;
            sensor_max = skid_data.tank_pressure.max;
            sensor_min = skid_data.tank_pressure.min;
        break;
        case SKID_PROPOTIONAL_VALVE_SENSOR:
            sensor_avg = skid_data.proportional_valve_pressure.avg;
            sensor_max = skid_data.proportional_valve_pressure.max;
            sensor_min = skid_data.proportional_valve_pressure.min;
        break;
        case SKID_TEMPERATURE:
            sensor_avg = skid_data.temperature.avg;
            sensor_max = skid_data.temperature.max;
            sensor_min = skid_data.temperature.min;
        break;
        case SKID_HUMIDITY:
            sensor_avg = skid_data.humidity.avg;
            sensor_max = skid_data.humidity.max;
            sensor_min = skid_data.humidity.min;
        break;
        default:
        break;
    }

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_AVG, lengthof( sampleazureiotTELEMETRY_AVG ), sensor_avg, 3);
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MAX, lengthof( sampleazureiotTELEMETRY_MAX ), sensor_max, 3);
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MIN, lengthof( sampleazureiotTELEMETRY_MIN ), sensor_min, 3);
    configASSERT( xResult == eAzureIoTSuccess );

    // End of top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid sensor data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnitSensorTelemetry( sensor_name_t name, UNIT_iot_status_t unit_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    double sensor_avg = 0.0;
    double sensor_max = 0.0;
    double sensor_min = 0.0;

    // Get the desired sensor avg,max,min
    switch(name){
        // Skid sensors
        case UNIT_VACUUM_SENSOR:
            sensor_avg = unit_data.vacuum_sensor.avg;
            sensor_max = unit_data.vacuum_sensor.max;
            sensor_min = unit_data.vacuum_sensor.min;
        break;
        case UNIT_AMBIENT_HUMIDITY:
            sensor_avg = unit_data.ambient_humidity.avg;
            sensor_max = unit_data.ambient_humidity.max;
            sensor_min = unit_data.ambient_humidity.min;
        break;
        case UNIT_AMBIENT_TEMPERATURE:
            sensor_avg = unit_data.ambient_temperature.avg;
            sensor_max = unit_data.ambient_temperature.max;
            sensor_min = unit_data.ambient_temperature.min;
        break;
        default:
        break;
    }

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_AVG, lengthof( sampleazureiotTELEMETRY_AVG ), sensor_avg, 3);
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MAX, lengthof( sampleazureiotTELEMETRY_MAX ), sensor_max, 3);
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MIN, lengthof( sampleazureiotTELEMETRY_MIN ), sensor_min, 3);
    configASSERT( xResult == eAzureIoTSuccess );

    // End of top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Unit sensor data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnitHeatersTelemetry( uint8_t index, UNIT_iot_status_t unit_data, uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    double sensor_avg = unit_data.heater_info[index].sensor_info.avg;
    double sensor_max = unit_data.heater_info[index].sensor_info.max;
    double sensor_min = unit_data.heater_info[index].sensor_info.avg;
    component_status_t status = unit_data.heater_info[index].status;

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_STATUS, lengthof( sampleazureiotTELEMETRY_STATUS ),
                                                                ( uint8_t * )component_status_stringified[status], strlen(component_status_stringified[status]));
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_AVG, lengthof( sampleazureiotTELEMETRY_AVG ), sensor_avg, 3);
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MAX, lengthof( sampleazureiotTELEMETRY_MAX ), sensor_max, 3);
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MIN, lengthof( sampleazureiotTELEMETRY_MIN ), sensor_min, 3);
    configASSERT( xResult == eAzureIoTSuccess );

    // End of top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid sensor data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateSkidTelemetry( uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    // Read the current sensor data
    SKID_iot_status_t skid_data = get_skid_status();
    // Other non-nested stuff
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_SKID_STATE, lengthof( sampleazureiotTELEMETRY_SKID_STATE ),
                                                                ( uint8_t * )sequence_state_stringified[skid_data.skid_state], strlen(sequence_state_stringified[skid_data.skid_state]));
    configASSERT( xResult == eAzureIoTSuccess );

    // Nested sensors
    // o2
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_O2, lengthof( sampleazureiotTELEMETRY_OBJECT_O2 ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidSensorTelemetry( SKID_O2, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // mass flow
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_MASSFLOW, lengthof( sampleazureiotTELEMETRY_OBJECT_MASSFLOW ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidSensorTelemetry( SKID_MASS_FLOW, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // co2
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_CO2, lengthof( sampleazureiotTELEMETRY_OBJECT_CO2 ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidSensorTelemetry( SKID_CO2, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // tank pressure
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_TANK_PRESSURE, lengthof( sampleazureiotTELEMETRY_OBJECT_TANK_PRESSURE ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidSensorTelemetry( SKID_TANK_PRESSURE, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // proptional valve sensor
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_PROPOTIONAL_VALVE_SENSOR, lengthof( sampleazureiotTELEMETRY_OBJECT_PROPOTIONAL_VALVE_SENSOR ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidSensorTelemetry( SKID_PROPOTIONAL_VALVE_SENSOR, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // temperature
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_TEMPERATURE, lengthof( sampleazureiotTELEMETRY_OBJECT_TEMPERATURE ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidSensorTelemetry( SKID_TEMPERATURE, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // humidity
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_HUMIDITY, lengthof( sampleazureiotTELEMETRY_OBJECT_HUMIDITY ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidSensorTelemetry( SKID_HUMIDITY, skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // Skid status
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_SKID_STATUS, lengthof( sampleazureiotTELEMETRY_OBJECT_SKID_STATUS ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidStatusTelemetry( skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // Skid component status
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_COMPONENT_STATUS, lengthof( sampleazureiotTELEMETRY_OBJECT_COMPONENT_STATUS ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidComponentStatusTelemetry( skid_data, ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // End of top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Skid data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnit1Telemetry( uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    // Read the current sensor data
    UNIT_iot_status_t unit_data = get_unit_status();
    // Other non-nested stuff
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_UNIT_STATE, lengthof( sampleazureiotTELEMETRY_UNIT_STATE ),
                                                                ( uint8_t * )sequence_state_stringified[unit_data.unit_state], strlen(sequence_state_stringified[unit_data.unit_state]));
    configASSERT( xResult == eAzureIoTSuccess );

    // heaters
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_HEATERS, lengthof( sampleazureiotTELEMETRY_OBJECT_HEATERS ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Begin heaters object
        xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess );

        for(uint8_t in = 0;in<NUMBER_OF_HEATERS;++in){
            const char* catridge = catridge_index_stringified[in];
            xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) catridge, strlen( catridge ) );
            configASSERT( xResult == eAzureIoTSuccess );

            // status, avg, max, min
            memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
            lBytesWritten = prvCreateUnitHeatersTelemetry( in, unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

            xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
            configASSERT( xResult == eAzureIoTSuccess );
        }

        // End heaters object
        xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // Unit status
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_UNIT_STATUS, lengthof( sampleazureiotTELEMETRY_OBJECT_UNIT_STATUS ) );
        configASSERT( xResult == eAzureIoTSuccess );

        memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
        lBytesWritten = prvCreateUnitStatusTelemetry( unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // component status
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_COMPONENT_STATUS, lengthof( sampleazureiotTELEMETRY_OBJECT_COMPONENT_STATUS ) );
        configASSERT( xResult == eAzureIoTSuccess );

        memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
        lBytesWritten = prvCreateUnitComponentStatusTelemetry( unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // Nested sensors
    // vacuum sensor
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_VACUUM_SENSOR, lengthof( sampleazureiotTELEMETRY_OBJECT_VACUUM_SENSOR ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
        lBytesWritten = prvCreateUnitSensorTelemetry( UNIT_VACUUM_SENSOR, unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // ambient humidity
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_AMBIENT_HUMIDITY, lengthof( sampleazureiotTELEMETRY_OBJECT_AMBIENT_HUMIDITY ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
        lBytesWritten = prvCreateUnitSensorTelemetry( UNIT_AMBIENT_HUMIDITY, unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // ambient temperature
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_AMBIENT_TEMPERATURE, lengthof( sampleazureiotTELEMETRY_OBJECT_AMBIENT_TEMPERATURE ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // Sensor: avg, max, min
        memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
        lBytesWritten = prvCreateUnitSensorTelemetry( UNIT_AMBIENT_TEMPERATURE, unit_data, ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Unit1 data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnitsTelemetry( uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    // Unit-1
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_UNIT1, lengthof( sampleazureiotTELEMETRY_OBJECT_UNIT1 ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // All unit1 sensors
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateUnit1Telemetry(ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // Unit-2, Unit-3 later

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Units data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

#if 0
uint32_t prvCreateExternalTelemetry( uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    // @todo-REVISIT - For now just hardcoding the stuff for simulation
    temperature_ext += 0.5;

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_TEMPERATURE, lengthof( sampleazureiotTELEMETRY_TEMPERATURE ), temperature_ext, 3 );
    configASSERT( xResult == eAzureIoTSuccess );

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "External data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}
#endif

/**
 * @brief Create and fill the telemetry data.
 *        Note: For now just filling the simulated ones
 */
uint32_t prvCreateTelemetry( uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    // Note: Later optimize the code separating common code etc!!

    // Skid
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_SKID, lengthof( sampleazureiotTELEMETRY_OBJECT_SKID ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // All skid sensors
        memset(ucScratchTempBuffer, '\0', sizeof(ucScratchTempBuffer));
        lBytesWritten = prvCreateSkidTelemetry(ucScratchTempBuffer, sizeof(ucScratchTempBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }
    // Endof Skid

    // Units
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_UNITS, lengthof( sampleazureiotTELEMETRY_OBJECT_UNITS ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // All Units
        memset(ucScratchTempBuffer, '\0', sizeof(ucScratchTempBuffer));
        lBytesWritten = prvCreateUnitsTelemetry(ucScratchTempBuffer, sizeof(ucScratchTempBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }
    // Endof Units

    #if 0 // External
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_EXTERNAL, lengthof( sampleazureiotTELEMETRY_OBJECT_EXTERNAL ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // All external sensors
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateExternalTelemetry(ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }
    // Endof External

    // @todo: Remove this and have nested stuff!!!
    // Temp Status
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_STATUS, lengthof( sampleazureiotTELEMETRY_STATUS ), "Running", 7 );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_ERROR_STATUS, lengthof( sampleazureiotTELEMETRY_ERROR_STATUS ), "No Error", 8 );
    configASSERT( xResult == eAzureIoTSuccess );
    #endif // Endof Temp status

    // End top object
    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the telemetry JSON" ) );
        return 0;
    }

    // LogInfo( ( "Telemetry message to be sent %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}
/*-----------------------------------------------------------*/

/**
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub.
 */
static void prvAzureDemoTask( void * pvParameters )
{
    int lPublishCount = 0;

    // Skytree specific
    // int error_state_count = 0;
    // char error_state[30];
    // memset(error_state, '\0', sizeof(error_state));
    // strncpy(error_state, "no-error", strlen("no-error"));

    uint32_t ulScratchBufferLength = 0U;
    const int lMaxPublishCount = 15;
    NetworkCredentials_t xNetworkCredentials = { 0 };
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTResult_t xResult;
    uint32_t ulStatus;
    AzureIoTHubClientOptions_t xHubOptions = { 0 };
    AzureIoTMessageProperties_t xPropertyBag;
    bool xSessionPresent;

    #ifdef democonfigENABLE_DPS_SAMPLE
        uint8_t * pucIotHubHostname = NULL;
        uint8_t * pucIotHubDeviceId = NULL;
        uint32_t pulIothubHostnameLength = 0;
        uint32_t pulIothubDeviceIdLength = 0;
    #else
        uint8_t * pucIotHubHostname = ( uint8_t * ) democonfigHOSTNAME;
        uint8_t * pucIotHubDeviceId = ( uint8_t * ) democonfigDEVICE_ID;
        uint32_t pulIothubHostnameLength = sizeof( democonfigHOSTNAME ) - 1;
        uint32_t pulIothubDeviceIdLength = sizeof( democonfigDEVICE_ID ) - 1;
    #endif /* democonfigENABLE_DPS_SAMPLE */

    ( void ) pvParameters;

    /* Idle for some time so that telemetry is populated first time upon boot. */
    LogInfo( ( "On boot-up: Keeping Connection Idle for %d seconds...\r\n\r\n", sampleazureiotDELAY_AT_START / 1000 ) );
    vTaskDelay( sampleazureiotDELAY_AT_START );

    /* Initialize Azure IoT Middleware.  */
    configASSERT( AzureIoT_Init() == eAzureIoTSuccess );

    ulStatus = prvSetupNetworkCredentials( &xNetworkCredentials );
    configASSERT( ulStatus == 0 );

    #ifdef democonfigENABLE_DPS_SAMPLE
        /* Run DPS.  */
        if( ( ulStatus = prvIoTHubInfoGet( &xNetworkCredentials, &pucIotHubHostname,
                                           &pulIothubHostnameLength, &pucIotHubDeviceId,
                                           &pulIothubDeviceIdLength ) ) != 0 )
        {
            LogError( ( "Failed on sample_dps_entry!: error code = 0x%08x\r\n", ulStatus ) );
            return;
        }
    #endif /* democonfigENABLE_DPS_SAMPLE */

    xNetworkContext.pParams = &xTlsTransportParams;

    for( ; ; )
    {
        if( xAzureSample_IsConnectedToInternet() )
        {
            /* Attempt to establish TLS session with IoT Hub. If connection fails,
             * retry after a timeout. Timeout value will be exponentially increased
             * until  the maximum number of attempts are reached or the maximum timeout
             * value is reached. The function returns a failure status if the TCP
             * connection cannot be established to the IoT Hub after the configured
             * number of attempts. */
            ulStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname,
                                                             democonfigIOTHUB_PORT,
                                                             &xNetworkCredentials, &xNetworkContext );
            configASSERT( ulStatus == 0 );

            /* Fill in Transport Interface send and receive function pointers. */
            xTransport.pxNetworkContext = &xNetworkContext;
            xTransport.xSend = TLS_Socket_Send;
            xTransport.xRecv = TLS_Socket_Recv;

            /* Init IoT Hub option */
            xResult = AzureIoTHubClient_OptionsInit( &xHubOptions );
            configASSERT( xResult == eAzureIoTSuccess );

            xHubOptions.pucModuleID = ( const uint8_t * ) democonfigMODULE_ID;
            xHubOptions.ulModuleIDLength = sizeof( democonfigMODULE_ID ) - 1;

            xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                              pucIotHubHostname, pulIothubHostnameLength,
                                              pucIotHubDeviceId, pulIothubDeviceIdLength,
                                              &xHubOptions,
                                              ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                              ullGetUnixTime,
                                              &xTransport );
            configASSERT( xResult == eAzureIoTSuccess );

            #ifdef democonfigDEVICE_SYMMETRIC_KEY
                xResult = AzureIoTHubClient_SetSymmetricKey( &xAzureIoTHubClient,
                                                             ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                             sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                             Crypto_HMAC );
                configASSERT( xResult == eAzureIoTSuccess );
            #endif /* democonfigDEVICE_SYMMETRIC_KEY */

            /* Sends an MQTT Connect packet over the already established TLS connection,
             * and waits for connection acknowledgment (CONNACK) packet. */
            LogInfo( ( "Creating an MQTT connection to %s.\r\n", pucIotHubHostname ) );

            xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                                 false, &xSessionPresent,
                                                 sampleazureiotCONNACK_RECV_TIMEOUT_MS );
            configASSERT( xResult == eAzureIoTSuccess );

            xResult = AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xAzureIoTHubClient, prvHandleCloudMessage,
                                                                       &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
            configASSERT( xResult == eAzureIoTSuccess );

            xResult = AzureIoTHubClient_SubscribeCommand( &xAzureIoTHubClient, prvHandleCommand,
                                                          &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
            configASSERT( xResult == eAzureIoTSuccess );

            xResult = AzureIoTHubClient_SubscribeProperties( &xAzureIoTHubClient, prvHandlePropertiesMessage,
                                                             &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
            configASSERT( xResult == eAzureIoTSuccess );

            /* Get property document after initial connection */
            xResult = AzureIoTHubClient_RequestPropertiesAsync( &xAzureIoTHubClient );
            configASSERT( xResult == eAzureIoTSuccess );

            /* Create a bag of properties for the telemetry */
            xResult = AzureIoTMessage_PropertiesInit( &xPropertyBag, ucPropertyBuffer, 0, sizeof( ucPropertyBuffer ) );
            configASSERT( xResult == eAzureIoTSuccess );

            /* Sending a default property (Content-Type). */
            xResult = AzureIoTMessage_PropertiesAppend( &xPropertyBag,
                                                        ( uint8_t * ) AZ_IOT_MESSAGE_PROPERTIES_CONTENT_TYPE, sizeof( AZ_IOT_MESSAGE_PROPERTIES_CONTENT_TYPE ) - 1,
                                                        ( uint8_t * ) sampleazureiotMESSAGE_CONTENT_TYPE, sizeof( sampleazureiotMESSAGE_CONTENT_TYPE ) - 1 );
            configASSERT( xResult == eAzureIoTSuccess );

            /* Sending a default property (Content-Encoding). */
            xResult = AzureIoTMessage_PropertiesAppend( &xPropertyBag,
                                                        ( uint8_t * ) AZ_IOT_MESSAGE_PROPERTIES_CONTENT_ENCODING, sizeof( AZ_IOT_MESSAGE_PROPERTIES_CONTENT_ENCODING ) - 1,
                                                        ( uint8_t * ) sampleazureiotMESSAGE_CONTENT_ENCODING, sizeof( sampleazureiotMESSAGE_CONTENT_ENCODING ) - 1 );
            configASSERT( xResult == eAzureIoTSuccess );

            /* How to send an user-defined custom property. */
            xResult = AzureIoTMessage_PropertiesAppend( &xPropertyBag, ( uint8_t * ) "name", sizeof( "name" ) - 1,
                                                        ( uint8_t * ) "value", sizeof( "value" ) - 1 );
            configASSERT( xResult == eAzureIoTSuccess );

            /* Publish messages with QoS1, send and process Keep alive messages. */
            for( lPublishCount = 0;
                 lPublishCount < lMaxPublishCount && xAzureSample_IsConnectedToInternet();
                 lPublishCount++ )
            {
                memset(ucScratchBuffer, '\0', sizeof (ucScratchBuffer) );
                // Create the json telemetry payload
                ulScratchBufferLength = prvCreateTelemetry( ucScratchBuffer, sizeof( ucScratchBuffer ) );
                LogInfo( ( "ucScratchBuffer = %s and ulScratchBufferLength =%d\r\n", ucScratchBuffer, ulScratchBufferLength ) );

                xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                           ucScratchBuffer, ulScratchBufferLength,
                                                           &xPropertyBag, eAzureIoTHubMessageQoS1, NULL );
                configASSERT( xResult == eAzureIoTSuccess );

                LogInfo( ( "Attempt to receive publish message from IoT Hub.\r\n" ) );
                xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                         sampleazureiotPROCESS_LOOP_TIMEOUT_MS );
                configASSERT( xResult == eAzureIoTSuccess );

                if( lPublishCount % 2 == 0 )
                {
                    // @todo-REVISIT: In actual code, we would like to send reported properties only when
                    //                there is any change in state or error-state.
                    //                Proposed states & error-states:
                    //                To be added.....

                    // if (error_state_count++ >= 20) {
                    //     memset(error_state, '\0', sizeof(error_state));
                    //     strncpy(error_state, "HardwareComponetErrors", strlen("HardwareComponetErrors"));
                    // }
                    // LogInfo( ( "error_state = %s \r\n", error_state ) );
                    // ulScratchBufferLength = snprintf( ( char * ) ucScratchBuffer, sizeof( ucScratchBuffer ),
                    //                               sampleazureiotPROPERTY, lPublishCount / 2 + 1, error_state );

                    /* Send reported property every other cycle */
                    ulScratchBufferLength = snprintf( ( char * ) ucScratchBuffer, sizeof( ucScratchBuffer ),
                                                      sampleazureiotPROPERTY, lPublishCount / 2 + 1 );
                    LogInfo( ( "Attempt to send reported properties from IoT Hub.\r\n" ) );
                    xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient,
                                                                        ucScratchBuffer, ulScratchBufferLength,
                                                                        NULL );
                    configASSERT( xResult == eAzureIoTSuccess );
                }

                /* Leave Connection Idle for some time. */
                LogInfo( ( "Keeping Connection Idle for %d seconds...\r\n\r\n", sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS / 1000 ) );
                vTaskDelay( sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS );
            }

            if( xAzureSample_IsConnectedToInternet() )
            {
                xResult = AzureIoTHubClient_UnsubscribeProperties( &xAzureIoTHubClient );
                configASSERT( xResult == eAzureIoTSuccess );

                xResult = AzureIoTHubClient_UnsubscribeCommand( &xAzureIoTHubClient );
                configASSERT( xResult == eAzureIoTSuccess );

                xResult = AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( &xAzureIoTHubClient );
                configASSERT( xResult == eAzureIoTSuccess );

                /* Send an MQTT Disconnect packet over the already connected TLS over
                 * TCP connection. There is no corresponding response for the disconnect
                 * packet. After sending disconnect, client must close the network
                 * connection. */
                xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
                configASSERT( xResult == eAzureIoTSuccess );
            }

            /* Close the network connection.  */
            TLS_Socket_Disconnect( &xNetworkContext );

            /* Wait for some time between two iterations to ensure that we do not
             * bombard the IoT Hub. */
            // LogInfo( ( "Demo completed successfully.\r\n" ) );
        }

        LogInfo( ( "Short delay (%d seconds) before starting the next iteration.... \r\n\r\n", sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS / 1000 ) );
        vTaskDelay( sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS );
    }
}
/*-----------------------------------------------------------*/

#ifdef democonfigENABLE_DPS_SAMPLE

/**
 * @brief Get IoT Hub endpoint and device Id info, when Provisioning service is used.
 *   This function will block for Provisioning service for result or return failure.
 */
    static uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                      uint8_t ** ppucIothubHostname,
                                      uint32_t * pulIothubHostnameLength,
                                      uint8_t ** ppucIothubDeviceId,
                                      uint32_t * pulIothubDeviceIdLength )
    {
        NetworkContext_t xNetworkContext = { 0 };
        TlsTransportParams_t xTlsTransportParams = { 0 };
        AzureIoTResult_t xResult;
        AzureIoTTransportInterface_t xTransport;
        uint32_t ucSamplepIothubHostnameLength = sizeof( ucSampleIotHubHostname );
        uint32_t ucSamplepIothubDeviceIdLength = sizeof( ucSampleIotHubDeviceId );
        uint32_t ulStatus;

        /* Set the pParams member of the network context with desired transport. */
        xNetworkContext.pParams = &xTlsTransportParams;

        ulStatus = prvConnectToServerWithBackoffRetries( democonfigENDPOINT, democonfigIOTHUB_PORT,
                                                         pXNetworkCredentials, &xNetworkContext );
        configASSERT( ulStatus == 0 );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_Socket_Send;
        xTransport.xRecv = TLS_Socket_Recv;

        #ifdef democonfigUSE_HSM

            /* Redefine the democonfigREGISTRATION_ID macro using registration ID
             * generated dynamically using the HSM */

            /* We use a pointer instead of a buffer so that the getRegistrationId
             * function can allocate the necessary memory depending on the HSM */
            char * registration_id = NULL;
            ulStatus = getRegistrationId( &registration_id );
            configASSERT( ulStatus == 0 );
#undef democonfigREGISTRATION_ID
        #define democonfigREGISTRATION_ID    registration_id
        #endif

        xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
                                                   ( const uint8_t * ) democonfigENDPOINT,
                                                   sizeof( democonfigENDPOINT ) - 1,
                                                   ( const uint8_t * ) democonfigID_SCOPE,
                                                   sizeof( democonfigID_SCOPE ) - 1,
                                                   ( const uint8_t * ) democonfigREGISTRATION_ID,
                                                   #ifdef democonfigUSE_HSM
                                                       strlen( democonfigREGISTRATION_ID ),
                                                   #else
                                                       sizeof( democonfigREGISTRATION_ID ) - 1,
                                                   #endif
                                                   NULL, ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                                   ullGetUnixTime,
                                                   &xTransport );
        configASSERT( xResult == eAzureIoTSuccess );

        #ifdef democonfigDEVICE_SYMMETRIC_KEY
            xResult = AzureIoTProvisioningClient_SetSymmetricKey( &xAzureIoTProvisioningClient,
                                                                  ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                                  sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                                  Crypto_HMAC );
            configASSERT( xResult == eAzureIoTSuccess );
        #endif /* democonfigDEVICE_SYMMETRIC_KEY */

        do
        {
            xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                           sampleazureiotProvisioning_Registration_TIMEOUT_MS );
        } while( xResult == eAzureIoTErrorPending );

        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTProvisioningClient_GetDeviceAndHub( &xAzureIoTProvisioningClient,
                                                              ucSampleIotHubHostname, &ucSamplepIothubHostnameLength,
                                                              ucSampleIotHubDeviceId, &ucSamplepIothubDeviceIdLength );
        configASSERT( xResult == eAzureIoTSuccess );

        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

        /* Close the network connection.  */
        TLS_Socket_Disconnect( &xNetworkContext );

        *ppucIothubHostname = ucSampleIotHubHostname;
        *pulIothubHostnameLength = ucSamplepIothubHostnameLength;
        *ppucIothubDeviceId = ucSampleIotHubDeviceId;
        *pulIothubDeviceIdLength = ucSamplepIothubDeviceIdLength;

        return 0;
    }

#endif /* democonfigENABLE_DPS_SAMPLE */
/*-----------------------------------------------------------*/

/**
 * @brief Connect to server with backoff retries.
 */
static uint32_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                      uint32_t port,
                                                      NetworkCredentials_t * pxNetworkCredentials,
                                                      NetworkContext_t * pxNetworkContext )
{
    TlsTransportStatus_t xNetworkStatus;
    BackoffAlgorithmStatus_t xBackoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t xReconnectParams;
    uint16_t usNextRetryBackOff = 0U;

    /* Initialize reconnect attempts and interval. */
    BackoffAlgorithm_InitializeParams( &xReconnectParams,
                                       sampleazureiotRETRY_BACKOFF_BASE_MS,
                                       sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS,
                                       sampleazureiotRETRY_MAX_ATTEMPTS );

    /* Attempt to connect to IoT Hub. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached.
     */
    do
    {
        LogInfo( ( "Creating a TLS connection to %s:%lu.\r\n", pcHostName, port ) );
        /* Attempt to create a mutually authenticated TLS connection. */
        xNetworkStatus = TLS_Socket_Connect( pxNetworkContext,
                                             pcHostName, port,
                                             pxNetworkCredentials,
                                             sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                             sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS );

        if( xNetworkStatus != eTLSTransportSuccess )
        {
            /* Generate a random number and calculate backoff value (in milliseconds) for
             * the next connection retry.
             * Note: It is recommended to seed the random number generator with a device-specific
             * entropy source so that possibility of multiple devices retrying failed network operations
             * at similar intervals can be avoided. */
            xBackoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &xReconnectParams, configRAND32(), &usNextRetryBackOff );

            if( xBackoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                LogError( ( "Connection to the IoT Hub failed, all attempts exhausted." ) );
            }
            else if( xBackoffAlgStatus == BackoffAlgorithmSuccess )
            {
                LogWarn( ( "Connection to the IoT Hub failed [%d]. "
                           "Retrying connection with backoff and jitter [%d]ms.",
                           xNetworkStatus, usNextRetryBackOff ) );
                vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
            }
        }
    } while( ( xNetworkStatus != eTLSTransportSuccess ) && ( xBackoffAlgStatus == BackoffAlgorithmSuccess ) );

    return xNetworkStatus == eTLSTransportSuccess ? 0 : 1;
}
/*-----------------------------------------------------------*/

/*
 * @brief Create the task that demonstrates the AzureIoTHub demo
 */
void vStartDemoTask( void )
{
    /* This example uses a single application task, which in turn is used to
     * connect, subscribe, publish, unsubscribe and disconnect from the IoT Hub */
    xTaskCreate( prvAzureDemoTask,         /* Function that implements the task. */
                 "AzureDemoTask",          /* Text name for the task - only used for debugging. */
                 democonfigDEMO_STACKSIZE, /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                     /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY,         /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );                   /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/
