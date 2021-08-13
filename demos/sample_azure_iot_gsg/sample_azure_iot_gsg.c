/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "sample_gsg_device.h"

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"
#include "azure_iot_provisioning_client.h"

/* Azure JSON includes */
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
#include "transport_tls_socket.h"

/* Crypto helper header. */
#include "crypto.h"

/* Demo specific configs. */
#include "demo_config.h"
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
 * @brief Property Values
 */
#define sampleazureiotPROPERTY_SUCCESS                        ( "success" )

/**
 * @brief The payload to send to the Device Provisioning Service
 */
#define sampleazureiotPROVISIONING_PAYLOAD_MODELID            ( "modelId" )

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

#define TELEMETRY_INTERVAL_PROPERTY                           ( "telemetryInterval" )
#define LED_STATE_PROPERTY                                    ( "ledState" )
#define SET_LED_STATE_COMMAND                                 ( "setLedState" )

#define DEVICE_INFORMATION_NAME                               ( "deviceInformation" )
#define MANUFACTURER_PROPERTY_NAME                            ( "manufacturer" )
#define MODEL_PROPERTY_NAME                                   ( "model" )
#define SOFTWARE_VERSION_PROPERTY_NAME                        ( "swVersion" )
#define OS_NAME_PROPERTY_NAME                                 ( "osName" )
#define PROCESSOR_ARCHITECTURE_PROPERTY_NAME                  ( "processorArchitecture" )
#define PROCESSOR_MANUFACTURER_PROPERTY_NAME                  ( "processorManufacturer" )
#define TOTAL_STORAGE_PROPERTY_NAME                           ( "totalStorage" )
#define TOTAL_MEMORY_PROPERTY_NAME                            ( "totalMemory" )
/*-----------------------------------------------------------*/

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    TlsTransportParams_t * pParams;
};
/*-----------------------------------------------------------*/

/* Define buffer for IoT Hub info.  */
#ifdef democonfigENABLE_DPS_SAMPLE

    static uint8_t ucSampleIotHubHostname[ 128 ];
    static uint8_t ucSampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;

#endif /* democonfigENABLE_DPS_SAMPLE */

/* Scratch buffer */
static uint8_t ucScratchBuffer[ 128 ];

/* Property buffer */
static uint8_t ucPropertyPayloadBuffer[ 400 ];

/* Device properties */
static int32_t lTelemetryInterval = 5;
static bool xLedState = false;

static AzureIoTHubClient_t xAzureIoTHubClient;
/*-----------------------------------------------------------*/

/**
 * @brief Unix time.
 *
 * @return Time in milliseconds.
 */
uint64_t ullGetUnixTime( void );
/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucMQTTMessageBuffer[ democonfigNETWORK_BUFFER_SIZE ];
/*-----------------------------------------------------------*/

static void prvReportLedState()
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    /* Update reported property */
    xResult = AzureIoTJSONWriter_Init( &xWriter, ucPropertyPayloadBuffer, sizeof( ucPropertyPayloadBuffer ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) LED_STATE_PROPERTY,
                                                     sizeof( LED_STATE_PROPERTY ) - 1 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendBool( &xWriter, xLedState );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the properties confirmation JSON" ) );
        return;
    }

    xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient, ucPropertyPayloadBuffer, lBytesWritten, NULL );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "There was an error sending the reported properties: 0x%08x", xResult ) );
    }
}
/*-----------------------------------------------------------*/

static void prvReportTelemetryInterval( uint32_t ulVersion )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, ucPropertyPayloadBuffer, sizeof( ucPropertyPayloadBuffer ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xAzureIoTHubClient,
                                                                      &xWriter,
                                                                      ( uint8_t * ) TELEMETRY_INTERVAL_PROPERTY,
                                                                      sizeof( TELEMETRY_INTERVAL_PROPERTY ) - 1,
                                                                      200,
                                                                      ulVersion,
                                                                      ( uint8_t * ) sampleazureiotPROPERTY_SUCCESS,
                                                                      sizeof( sampleazureiotPROPERTY_SUCCESS ) - 1 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendInt32( &xWriter, lTelemetryInterval );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTHubClientProperties_BuilderEndResponseStatus( &xAzureIoTHubClient,
                                                                    &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the properties confirmation JSON" ) );
    }
    else
    {
        LogDebug( ( "Sending acknowledged writable property. Payload: %.*s", lBytesWritten, ucPropertyPayloadBuffer ) );
        xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient, ucPropertyPayloadBuffer, lBytesWritten, NULL );

        if( xResult != eAzureIoTSuccess )
        {
            LogError( ( "There was an error sending the reported properties: 0x%08x", xResult ) );
        }
    }
}
/*-----------------------------------------------------------*/

