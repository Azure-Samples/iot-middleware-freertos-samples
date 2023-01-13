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

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
#include "transport_tls_socket.h"

/* Crypto helper header. */
#include "azure_sample_crypto.h"

#include "azure_ca_recovery_rsa_verify.h"
#include "azure_ca_recovery_parse.h"
#include "azure_trust_bundle_storage.h"
#include "azure_iot_jws.h"

/*-----------------------------------------------------------*/

/* Compile time error for undefined configs. */
#if !defined( democonfigHOSTNAME ) && !defined( democonfigENABLE_DPS_SAMPLE )
    #error "Define the config democonfigHOSTNAME by following the instructions in file demo_config.h."
#endif

#if !defined( democonfigENDPOINT ) && defined( democonfigENABLE_DPS_SAMPLE )
    #error "Define the config dps endpoint by following the instructions in file demo_config.h."
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
#define sampleazureiotMESSAGE                                 "Hello World : %d !"

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
#define sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS           ( pdMS_TO_TICKS( 2000U ) )

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

/**
 * @brief Return value to signify recovery initiated.
 *
 */
#define sampleazureiotRECOVERY_INITIATED                      ( 0xDEAD )

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

static uint8_t ucPropertyBuffer[ 32 ];
static uint8_t ucScratchBuffer[ 128 ];
static uint8_t ucRootCABuffer[ 5000 ];
static uint32_t ulRootCABufferWrittenLength;
static uint8_t ucRootCATrustBundleVersion[ 16 ];
static uint32_t ulRootCATrustBundleVersionLength;
static uint8_t ucSignatureValidateScratchBuffer[ azureiotjwsSHA_CALCULATION_SCRATCH_SIZE ];

static uint8_t ucAzureIoTRecoveryRootKeyN[] =
{
    0x00, 0xC3, 0x87, 0x64, 0x80, 0xF7, 0x88, 0x8A, 0xB3, 0xC1, 0xE2, 0x0D, 0x3D,
    0xD0, 0xA4, 0xA9, 0x72, 0x7E, 0x9A, 0x42, 0xAE, 0x8E, 0x7F, 0x32, 0x5D, 0x2B,
    0x84, 0x20, 0x4C, 0xC7, 0x1D, 0xA9, 0x6D, 0xF6, 0xD9, 0xF2, 0xCD, 0x7A, 0x7A,
    0xA7, 0x0F, 0x53, 0x45, 0xC8, 0x40, 0x22, 0x85, 0x91, 0x16, 0x62, 0xC7, 0x29,
    0xA2, 0x43, 0xC4, 0x60, 0x60, 0x63, 0x4C, 0x42, 0x11, 0xAA, 0x02, 0x9C, 0x40,
    0x98, 0xB3, 0x71, 0x9D, 0xC3, 0xB9, 0xE3, 0x8E, 0x43, 0x97, 0xDE, 0x4C, 0x5C,
    0xBC, 0x15, 0x85, 0x7D, 0xE1, 0x5C, 0xF4, 0x9C, 0x4B, 0x12, 0xED, 0x49, 0xE9,
    0x9F, 0xD0, 0x45, 0xE3, 0xF2, 0x2A, 0x1A, 0x15, 0x59, 0x85, 0x8E, 0xCB, 0xE1,
    0x1E, 0xC9, 0xBE, 0x3E, 0x13, 0xEE, 0xEB, 0xB9, 0x5D, 0x01, 0xBE, 0x66, 0x65,
    0xE5, 0xE0, 0xAD, 0x08, 0x34, 0x0F, 0xCD, 0x7C, 0xC0, 0x48, 0x7A, 0x19, 0x5B,
    0xB1, 0xC6, 0x5A, 0x8F, 0xCA, 0x74, 0x99, 0xF6, 0x32, 0x4C, 0x8E, 0xE2, 0xB6,
    0x20, 0xE6, 0x55, 0x27, 0xE6, 0x09, 0x1E, 0xFF, 0xB8, 0x01, 0xEF, 0xA4, 0x47,
    0x10, 0x7F, 0x5F, 0x0D, 0x65, 0x40, 0xB9, 0xE7, 0xFF, 0x47, 0xD1, 0x47, 0xED,
    0x83, 0x72, 0xF4, 0x64, 0x17, 0xF4, 0x42, 0x25, 0xD8, 0x92, 0x34, 0x96, 0x7A,
    0x9D, 0x45, 0xA4, 0x0E, 0x23, 0x79, 0x6C, 0x83, 0x68, 0x77, 0xD1, 0xDB, 0x84,
    0x10, 0xB0, 0x6E, 0xCB, 0x8A, 0x27, 0x48, 0x2E, 0xA0, 0x88, 0x3D, 0xE8, 0x0C,
    0xAA, 0x8A, 0x67, 0x99, 0xD0, 0xC6, 0xE5, 0x26, 0xCF, 0xA3, 0x44, 0x99, 0x79,
    0x87, 0x76, 0x46, 0x50, 0xB6, 0x56, 0xA0, 0xB8, 0x39, 0x1F, 0x18, 0x1B, 0xD1,
    0x7B, 0xD3, 0x98, 0x73, 0x8C, 0x84, 0x75, 0xA1, 0x98, 0x57, 0x27, 0x4F, 0xD3,
    0xF6, 0x1B, 0xA8, 0xE0, 0xB6, 0xB8, 0x58, 0xC6, 0x5A, 0xD3
};
static uint8_t ucAzureIoTRecoveryKeyE[ 3 ] = { 0x01, 0x00, 0x01 };

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

