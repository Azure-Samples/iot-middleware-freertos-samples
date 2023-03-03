/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

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
#include "azure_sample_crypto.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* Data Interface Definition */
#include "sample_azure_iot_pnp_data_if.h"

/* Common Sample Helpers */
#include "sample_azure_common.h"

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
 * @brief Date-time to use for the model id
 */
#define sampleazureiotDATE_TIME_FORMAT                        "%Y-%m-%dT%H:%M:%S.000Z"

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

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    void * pParams;
};

AzureIoTHubClient_t xAzureIoTHubClient;

/* Telemetry buffers */
static uint8_t ucScratchBuffer[ 512 ];

/* Command buffers */
static uint8_t ucCommandResponsePayloadBuffer[ 256 ];

/* Reported Properties buffers */
static uint8_t ucReportedPropertiesUpdate[ 380 ];
static uint32_t ulReportedPropertiesUpdateLength;
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
 * @brief The task used to demonstrate the Azure IoT Hub API.
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

/**
 * @brief Publish messages with QoS1, send and process keep alive messages
 *
 * @param ulScratchBufferLength Length of scratch buffer for sending telemetry
 * @return AzureIoTResult_t The status of publishing and processing messages
 */
static AzureIoTResult_t prvTelemetryLoop( uint32_t ulScratchBufferLength );
/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucMQTTMessageBuffer[ democonfigNETWORK_BUFFER_SIZE ];

/**
 * @brief Internal function for handling Command requests.
 *
 * @remark This function is required for the interface with samples to work properly.
 */