static void prvReportDeviceInfo()
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    /* Update reported property */
    xResult = AzureIoTJSONWriter_Init( &xWriter, ucPropertyPayloadBuffer, sizeof( ucPropertyPayloadBuffer ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTHubClientProperties_BuilderBeginComponent( &xAzureIoTHubClient, &xWriter, ( const uint8_t * ) DEVICE_INFORMATION_NAME, strlen( DEVICE_INFORMATION_NAME ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) MANUFACTURER_PROPERTY_NAME, sizeof( MANUFACTURER_PROPERTY_NAME ) - 1,
                                                                ( uint8_t * ) pcManufacturerPropertyValue, strlen( pcManufacturerPropertyValue ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) MODEL_PROPERTY_NAME, sizeof( MODEL_PROPERTY_NAME ) - 1,
                                                                ( uint8_t * ) pcModelPropertyValue, strlen( pcModelPropertyValue ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) SOFTWARE_VERSION_PROPERTY_NAME, sizeof( SOFTWARE_VERSION_PROPERTY_NAME ) - 1,
                                                                ( uint8_t * ) pcSoftwareVersionPropertyValue, strlen( pcSoftwareVersionPropertyValue ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) OS_NAME_PROPERTY_NAME, sizeof( OS_NAME_PROPERTY_NAME ) - 1,
                                                                ( uint8_t * ) pcOsNamePropertyValue, strlen( pcOsNamePropertyValue ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) PROCESSOR_ARCHITECTURE_PROPERTY_NAME, sizeof( PROCESSOR_ARCHITECTURE_PROPERTY_NAME ) - 1,
                                                                ( uint8_t * ) pcProcessorArchitecturePropertyValue, strlen( pcProcessorArchitecturePropertyValue ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, ( uint8_t * ) PROCESSOR_MANUFACTURER_PROPERTY_NAME, sizeof( PROCESSOR_MANUFACTURER_PROPERTY_NAME ) - 1,
                                                                ( uint8_t * ) pcProcessorManufacturerPropertyValue, strlen( pcProcessorManufacturerPropertyValue ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) TOTAL_STORAGE_PROPERTY_NAME, sizeof( TOTAL_STORAGE_PROPERTY_NAME ) - 1,
                                                                pxTotalStoragePropertyValue, 0 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, ( uint8_t * ) TOTAL_MEMORY_PROPERTY_NAME, sizeof( TOTAL_MEMORY_PROPERTY_NAME ) - 1,
                                                                pxTotalMemoryPropertyValue, 0 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTHubClientProperties_BuilderEndComponent( &xAzureIoTHubClient, &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

    if( lBytesWritten < 0 )
    {
        LogError( ( "Error getting the bytes written for the device properties JSON" ) );
        return;
    }

    xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient, ucPropertyPayloadBuffer, lBytesWritten, NULL );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "There was an error sending the device properties: 0x%08x", xResult ) );
    }
}

static void prvInvokeSetLedStateCommand( const void * pvMessagePayload,
                                         uint32_t ulMessageLength )
{
    xLedState = ( strncmp( "true", ( const char * ) pvMessagePayload, ulMessageLength ) == 0 );

    setLedState( xLedState );
}
/*-----------------------------------------------------------*/

/**
 * @brief Command message callback handler
 */