/**
 * @brief Run the CA recovery protocol
 *
 * @param[in] pXNetworkCredentials  Network credential used to connect to Provisioning service
 */
    static uint32_t prvRunRecovery( NetworkCredentials_t * pXNetworkCredentials );

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
 * @return TlsTransportStatus_t The status of the final connection attempt.
 */
static TlsTransportStatus_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
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
               pxMessage->ulPayloadLength,
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
               pxMessage->ulPayloadLength,
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
 * @brief Property message callback handler
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
               pxMessage->ulPayloadLength,
               ( const char * ) pxMessage->pvMessagePayload ) );
}
/*-----------------------------------------------------------*/

/**
 * @brief Setup transport credentials.
 */
static uint32_t prvSetupNetworkCredentials( NetworkCredentials_t * pxNetworkCredentials )
{
    LogInfo( ( "Reading trust bundle from NVS\r\n" ) );
    memset( ucRootCABuffer, 0, sizeof( ucRootCABuffer ) );
    AzureIoTResult_t xResult = AzureIoTCAStorage_ReadTrustBundle( ucRootCABuffer,
                                                                  sizeof( ucRootCABuffer ),
                                                                  &ulRootCABufferWrittenLength,
                                                                  ucRootCATrustBundleVersion,
                                                                  sizeof( ucRootCATrustBundleVersion ),
                                                                  &ulRootCATrustBundleVersionLength );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "Could not read trust bundle from NVS. Please run az-nvs-cert-bundle sample to save certs to NVS\r\n" ) );
        return 1;
    }

    pxNetworkCredentials->xDisableSni = pdFALSE;
    /* Set the credentials for establishing a TLS connection. */
    pxNetworkCredentials->pucRootCa = ( const unsigned char * ) ucRootCABuffer;
    pxNetworkCredentials->xRootCaSize = ulRootCABufferWrittenLength;
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
 * @brief Setup transport credentials.
 */