static void prvHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                              void * pvContext )
{
    AzureIoTHubClient_t * pxHandle = ( AzureIoTHubClient_t * ) pvContext;
    uint32_t ulResponseStatus = 0;
    AzureIoTResult_t xResult;

    uint32_t ulCommandResponsePayloadLength = ulHandleCommand( pxMessage,
                                                               &ulResponseStatus,
                                                               ucCommandResponsePayloadBuffer,
                                                               sizeof( ucCommandResponsePayloadBuffer ) );

    if( ( xResult = AzureIoTHubClient_SendCommandResponse( pxHandle, pxMessage, ulResponseStatus,
                                                           ucCommandResponsePayloadBuffer,
                                                           ulCommandResponsePayloadLength ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error sending command response: result 0x%08x", ( uint16_t ) xResult ) );
    }
    else
    {
        LogInfo( ( "Successfully sent command response %d", ( int16_t ) ulResponseStatus ) );
    }
}


static void prvDispatchPropertiesUpdate( AzureIoTHubClientPropertiesResponse_t * pxMessage )
{
    vHandleWritableProperties( pxMessage,
                               ucReportedPropertiesUpdate,
                               sizeof( ucReportedPropertiesUpdate ),
                               &ulReportedPropertiesUpdateLength );

    if( ulReportedPropertiesUpdateLength == 0 )
    {
        LogError( ( "Failed to send response to writable properties update, length of response is zero." ) );
    }
    else
    {
        AzureIoTResult_t xResult;

        if( ( xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient,
                                                                  ucReportedPropertiesUpdate,
                                                                  ulReportedPropertiesUpdateLength,
                                                                  NULL ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Send properties reported failed: error=0x%08x\r\n", xResult ) );
            panic_handler();
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Private property message callback handler.
 *        This handler dispatches the calls to the functions defined in
 *        sample_azure_iot_pnp_data_if.h
 */
static void prvHandleProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                 void * pvContext )
{
    ( void ) pvContext;

    LogDebug( ( "Property document payload : %.*s \r\n",
                ( int16_t ) pxMessage->ulPayloadLength,
                ( const char * ) pxMessage->pvMessagePayload ) );

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubPropertiesRequestedMessage:
            LogDebug( ( "Device property document GET received" ) );
            prvDispatchPropertiesUpdate( pxMessage );
            break;

        case eAzureIoTHubPropertiesWritablePropertyMessage:
            LogDebug( ( "Device writeable property received" ) );
            prvDispatchPropertiesUpdate( pxMessage );
            break;

        case eAzureIoTHubPropertiesReportedResponseMessage:
            LogDebug( ( "Device reported property response received" ) );
            break;

        default:
            LogError( ( "Unknown property message: 0x%08x", pxMessage->xMessageType ) );
            configASSERT( false );
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
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub and
 *  function to adhere to the Plug and Play device convention.
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
        /* Attempt to establish TLS session with IoT Hub. If connection fails,
         * retry after a timeout. Timeout value will be exponentially increased
         * until  the maximum number of attempts are reached or the maximum timeout
         * value is reached. The function returns a failure status if the TCP
         * connection cannot be established to the IoT Hub after the configured
         * number of attempts. */
        if( ( ulStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname,
                                                               democonfigIOTHUB_PORT,
                                                               &xNetworkCredentials, &xNetworkContext ) ) != 0 )
        {
            LogError( ( "Error connecting to server\r\n" ) );
            panic_handler();
        }

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_Socket_Send;
        xTransport.xRecv = TLS_Socket_Recv;

        /* Init IoT Hub option */
        xResult = AzureIoTHubClient_OptionsInit( &xHubOptions );
        configASSERT( xResult == eAzureIoTSuccess );

        xHubOptions.pucModuleID = ( const uint8_t * ) democonfigMODULE_ID;
        xHubOptions.ulModuleIDLength = sizeof( democonfigMODULE_ID ) - 1;
        xHubOptions.pucModelID = ( const uint8_t * ) sampleazureiotMODEL_ID;
        xHubOptions.ulModelIDLength = sizeof( sampleazureiotMODEL_ID ) - 1;

        #ifdef democonfigPNP_COMPONENTS_LIST_LENGTH
            #if democonfigPNP_COMPONENTS_LIST_LENGTH > 0
                xHubOptions.pxComponentList = democonfigPNP_COMPONENTS_LIST;
                xHubOptions.ulComponentListLength = democonfigPNP_COMPONENTS_LIST_LENGTH;
            #endif /* > 0 */
        #endif /* democonfigPNP_COMPONENTS_LIST_LENGTH */

        if( ( xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                                pucIotHubHostname, pulIothubHostnameLength,
                                                pucIotHubDeviceId, pulIothubDeviceIdLength,
                                                &xHubOptions,
                                                ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                                ullGetUnixTime,
                                                &xTransport ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Failed to initialize IoT Hub client: error=0x%08x\r\n", xResult ) );
            panic_handler();
        }

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

        if( ( xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                                   false, &xSessionPresent,
                                                   sampleazureiotCONNACK_RECV_TIMEOUT_MS ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Failed to connect to IoT Hub client\r\n" ) );
        }
        else if( ( xResult = AzureIoTHubClient_SubscribeCommand( &xAzureIoTHubClient, prvHandleCommand,
                                                                 &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Command subscribe failed\r\n" ) );
        }
        else if( ( xResult = AzureIoTHubClient_SubscribeProperties( &xAzureIoTHubClient, prvHandleProperties,
                                                                    &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Property subscribe failed\r\n" ) );
        }
        /* Get property document after initial connection */
        else if( ( xResult = AzureIoTHubClient_RequestPropertiesAsync( &xAzureIoTHubClient ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Request properties failed\r\n" ) );
        }
        else
        {
            /* Publish messages with QoS1, send and process Keep alive messages. */
            for( ; ; )
            {
                if( ( xResult = prvTelemetryLoop( ulScratchBufferLength ) ) != eAzureIoTSuccess )
                {
                    LogError( ( "Telemetry loop failed: error=0x%08x\r\n", xResult ) );
                    panic_handler();
                }
            }

            if( ( xResult = AzureIoTHubClient_UnsubscribeProperties( &xAzureIoTHubClient ) ) != eAzureIoTSuccess )
            {
                LogError( ( "Property unsubscribe failed\r\n" ) );
            }
            else if( ( xResult = AzureIoTHubClient_UnsubscribeCommand( &xAzureIoTHubClient ) ) != eAzureIoTSuccess )
            {
                LogError( ( "Command unsubscribe failed\r\n" ) );
            }

            /* Send an MQTT Disconnect packet over the already connected TLS over
             * TCP connection. There is no corresponding response for the disconnect
             * packet. After sending disconnect, client must close the network
             * connection. */
            else if( ( xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient ) ) != eAzureIoTSuccess )
            {
                LogError( ( "Failed to disconnect from IoT Hub\r\n" ) );
            }
            /* Close the network connection.  */
            else
            {
                TLS_Socket_Disconnect( &xNetworkContext );

                /* Wait for some time between two iterations to ensure that we do not
                 * bombard the IoT Hub. */
                LogInfo( ( "Demo completed successfully.\r\n" ) );
                LogInfo( ( "Short delay before starting the next iteration.... \r\n\r\n" ) );
                vTaskDelay( sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS );
            }
        }

        if( xResult != eAzureIoTSuccess )
        {
            LogError( ( "Sample loop failed: error=0x%08x\n", xResult ) );
            panic_handler();
        }
    }
}
/*-----------------------------------------------------------*/

static AzureIoTResult_t prvTelemetryLoop( uint32_t ulScratchBufferLength )
{
    AzureIoTResult_t xResult = eAzureIoTSuccess;

    /* Hook for sending Telemetry */
    if( ( ulCreateTelemetry( ucScratchBuffer, sizeof( ucScratchBuffer ), &ulScratchBufferLength ) == 0 ) &&
        ( ulScratchBufferLength > 0 ) )
    {
        xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                   ucScratchBuffer, ulScratchBufferLength,
                                                   NULL, eAzureIoTHubMessageQoS1, NULL );
    }

    if( xResult == eAzureIoTSuccess )
    {
        /* Hook for sending update to reported properties */
        ulReportedPropertiesUpdateLength = ulCreateReportedPropertiesUpdate( ucReportedPropertiesUpdate, sizeof( ucReportedPropertiesUpdate ) );

        if( ulReportedPropertiesUpdateLength > 0 )
        {
            xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient, ucReportedPropertiesUpdate, ulReportedPropertiesUpdateLength, NULL );
        }
    }

    if( xResult == eAzureIoTSuccess )
    {
        LogInfo( ( "Attempt to receive publish message from IoT Hub.\r\n" ) );
        xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                 sampleazureiotPROCESS_LOOP_TIMEOUT_MS );
    }

    if( xResult == eAzureIoTSuccess )
    {
        /* Leave Connection Idle for some time. */
        LogInfo( ( "Keeping Connection Idle...\r\n\r\n" ) );
        vTaskDelay( sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS );
    }

    return xResult;
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

        if( ( ulStatus = prvConnectToServerWithBackoffRetries( democonfigENDPOINT, democonfigIOTHUB_PORT,
                                                               pXNetworkCredentials, &xNetworkContext ) ) != 0 )
        {
            LogError( ( "Error connecting to server\r\n" ) );
            panic_handler();
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

            if( ( ulStatus = getRegistrationId( &registration_id ) ) != 0 )
            {
                LogError( ( "Error getting registration ID\r\n" ) );
                panic_handler();
            }

#undef democonfigREGISTRATION_ID
        #define democonfigREGISTRATION_ID    registration_id
        #endif /* ifdef democonfigUSE_HSM */

        if( ( xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
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
                                                         &xTransport ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Provisioning client init failed\r\n" ) );
        }
        else
        {
            #ifdef democonfigDEVICE_SYMMETRIC_KEY
                xResult = AzureIoTProvisioningClient_SetSymmetricKey( &xAzureIoTProvisioningClient,
                                                                      ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                                      sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                                      Crypto_HMAC );
                configASSERT( xResult == eAzureIoTSuccess );
            #endif /* democonfigDEVICE_SYMMETRIC_KEY */

            if( ( xResult = AzureIoTProvisioningClient_SetRegistrationPayload( &xAzureIoTProvisioningClient,
                                                                               ( const uint8_t * ) sampleazureiotPROVISIONING_PAYLOAD,
                                                                               sizeof( sampleazureiotPROVISIONING_PAYLOAD ) - 1 ) ) != eAzureIoTSuccess )
            {
                LogError( ( "Set registration payload failed\r\n" ) );
            }
            else
            {
                do
                {
                    xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                                   sampleazureiotProvisioning_Registration_TIMEOUT_MS );
                } while( xResult == eAzureIoTErrorPending );

                if( xResult != eAzureIoTSuccess )
                {
                    LogInfo( ( "Error getting IoT Hub name and Device ID: 0x%08x", ( uint16_t ) xResult ) );
                }
                else
                {
                    LogInfo( ( "Successfully acquired IoT Hub name and Device ID" ) );

                    if( ( xResult = AzureIoTProvisioningClient_GetDeviceAndHub( &xAzureIoTProvisioningClient,
                                                                                ucSampleIotHubHostname, &ucSamplepIothubHostnameLength,
                                                                                ucSampleIotHubDeviceId, &ucSamplepIothubDeviceIdLength ) ) != eAzureIoTSuccess )
                    {
                        LogError( ( "Get device and hub failed\r\n" ) );
                    }
                    else
                    {
                        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

                        /* Close the network connection.  */
                        TLS_Socket_Disconnect( &xNetworkContext );

                        *ppucIothubHostname = ucSampleIotHubHostname;
                        *pulIothubHostnameLength = ucSamplepIothubHostnameLength;
                        *ppucIothubDeviceId = ucSampleIotHubDeviceId;
                        *pulIothubDeviceIdLength = ucSamplepIothubDeviceIdLength;
                    }
                }
            }
        }

        if( xResult != eAzureIoTSuccess )
        {
            LogError( ( "Getting IoT Hub info failed: error=0x%08x\n", xResult ) );
            panic_handler();
            return 1;
        }

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
        LogInfo( ( "Creating a TLS connection to %s:%u.\r\n", pcHostName, ( uint16_t ) port ) );
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
