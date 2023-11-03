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
#define sampleazureiotTELEMETRY_OBJECT_EXTERNAL                   ( "environment" )
#define sampleazureiotTELEMETRY_OBJECT_STATE                      ( "state" )
#define sampleazureiotTELEMETRY_OBJECT_ERROR_STATE                ( "error-state" )
// Secondary
#define sampleazureiotTELEMETRY_OBJECT_UNIT1                      ( "unit1" )
#define sampleazureiotTELEMETRY_OBJECT_UNIT2                      ( "unit2" )
#define sampleazureiotTELEMETRY_OBJECT_UNIT3                      ( "unit3" )
#define sampleazureiotTELEMETRY_OBJECT_PRESSURE                   ( "pressure" )
#define sampleazureiotTELEMETRY_OBJECT_TEMPERATURE                ( "temperature" )

// Telemetry
#define sampleazureiotTELEMETRY_PRESSURE1                         ( "pressure1" )
#define sampleazureiotTELEMETRY_PRESSURE2                         ( "pressure2" )
#define sampleazureiotTELEMETRY_PRESSURE3                         ( "pressure3" )
#define sampleazureiotTELEMETRY_MASS_FLOW                         ( "massflow" )
#define sampleazureiotTELEMETRY_CO2                               ( "co2" )
#define sampleazureiotTELEMETRY_TEMPERATURE                       ( "temperature" )
#define sampleazureiotTELEMETRY_CARTRIDGE11                       ( "cartridge11" )
#define sampleazureiotTELEMETRY_CARTRIDGE12                       ( "cartridge12" )
#define sampleazureiotTELEMETRY_CARTRIDGE21                       ( "cartridge21" )
#define sampleazureiotTELEMETRY_CARTRIDGE22                       ( "cartridge22" )
#define sampleazureiotTELEMETRY_CARTRIDGE31                       ( "cartridge31" )
#define sampleazureiotTELEMETRY_CARTRIDGE32                       ( "cartridge32" )
#define sampleazureiotTELEMETRY_CARTRIDGE41                       ( "cartridge41" )
#define sampleazureiotTELEMETRY_CARTRIDGE42                       ( "cartridge42" )
#define sampleazureiotTELEMETRY_VACUUM                            ( "vacuum" )
#define sampleazureiotTELEMETRY_VIBRATION                         ( "vibration" )
#define sampleazureiotTELEMETRY_HUMIDITY                          ( "humidity" )
#define sampleazureiotTELEMETRY_PWM_DUTY_CYCLE                    ( "pwm_duty_cycle" )

#define sampleazureiotTELEMETRY_STATUS                     ( "status" )
#define sampleazureiotTELEMETRY_ERROR_STATUS               ( "errorstatus" )

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
// #define sampleazureiotPROPERTY                                "{ \"PropertyIterationForCurrentConnection\": \"%d\", \"state\": \"provisioned\", \"error-state\": \"%s\" }"

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

#define SCRATCH_BUFFER_LENGTH 1024
#define MINI_SCRATCH_BUFFER_LENGTH 128
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


// @todo -REVISIT:
// For now have the similated values

// Skid
int32_t mass_flow = 10;
uint32_t co2 = 4;
double temperature_skid = 10.206;
int32_t pressure1 = 0;
int32_t pressure2 = 7;
int32_t pressure3 = 10;

// Units
// Unit-1
int32_t humidity = 76;
double cartridge11 = 10.26;
double cartridge12 = 11.36;
double cartridge21 = 12.46;
double cartridge22 = 13.56;
double cartridge31 = 14.66;
double cartridge32 = 15.76;
double cartridge41 = 16.86;
double cartridge42 = 17.96;
double vacuum = 0.1;
double vibration = 1.1;
double pwm_duty_cycle = 5.7;
// Unit-2 & Unit-3 later

// External
double temperature_ext = 14.712;