static uint32_t prvSetupRecoveryNetworkCredentials( NetworkCredentials_t * pxNetworkCredentials )
{
    /* Don't set CA cert since we ignore CA validation on recovery */
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
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub.
 */
static void prvAzureDemoTask( void * pvParameters )
{
    int lPublishCount = 0;
    uint32_t ulScratchBufferLength = 0U;
    const int lMaxPublishCount = 5;
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
            if( ulStatus == sampleazureiotRECOVERY_INITIATED )
            {
                memset( &xNetworkCredentials, 0, sizeof( xNetworkCredentials ) );
                ulStatus = prvSetupRecoveryNetworkCredentials( &xNetworkCredentials );
                configASSERT( ulStatus == 0 );

                if( ( ulStatus = prvRunRecovery( &xNetworkCredentials ) ) != 0 )
                {
                    LogError( ( "Failed to run recovery error code = 0x%08x\r\n", ulStatus ) );
                    configASSERT( ulStatus == 0 );
                }
                else if( ( ulStatus = prvSetupNetworkCredentials( &xNetworkCredentials ) ) != 0 )
                {
                    LogError( ( "Could not set network credentials\r\n" ) );
                    configASSERT( ulStatus == 0 );
                }
                else if( ( ulStatus = prvIoTHubInfoGet( &xNetworkCredentials, &pucIotHubHostname,
                                                        &pulIothubHostnameLength, &pucIotHubDeviceId,
                                                        &pulIothubDeviceIdLength ) ) != 0 )
                {
                    LogError( ( "Failed to run DPS after recovery!: error code = 0x%08x\r\n", ulStatus ) );
                    configASSERT( ulStatus == 0 );
                }
            }
            else
            {
                LogError( ( "Failed on sample_dps_entry!: error code = 0x%08x\r\n", ulStatus ) );
                configASSERT( false );
            }
        }
    #endif /* democonfigENABLE_DPS_SAMPLE */

    xNetworkContext.pParams = &xTlsTransportParams;

    for( ; ; )
    {
        /* Attempt to establish TLS session with IoT Hub. If connection fails,
         * retry after a timeout. Timeout value will be exponentially increased
         * until  the maximum number of attempts are reached or the maximum timeout
         * value is reached. The function returns a failure status if the TCP
         * connection cannot be established to the IoT Hub after the configured
         * number of attempts. */
        TlsTransportStatus_t ulTLSStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname,
                                                                                 democonfigIOTHUB_PORT,
                                                                                 &xNetworkCredentials, &xNetworkContext );

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

        xResult = AzureIoTMessage_PropertiesAppend( &xPropertyBag, ( uint8_t * ) "name", sizeof( "name" ) - 1,
                                                    ( uint8_t * ) "value", sizeof( "value" ) - 1 );
        configASSERT( xResult == eAzureIoTSuccess );

        /* Publish messages with QoS1, send and process Keep alive messages. */
        for( lPublishCount = 0; lPublishCount < lMaxPublishCount; lPublishCount++ )
        {
            ulScratchBufferLength = snprintf( ( char * ) ucScratchBuffer, sizeof( ucScratchBuffer ),
                                              sampleazureiotMESSAGE, lPublishCount );
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
                /* Send reported property every other cycle */
                ulScratchBufferLength = snprintf( ( char * ) ucScratchBuffer, sizeof( ucScratchBuffer ),
                                                  sampleazureiotPROPERTY, lPublishCount / 2 + 1 );
                xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient,
                                                                    ucScratchBuffer, ulScratchBufferLength,
                                                                    NULL );
                configASSERT( xResult == eAzureIoTSuccess );
            }

            /* Leave Connection Idle for some time. */
            LogInfo( ( "Keeping Connection Idle...\r\n\r\n" ) );
            vTaskDelay( sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS );
        }

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

        /* Close the network connection.  */
        TLS_Socket_Disconnect( &xNetworkContext );

        /* Wait for some time between two iterations to ensure that we do not
         * bombard the IoT Hub. */
        LogInfo( ( "Demo completed successfully.\r\n" ) );
        LogInfo( ( "Short delay before starting the next iteration.... \r\n\r\n" ) );
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

        /* Set the pParams member of the network context with desired transport. */
        xNetworkContext.pParams = &xTlsTransportParams;

        TlsTransportStatus_t ulTLSStatus = prvConnectToServerWithBackoffRetries( democonfigENDPOINT, democonfigIOTHUB_PORT,
                                                                                 pXNetworkCredentials, &xNetworkContext );

        if( ulTLSStatus == eTLSTransportCAVerifyFailed )
        {
            LogInfo( ( "In recovery\r\n" ) );

            return sampleazureiotRECOVERY_INITIATED;
        }

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