static void prvHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                              void * pvContext )
{
    AzureIoTHubClient_t * pxHandle = ( AzureIoTHubClient_t * ) pvContext;

    LogInfo( ( "Received direct command: %.*s", pxMessage->usCommandNameLength, pxMessage->pucCommandName ) );

    if( strncmp( SET_LED_STATE_COMMAND, ( const char * ) pxMessage->pucCommandName, strlen( SET_LED_STATE_COMMAND ) ) == 0 )
    {
        prvInvokeSetLedStateCommand( pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );

        if( AzureIoTHubClient_SendCommandResponse( pxHandle, pxMessage, 200, NULL, 0 ) != eAzureIoTSuccess )
        {
            LogInfo( ( "Error sending command response" ) );
        }

        /* Update the associated reported property */
        prvReportLedState();
    }
    else
    {
        LogInfo( ( "Received command is not for this device" ) );

        if( AzureIoTHubClient_SendCommandResponse( pxHandle, pxMessage, 404, NULL, 0 ) != eAzureIoTSuccess )
        {
            LogError( ( "Error sending command response" ) );
        }
    }
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
 * @brief Properties callback handler
 */
static AzureIoTResult_t prvProcessProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                              AzureIoTHubClientPropertyType_t xPropertyType )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONReader_t xReader;
    const uint8_t * pucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    uint32_t ulVersion;

    xResult = AzureIoTJSONReader_Init( &xReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTHubClientProperties_GetPropertiesVersion( &xAzureIoTHubClient, &xReader, pxMessage->xMessageType, &ulVersion );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "Error getting the property version" ) );
    }
    else
    {
        /* Reset JSON reader to the beginning */
        xResult = AzureIoTJSONReader_Init( &xReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
        configASSERT( xResult == eAzureIoTSuccess );

        while( ( xResult = AzureIoTHubClientProperties_GetNextComponentProperty( &xAzureIoTHubClient, &xReader,
                                                                                 pxMessage->xMessageType, xPropertyType,
                                                                                 &pucComponentName, &ulComponentNameLength ) ) == eAzureIoTSuccess )
        {
            if( ulComponentNameLength > 0 )
            {
                LogInfo( ( "Unknown component name received %s", pucComponentName ) );

                /* Unknown component name arrived (there are none for this device).
                 * We have to skip over the property and value to continue iterating */
                prvSkipPropertyAndValue( &xReader );
            }
            else if( AzureIoTJSONReader_TokenIsTextEqual( &xReader,
                                                          ( uint8_t * ) TELEMETRY_INTERVAL_PROPERTY,
                                                          sizeof( TELEMETRY_INTERVAL_PROPERTY ) - 1 ) )
            {
                xResult = AzureIoTJSONReader_NextToken( &xReader );
                configASSERT( xResult == eAzureIoTSuccess );

                /* Get telemetry interval */
                int32_t lNewTelemetryInterval,
                        xResult = AzureIoTJSONReader_GetTokenInt32( &xReader, &lNewTelemetryInterval );

                if( xResult != eAzureIoTSuccess )
                {
                    LogError( ( "Error getting the property: result 0x%08x", xResult ) );
                    break;
                }

                xResult = AzureIoTJSONReader_NextToken( &xReader );
                configASSERT( xResult == eAzureIoTSuccess );

                /* Update the property and report back */
                lTelemetryInterval = lNewTelemetryInterval;
                prvReportTelemetryInterval( ulVersion );

                LogInfo( ( "TelemetryInterval Property received: %d.", lTelemetryInterval ) );
            }
            else
            {
                LogInfo( ( "Unknown property arrived: skipping over it." ) );

                /* Unknown property arrived. We have to skip over the property and value to continue iterating. */
                prvSkipPropertyAndValue( &xReader );
            }
        }

        if( xResult != eAzureIoTErrorEndOfProperties )
        {
            LogError( ( "There was an error parsing the properties: 0x%08x", xResult ) );
        }
        else
        {
            LogInfo( ( "Successfully parsed properties" ) );
            xResult = eAzureIoTSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

/**
 * @brief Property mesage callback handler
 */
static void prvHandleProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                 void * pvContext )
{
    ( void ) pvContext;

    AzureIoTResult_t xResult;

    LogInfo( ( "Property document payload : %.*s\r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubPropertiesGetMessage:
            LogInfo( ( "Device property document GET received" ) );

            xResult = prvProcessProperties( pxMessage, eAzureIoTHubClientPropertyWritable );

            if( xResult != eAzureIoTSuccess )
            {
                LogError( ( "There was an error processing incoming properties" ) );
            }

            break;

        case eAzureIoTHubPropertiesWritablePropertyMessage:
            LogInfo( ( "Device writeable property received" ) );

            xResult = prvProcessProperties( pxMessage, eAzureIoTHubClientPropertyWritable );

            if( xResult != eAzureIoTSuccess )
            {
                LogError( ( "There was an error processing incoming properties" ) );
            }

            break;

        case eAzureIoTHubPropertiesReportedResponseMessage:
            LogInfo( ( "Device reported property response received" ) );

            break;

        default:
            LogError( ( "Unknown property message" ) );
    }
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

/**
 * @brief Connect to IoT Hub with reconnection retries.
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
        LogInfo( ( "Creating a TLS connection to %s:%u.\r\n", pcHostName, port ) );
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
                                      uint32_t * pulIothubDeviceIdLength )
    {
        NetworkContext_t xNetworkContext = { 0 };
        TlsTransportParams_t xTlsTransportParams = { 0 };
        AzureIoTResult_t xResult;
        AzureIoTJSONWriter_t xWriter;
        AzureIoTTransportInterface_t xTransport;
        uint32_t ucSamplepIothubHostnameLength = sizeof( ucSampleIotHubHostname );
        uint32_t ucSamplepIothubDeviceIdLength = sizeof( ucSampleIotHubDeviceId );
        uint32_t ulStatus;
        int32_t lBytesWritten;

        /* Set the pParams member of the network context with desired transport. */
        xNetworkContext.pParams = &xTlsTransportParams;

        ulStatus = prvConnectToServerWithBackoffRetries( democonfigENDPOINT, democonfigIOTHUB_PORT,
                                                         pXNetworkCredentials, &xNetworkContext );
        configASSERT( ulStatus == 0 );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_Socket_Send;
        xTransport.xRecv = TLS_Socket_Recv;

        xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
                                                   ( const uint8_t * ) democonfigENDPOINT,
                                                   sizeof( democonfigENDPOINT ) - 1,
                                                   ( const uint8_t * ) democonfigID_SCOPE,
                                                   sizeof( democonfigID_SCOPE ) - 1,
                                                   ( const uint8_t * ) democonfigREGISTRATION_ID,
                                                   sizeof( democonfigREGISTRATION_ID ) - 1,
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

        /* Create the DPS payload */
        xResult = AzureIoTJSONWriter_Init( &xWriter, ( uint8_t * ) &ucScratchBuffer, sizeof( ucScratchBuffer ) - 1 );
        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                    ( uint8_t * ) sampleazureiotPROVISIONING_PAYLOAD_MODELID,
                                                                    sizeof( sampleazureiotPROVISIONING_PAYLOAD_MODELID ) - 1,
                                                                    ( uint8_t * ) pcModelId, strlen( pcModelId ) );
        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
        configASSERT( xResult == eAzureIoTSuccess );

        lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );
        configASSERT( lBytesWritten > 0 );

        xResult = AzureIoTProvisioningClient_SetRegistrationPayload( &xAzureIoTProvisioningClient,
                                                                     ( const uint8_t * ) ucScratchBuffer,
                                                                     lBytesWritten );
        configASSERT( xResult == eAzureIoTSuccess );

        /* Register the device with DPS */
        do
        {
            xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                           sampleazureiotProvisioning_Registration_TIMEOUT_MS );
        } while( xResult == eAzureIoTErrorPending );

        if( xResult == eAzureIoTSuccess )
        {
            LogInfo( ( "Successfully acquired IoT Hub name and Device ID" ) );
        }
        else
        {
            LogInfo( ( "Error geting IoT Hub name and Device ID: 0x%08", xResult ) );
        }

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
/*-----------------------------------------------------------*/

#endif /* democonfigENABLE_DPS_SAMPLE */

/**
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub.
 *
 * @param[in] pvParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 */
static void prvAzureDemoTask( void * pvParameters )
{
    uint32_t ulScratchBufferLength = 0U;
    NetworkCredentials_t xNetworkCredentials = { 0 };
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTResult_t xResult;
    uint32_t ulStatus;
    AzureIoTHubClientOptions_t xHubOptions = { 0 };
    bool xSessionPresent;
    uint64_t lastTelemetryTime;

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

    /* Initialize Azure IoT Middleware. */
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
    xHubOptions.pucModelID = ( const uint8_t * ) pcModelId;
    xHubOptions.ulModelIDLength = strlen( pcModelId );

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

    xResult = AzureIoTHubClient_SubscribeCommand( &xAzureIoTHubClient, prvHandleCommand,
                                                  &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTHubClient_SubscribeProperties( &xAzureIoTHubClient, prvHandleProperties,
                                                     &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
    configASSERT( xResult == eAzureIoTSuccess );

    /* Get property document after initial connection */
    xResult = AzureIoTHubClient_GetProperties( &xAzureIoTHubClient );
    configASSERT( xResult == eAzureIoTSuccess );

    lastTelemetryTime = ullGetUnixTime();

    /* Report properties */
    prvReportLedState();
    prvReportTelemetryInterval( 0 );
    prvReportDeviceInfo();

    /* Loop forever */
    while( true )
    {
        uint64_t currentTime = ullGetUnixTime();

        if( currentTime > ( lastTelemetryTime + lTelemetryInterval ) )
        {
            /* Advance the time */
            while( currentTime > ( lastTelemetryTime + lTelemetryInterval ) )
            {
                lastTelemetryTime += lTelemetryInterval;
            }

            ulScratchBufferLength = createTelemetry( ucScratchBuffer, sizeof( ucScratchBuffer ) - 1 );

            xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                       ucScratchBuffer, ulScratchBufferLength,
                                                       NULL, eAzureIoTHubMessageQoS1, NULL );
            configASSERT( xResult == eAzureIoTSuccess );
        }

        /* :TODO: the processloop runs for 10 seconds */
        xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient, 0 );

        configASSERT( xResult == eAzureIoTSuccess );
    }
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