// Scratch buffers, improve these later
static uint8_t ucScratchTempBuffer[ SCRATCH_BUFFER_LENGTH ];
static uint8_t ucScratchTempHalfBuffer[ SCRATCH_BUFFER_LENGTH / 2 ];
static uint8_t ucScratchTempHalfBuffer2[ SCRATCH_BUFFER_LENGTH / 2 ];

uint32_t prvCreateSkidPressureTelemetry( uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, pucTelemetryData, ulTelemetryDataLength );
    configASSERT( xResult == eAzureIoTSuccess );

    // Begin top object
    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    // @todo - REVISIT: For now just hardcoding the stuff for simulation
    if (pressure1 > 16) pressure1 = 0;
    else pressure1++;
    if (pressure2 > 16) pressure2 = 0;
    else pressure2++;
    if (pressure3 > 16) pressure3 = 0;
    else pressure3++;

    xResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_PRESSURE1, lengthof( sampleazureiotTELEMETRY_PRESSURE1 ), pressure1);
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_PRESSURE2, lengthof( sampleazureiotTELEMETRY_PRESSURE2 ), pressure2);
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_PRESSURE3, lengthof( sampleazureiotTELEMETRY_PRESSURE3 ), pressure3);
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

    // LogInfo( ( "Skid pressure data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

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
    
    // @todo-REVISIT - For now just hardcoding the stuff for simulation
    if (mass_flow > 50) mass_flow = 0;
    else mass_flow++;
    temperature_skid += 0.5;
    co2 += 2;
    if (co2 >= 100) {
        co2 -= 100;
    }

    // Other non-nested sensors
    xResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_MASS_FLOW, lengthof( sampleazureiotTELEMETRY_MASS_FLOW ), mass_flow);
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CO2, lengthof( sampleazureiotTELEMETRY_CO2 ), co2);
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_TEMPERATURE, lengthof( sampleazureiotTELEMETRY_TEMPERATURE ), temperature_skid, 3 );
    configASSERT( xResult == eAzureIoTSuccess );

    // Pressure sensor
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_PRESSURE, lengthof( sampleazureiotTELEMETRY_OBJECT_PRESSURE ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // All pressure sensors
        memset(ucScratchTempHalfBuffer, '\0', sizeof(ucScratchTempHalfBuffer));
        lBytesWritten = prvCreateSkidPressureTelemetry(ucScratchTempHalfBuffer, sizeof(ucScratchTempHalfBuffer) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer, lBytesWritten );
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

    // LogInfo( ( "Skid data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

    return ( uint32_t ) lBytesWritten;
}