/**
 * @brief Run trust bundle recovery.
 */
    static uint32_t prvRunRecovery( NetworkCredentials_t * pXNetworkCredentials )
    {
        NetworkContext_t xNetworkContext = { 0 };
        TlsTransportParams_t xTlsTransportParams = { 0 };
        AzureIoTResult_t xResult;
        AzureIoTTransportInterface_t xTransport;
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
                                                   ( const uint8_t * ) democonfigRECOVERY_ID_SCOPE,
                                                   sizeof( democonfigRECOVERY_ID_SCOPE ) - 1,
                                                   ( const uint8_t * ) democonfigRECOVERY_REGISTRATION_ID,
                                                   #ifdef democonfigUSE_HSM
                                                       strlen( democonfigRECOVERY_REGISTRATION_ID ),
                                                   #else
                                                       sizeof( democonfigRECOVERY_REGISTRATION_ID ) - 1,
                                                   #endif
                                                   NULL, ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                                   ullGetUnixTime,
                                                   &xTransport );
        configASSERT( xResult == eAzureIoTSuccess );

        #ifdef democonfigDEVICE_RECOVERY_SYMMETRIC_KEY
            xResult = AzureIoTProvisioningClient_SetSymmetricKey( &xAzureIoTProvisioningClient,
                                                                  ( const uint8_t * ) democonfigDEVICE_RECOVERY_SYMMETRIC_KEY,
                                                                  sizeof( democonfigDEVICE_RECOVERY_SYMMETRIC_KEY ) - 1,
                                                                  Crypto_HMAC );
            configASSERT( xResult == eAzureIoTSuccess );
        #endif /* democonfigDEVICE_RECOVERY_SYMMETRIC_KEY */

        LogInfo( ( "Registering with Recovery DPS\r\n" ) );

        do
        {
            xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                           sampleazureiotProvisioning_Registration_TIMEOUT_MS );
        } while( xResult == eAzureIoTErrorPending );

        configASSERT( xResult == eAzureIoTSuccess );

        AzureIoTCARecovery_RecoveryPayload xRecoveryPayload;
        AzureIoTJSONReader_t xJSONReader;

        LogInfo( ( "Received Trust Bundle:\r\n" ) );
        LogInfo( ( "%.*s", az_span_size( xAzureIoTProvisioningClient._internal.xRegisterResponse.registration_state.payload ),
                   az_span_ptr( xAzureIoTProvisioningClient._internal.xRegisterResponse.registration_state.payload ) ) );
        xResult = AzureIoTJSONReader_Init( &xJSONReader,
                                           az_span_ptr( xAzureIoTProvisioningClient._internal.xRegisterResponse.registration_state.payload ),
                                           az_span_size( xAzureIoTProvisioningClient._internal.xRegisterResponse.registration_state.payload ) );
        configASSERT( xResult == eAzureIoTSuccess );

        LogInfo( ( "Parsing Recovery Payload\r\n" ) );
        xResult = AzureIoTCARecovery_ParseRecoveryPayload( &xJSONReader, &xRecoveryPayload );
        configASSERT( xResult == eAzureIoTSuccess );

        LogInfo( ( "Parsed Bundle: Version %.*s | Length %i\r\n", xRecoveryPayload.xTrustBundle.ulVersionLength,
                   xRecoveryPayload.xTrustBundle.pucVersion,
                   xRecoveryPayload.xTrustBundle.ulCertificatesLength ) );

        LogInfo( ( "Validating Trust Bundle Signature\r\n" ) );
        xResult = AzureIoTSample_RS256Verify( ( uint8_t * ) xRecoveryPayload.pucTrustBundleJSONObjectText,
                                              xRecoveryPayload.ulTrustBundleJSONObjectTextLength,
                                              ( uint8_t * ) xRecoveryPayload.pucPayloadSignature,
                                              xRecoveryPayload.ulPayloadSignatureLength,
                                              ucAzureIoTRecoveryRootKeyN,
                                              sizeof( ucAzureIoTRecoveryRootKeyN ) / sizeof( ucAzureIoTRecoveryRootKeyN[ 0 ] ),
                                              ucAzureIoTRecoveryKeyE,
                                              sizeof( ucAzureIoTRecoveryKeyE ) / sizeof( ucAzureIoTRecoveryKeyE[ 0 ] ),
                                              ucSignatureValidateScratchBuffer,
                                              sizeof( ucSignatureValidateScratchBuffer ) );
        configASSERT( xResult == eAzureIoTSuccess );
        LogInfo( ( "Trust Bundle Signature Successfully Validated\r\n" ) );

        LogInfo( ( "Unescaping the trust bundle cert\r\n" ) );
        az_span xUnescapeSpan = az_span_create( xRecoveryPayload.xTrustBundle.pucCertificates,
                                                xRecoveryPayload.xTrustBundle.ulCertificatesLength );
        xUnescapeSpan = az_json_string_unescape( xUnescapeSpan, xUnescapeSpan );

        LogInfo( ( "Unescaped bundle length %i value\r\n%.*s", az_span_size( xUnescapeSpan ), az_span_size( xUnescapeSpan ), az_span_ptr( xUnescapeSpan ) ) );

        LogInfo( ( "Writing trust bundle to NVS\r\n" ) );
        xResult = AzureIoTCAStorage_WriteTrustBundle( az_span_ptr( xUnescapeSpan ),
                                                      az_span_size( xUnescapeSpan ),
                                                      xRecoveryPayload.xTrustBundle.pucVersion,
                                                      xRecoveryPayload.xTrustBundle.ulVersionLength );
        configASSERT( xResult == eAzureIoTSuccess );

        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

        /* Close the network connection.  */
        TLS_Socket_Disconnect( &xNetworkContext );

        return 0;
    }

#endif /* democonfigENABLE_DPS_SAMPLE */
/*-----------------------------------------------------------*/

/**
 * @brief Connect to server with backoff retries.
 */
static TlsTransportStatus_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
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
            if( xNetworkStatus == eTLSTransportCAVerifyFailed )
            {
                /* Break to return error and handle recovery scenario */
                /* TODO: look into retrying in case other "CA failed" scenarios caused the */
                /* error instead. */
                break;
            }
            else
            {
                /* Generate a random number and calculate backoff value (in milliseconds) for
                 * the next connection retry.
                 * Note: It is recommended to seed the random number generator with a device-specific
                 * entropy source so that possibility of multiple devices retrying failed network operations
                 * at similar intervals can be avoided. */
                xBackoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &xReconnectParams, configRAND32(), &usNextRetryBackOff );

                if( xBackoffAlgStatus == BackoffAlgorithmRetriesExhausted )
                {
                    LogError( ( "Connection to the endpoint failed, all attempts exhausted." ) );
                }
                else if( xBackoffAlgStatus == BackoffAlgorithmSuccess )
                {
                    LogWarn( ( "Connection to the endpoint failed [%d]. "
                               "Retrying connection with backoff and jitter [%d]ms.",
                               xNetworkStatus, usNextRetryBackOff ) );
                    vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
                }
            }
        }
    } while( ( xNetworkStatus != eTLSTransportSuccess ) && ( xBackoffAlgStatus == BackoffAlgorithmSuccess ) );

    return xNetworkStatus;
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