uint32_t prvCreateUnit1TemperatureTelemetry( uint8_t * pucTelemetryData, uint32_t ulTelemetryDataLength )
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
    cartridge11 += 0.4;
    cartridge12 += 1.7;
    cartridge21 += 0.8;
    cartridge22 += 0.9;
    cartridge31 += 2.4;
    cartridge32 += 6.9;
    cartridge41 += 0.2;
    cartridge42 += 3.4;

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CARTRIDGE11, lengthof( sampleazureiotTELEMETRY_CARTRIDGE11 ), cartridge11, 3 );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CARTRIDGE12, lengthof( sampleazureiotTELEMETRY_CARTRIDGE12 ), cartridge12, 3 );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CARTRIDGE21, lengthof( sampleazureiotTELEMETRY_CARTRIDGE21 ), cartridge21, 3 );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CARTRIDGE22, lengthof( sampleazureiotTELEMETRY_CARTRIDGE22 ), cartridge22, 3 );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CARTRIDGE31, lengthof( sampleazureiotTELEMETRY_CARTRIDGE31 ), cartridge31, 3 );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CARTRIDGE32, lengthof( sampleazureiotTELEMETRY_CARTRIDGE32 ), cartridge32, 3 );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CARTRIDGE41, lengthof( sampleazureiotTELEMETRY_CARTRIDGE41 ), cartridge41, 3 );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_CARTRIDGE42, lengthof( sampleazureiotTELEMETRY_CARTRIDGE42 ), cartridge42, 3 );
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

    // LogInfo( ( "Unit1 temperature data %.*s\r\n", lBytesWritten, pucTelemetryData ) );

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

    // @todo-REVISIT - For now just hardcoding the stuff for simulation
    humidity += 5;
    if (humidity > 100) {
        humidity -= 100;
    }
    vacuum += 0.1;
    if (vacuum > 1.0) {
        vacuum -= 1.0;
    }

    // Temperature
    {
        xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_TEMPERATURE, lengthof( sampleazureiotTELEMETRY_OBJECT_TEMPERATURE ) );
        configASSERT( xResult == eAzureIoTSuccess );

        // All unit1 sensors
        memset(ucScratchTempHalfBuffer2, '\0', sizeof(ucScratchTempHalfBuffer2));
        lBytesWritten = prvCreateUnit1TemperatureTelemetry(ucScratchTempHalfBuffer2, sizeof(ucScratchTempHalfBuffer2) );

        xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempHalfBuffer2, lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );
    }

    // Other sensors
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_VACUUM, lengthof( sampleazureiotTELEMETRY_VACUUM ), vacuum, 3 );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_VIBRATION, lengthof( sampleazureiotTELEMETRY_VIBRATION ), vibration, 3 );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithInt32Value( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_HUMIDITY, lengthof( sampleazureiotTELEMETRY_HUMIDITY ), humidity );
    configASSERT( xResult == eAzureIoTSuccess );
    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_PWM_DUTY_CYCLE, lengthof( sampleazureiotTELEMETRY_PWM_DUTY_CYCLE ), pwm_duty_cycle, 3 );
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

    // External
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
    // Endof Temp status

    // For now state and errorState to be added in telemetry
    // Imp!!! To be added as device twin in actual PCBA code
    // // State
    // {
    //     xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_STATE, lengthof( sampleazureiotTELEMETRY_OBJECT_STATE ) );
    //     configASSERT( xResult == eAzureIoTSuccess );

    //     // All skid sensors
    //     memset(ucScratchTempBuffer, '\0', sizeof(ucScratchTempBuffer));
    //     lBytesWritten = prvCreateStateTelemetry(ucScratchTempBuffer, sizeof(ucScratchTempBuffer) );

    //     xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempBuffer, lBytesWritten );
    //     configASSERT( xResult == eAzureIoTSuccess );
    // }
    // // Endof State

    // // ErrorState
    // {
    //     xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotTELEMETRY_OBJECT_ERROR_STATE, lengthof( sampleazureiotTELEMETRY_OBJECT_ERROR_STATE ) );
    //     configASSERT( xResult == eAzureIoTSuccess );

    //     // All skid sensors
    //     memset(ucScratchTempBuffer, '\0', sizeof(ucScratchTempBuffer));
    //     lBytesWritten = prvCreateErrorStateTelemetry(ucScratchTempBuffer, sizeof(ucScratchTempBuffer) );

    //     xResult = AzureIoTJSONWriter_AppendJSONText( &xWriter, ucScratchTempBuffer, lBytesWritten );
    //     configASSERT( xResult == eAzureIoTSuccess );
    // }
    // // Endof ErrorState


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
                // We dont need the macro one anymore
                // ulScratchBufferLength = snprintf( ( char * ) ucScratchBuffer, sizeof( ucScratchBuffer ),
                //                                   sampleazureiotMESSAGE, lPublishCount );
                // LogInfo( ( "ucScratchBuffer = %s and ulScratchBufferLength = %d\r\n", ucScratchBuffer, ulScratchBufferLength ) );

                memset(ucScratchBuffer, '\0', sizeof (ucScratchBuffer) );
                // Create the json telemetry payload
                ulScratchBufferLength = prvCreateTelemetry( ucScratchBuffer, sizeof( ucScratchBuffer ) );
                LogInfo( ( "ucScratchBuffer = %s and ulScratchBufferLength =%d\r\n", ucScratchBuffer, ulScratchBufferLength ) );

                // @todo-REVISIT: ToDo: Read from Controllino
                // getTelemetryData(); once ready

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
