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

/* DPS CSR implies DPS provisioning — auto-enable it. */
#if defined( democonfigENABLE_DPS_CSR ) && !defined( democonfigENABLE_DPS_SAMPLE )
    #define democonfigENABLE_DPS_SAMPLE
#endif

#if !defined( democonfigDEVICE_SYMMETRIC_KEY ) && \
    !defined( democonfigCLIENT_CERTIFICATE_PEM ) && \
    !defined( democonfigENABLE_DPS_CSR )
    #error "Please define one of: democonfigDEVICE_SYMMETRIC_KEY, democonfigCLIENT_CERTIFICATE_PEM, or democonfigENABLE_DPS_CSR."
#endif

#if ( defined( democonfigENABLE_DPS_CSR ) || defined( democonfigENABLE_IOT_HUB_CSR ) ) && \
    ( !defined( democonfigCERTIFICATE_SIGNING_REQUEST_DATA ) || \
      !defined( democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM ) )
    #error "CSR is enabled but democonfigCERTIFICATE_SIGNING_REQUEST_DATA and/or democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM is not defined."
#endif

#if defined( democonfigENABLE_IOT_HUB_CSR ) && !defined( democonfigCERTIFICATE_SIGNING_REQUEST_ID )
    #error "democonfigENABLE_IOT_HUB_CSR requires democonfigCERTIFICATE_SIGNING_REQUEST_ID."
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
 * @brief  The content type of the Telemetry message published in this example.
 * @remark Message properties must be url-encoded.
 *         This message property is not required to send telemetry.
 */
#define sampleazureiotMESSAGE_CONTENT_TYPE                    "text%2Fplain"

/**
 * @brief  The content encoding of the Telemetry message published in this example.
 * @remark Message properties must be url-encoded.
 *         This message property is not required to send telemetry.
 */
#define sampleazureiotMESSAGE_CONTENT_ENCODING                "us-ascii"

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

#if defined( democonfigENABLE_DPS_CSR ) || defined( democonfigENABLE_IOT_HUB_CSR )

/**
 * @brief Buffer size for storing the issued certificate from CSR response.
 */
    #define sampleazureiotCSR_ISSUED_CERT_BUFFER_SIZE         ( 8 * 1024U )

#endif /* democonfigENABLE_DPS_CSR || democonfigENABLE_IOT_HUB_CSR */

#ifdef democonfigENABLE_IOT_HUB_CSR

/**
 * @brief Buffer size for CSR JSON payload construction.
 */
    #define sampleazureiotCSR_PAYLOAD_BUFFER_SIZE             ( 1024U )

/**
 * @brief Timeout waiting for the initial CSR response (90s with grace over the 60s gateway timeout).
 */
    #define sampleazureiotCSR_INITIAL_RESPONSE_TIMEOUT_MS     ( 90 * 1000U )

/**
 * @brief Timeout waiting for certificate issuance after 202 Accepted.
 */
    #define sampleazureiotCSR_COMPLETION_TIMEOUT_MS            ( 12 * 60 * 60 * 1000U )

#endif /* democonfigENABLE_IOT_HUB_CSR */
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

/* Shared CSR issued certificate buffer. */
#if defined( democonfigENABLE_DPS_CSR ) || defined( democonfigENABLE_IOT_HUB_CSR )
    static uint8_t ucCSRIssuedCertBuffer[ sampleazureiotCSR_ISSUED_CERT_BUFFER_SIZE ];
    static uint32_t ulCSRIssuedCertLength = 0;
#endif /* democonfigENABLE_DPS_CSR || democonfigENABLE_IOT_HUB_CSR */

static uint8_t ucPropertyBuffer[ 80 ];
static uint8_t ucScratchBuffer[ 128 ];

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    void * pParams;
};

static AzureIoTHubClient_t xAzureIoTHubClient;

#ifdef democonfigENABLE_IOT_HUB_CSR
    static uint8_t ucCSRPayloadBuffer[ sampleazureiotCSR_PAYLOAD_BUFFER_SIZE ];
    static volatile bool xCSRAccepted  = false;
    static volatile bool xCSRCompleted = false;
    static volatile bool xCSRError     = false;
    static volatile uint16_t usCSRErrorStatus = 0;

#endif /* democonfigENABLE_IOT_HUB_CSR */
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

/* Shared PEM certificate header/footer constants for CSR flows. */
#if defined( democonfigENABLE_DPS_CSR ) || defined( democonfigENABLE_IOT_HUB_CSR )
    static const char pcPemCertBegin[] = "-----BEGIN CERTIFICATE-----\r\n";
    static const char pcPemCertEnd[]   = "\r\n-----END CERTIFICATE-----\r\n";
#endif /* democonfigENABLE_DPS_CSR || democonfigENABLE_IOT_HUB_CSR */

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

#ifdef democonfigENABLE_IOT_HUB_CSR

/**
 * @brief CSR response callback handler.
 */
static void prvHandleCSRResponse( AzureIoTHubClientCertificateSigningResponse_t * pxResponse,
                                   void * pvContext )
{
    ( void ) pvContext;

    LogInfo( ( "[CSR] Received response: status=%u, requestID=%.*s",
               ( unsigned ) pxResponse->xMessageStatus,
               ( int ) pxResponse->usRequestIDLength,
               ( const char * ) pxResponse->pucRequestID ) );

    switch( pxResponse->xResponseType )
    {
        case eAzureIoTHubClientCertificateSigningResponseAccepted:
            LogInfo( ( "[CSR] Accepted (202). Payload: %.*s",
                       ( int ) pxResponse->ulPayloadLength,
                       ( const char * ) pxResponse->pvMessagePayload ) );
            xCSRAccepted = true;
            break;

        case eAzureIoTHubClientCertificateSigningResponseCompleted:
        {
            LogInfo( ( "[CSR] Completed (200). Certificate received (%u bytes).",
                       ( unsigned ) pxResponse->ulPayloadLength ) );

            /* Use the accessor functions to retrieve the parsed certificate chain.
             * The middleware auto-parses the completed response; the accessor data
             * is only valid until the next ProcessLoop call. */
            uint32_t ulChainLength = 0;
            AzureIoTResult_t xAccessorResult;

            xAccessorResult = AzureIoTHubClient_GetIssuedCertificateChainLength(
                                  &xAzureIoTHubClient, &ulChainLength );

            if( ( xAccessorResult != eAzureIoTSuccess ) || ( ulChainLength == 0 ) )
            {
                LogError( ( "[CSR] Failed to get certificate chain length or chain is empty." ) );
                ulCSRIssuedCertLength = 0;
            xCSRCompleted = true;
            break;
            }

            LogInfo( ( "[CSR] Issued certificate chain contains %u certificate(s).",
                       ( unsigned ) ulChainLength ) );

    uint32_t ulWritten = 0;

            for( uint32_t i = 0; i < ulChainLength; i++ )
            {
                /* Write PEM header at the current offset, then let the API fill
                 * the base64 data immediately after the header, then append footer. */
                uint32_t ulHeaderLen = ( uint32_t ) ( sizeof( pcPemCertBegin ) - 1 );
                uint32_t ulFooterLen = ( uint32_t ) ( sizeof( pcPemCertEnd ) - 1 );

                /* Ensure space for at least header + footer + null term. */
                if( ( ulWritten + ulHeaderLen + ulFooterLen + 1 ) >= sampleazureiotCSR_ISSUED_CERT_BUFFER_SIZE )
    {
                    LogError( ( "[CSR] PEM buffer too small for certificate %u.", ( unsigned ) i ) );
                    ulWritten = 0;
                    break;
    }

                /* Write header. */
                memcpy( ucCSRIssuedCertBuffer + ulWritten, pcPemCertBegin, ulHeaderLen );

                /* GetIssuedCertificate fills base64 data after the header. */
                uint32_t ulRawLen = sampleazureiotCSR_ISSUED_CERT_BUFFER_SIZE - ulWritten - ulHeaderLen - ulFooterLen - 1;
                xAccessorResult = AzureIoTHubClient_GetIssuedCertificate(
                                      &xAzureIoTHubClient, i,
                                      ucCSRIssuedCertBuffer + ulWritten + ulHeaderLen,
                                      &ulRawLen );

                if( xAccessorResult != eAzureIoTSuccess )
                {
                    LogError( ( "[CSR] Failed to get certificate at position %u: %d",
                                ( unsigned ) i, xAccessorResult ) );
                    ulWritten = 0;
            break;
        }

                /* Append footer after the base64 data. */
                memcpy( ucCSRIssuedCertBuffer + ulWritten + ulHeaderLen + ulRawLen,
                        pcPemCertEnd, ulFooterLen );

                ulWritten += ulHeaderLen + ulRawLen + ulFooterLen;

                /* Log the certificate in PEM format. */
                LogInfo( ( "[CSR] Certificate[%u]:\r\n%.*s",
                           ( unsigned ) i,
                           ( int ) ( ulHeaderLen + ulRawLen + ulFooterLen ),
                           ( const char * ) ( ucCSRIssuedCertBuffer + ulWritten - ulHeaderLen - ulRawLen - ulFooterLen ) ) );
        }

            if( ulWritten > 0 )
            {
    /* Null-terminate (required by mbedTLS PEM parser). */
                ucCSRIssuedCertBuffer[ ulWritten ] = '\0';
    ulWritten++;
                ulCSRIssuedCertLength = ulWritten;

                LogInfo( ( "[CSR] Certificate chain converted to PEM (%u bytes).",
                           ( unsigned ) ulCSRIssuedCertLength ) );
            }
            else
            {
                ulCSRIssuedCertLength = 0;
            }

            xCSRCompleted = true;
            break;
        }

        case eAzureIoTHubClientCertificateSigningResponseError:
            LogError( ( "[CSR] Error (%u). Payload: %.*s",
                        ( unsigned ) pxResponse->xMessageStatus,
                        ( int ) pxResponse->ulPayloadLength,
                        ( const char * ) pxResponse->pvMessagePayload ) );
            xCSRError = true;
            usCSRErrorStatus = ( uint16_t ) pxResponse->xMessageStatus;
            break;

        default:
            LogError( ( "[CSR] Unknown response type: %d",
                        ( int ) pxResponse->xResponseType ) );
            break;
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Run the MQTT process loop until a CSR flag is set or timeout expires.
 */
static AzureIoTResult_t prvWaitForCSRResponse( uint32_t ulTimeoutMs )
{
    AzureIoTResult_t xResult;
    uint32_t ulElapsedMs = 0;

    while( !xCSRAccepted && !xCSRCompleted && !xCSRError &&
           ( ulElapsedMs < ulTimeoutMs ) )
    {
        xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                  sampleazureiotPROCESS_LOOP_TIMEOUT_MS );

        if( xResult != eAzureIoTSuccess )
        {
            LogError( ( "[CSR] ProcessLoop failed: %d", xResult ) );
            return xResult;
        }

        ulElapsedMs += sampleazureiotPROCESS_LOOP_TIMEOUT_MS;
    }

    return eAzureIoTSuccess;
}
/*-----------------------------------------------------------*/

/**
 * @brief Send a CSR and wait for the two-phase response (202 Accepted, then 200 Completed).
 */
static AzureIoTResult_t prvSendCSRAndWaitForCertificate( void )
{
    AzureIoTResult_t xResult;

    /* Step 1: Send the CSR. */
    xResult = AzureIoTHubClient_SendCertificateSigningRequest(
                  &xAzureIoTHubClient,
                  ( const uint8_t * ) democonfigCERTIFICATE_SIGNING_REQUEST_DATA,
                  sizeof( democonfigCERTIFICATE_SIGNING_REQUEST_DATA ) - 1,
                  ( const uint8_t * ) democonfigCERTIFICATE_SIGNING_REQUEST_ID,
                  sizeof( democonfigCERTIFICATE_SIGNING_REQUEST_ID ) - 1,
                  NULL,
                  ucCSRPayloadBuffer,
                  sizeof( ucCSRPayloadBuffer ) );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[CSR] Failed to send CSR: %d", xResult ) );
        return xResult;
    }

    LogInfo( ( "[CSR] CSR sent successfully. Waiting for 202 Accepted..." ) );

    /* Step 2: Wait for 202 Accepted or error. */
    xCSRAccepted = false;
    xCSRCompleted = false;
    xCSRError = false;

    xResult = prvWaitForCSRResponse( sampleazureiotCSR_INITIAL_RESPONSE_TIMEOUT_MS );

    if( xResult != eAzureIoTSuccess )
    {
        return xResult;
    }

    /* Handle 409 conflict — retry with replace="*". */
    if( xCSRError && ( usCSRErrorStatus == eAzureIoTStatusNotConflict ) )
    {
        LogInfo( ( "[CSR] Conflict (409). Resubmitting with replace=\"*\"..." ) );

        AzureIoTHubClientCertificateSigningRequestOptions_t xOptions = { 0 };
        xOptions.pucReplace = ( const uint8_t * ) "*";
        xOptions.usReplaceLength = 1;

        xCSRError = false;

        xResult = AzureIoTHubClient_SendCertificateSigningRequest(
                      &xAzureIoTHubClient,
                      ( const uint8_t * ) democonfigCERTIFICATE_SIGNING_REQUEST_DATA,
                      sizeof( democonfigCERTIFICATE_SIGNING_REQUEST_DATA ) - 1,
                      ( const uint8_t * ) democonfigCERTIFICATE_SIGNING_REQUEST_ID,
                      sizeof( democonfigCERTIFICATE_SIGNING_REQUEST_ID ) - 1,
                      &xOptions,
                      ucCSRPayloadBuffer,
                      sizeof( ucCSRPayloadBuffer ) );

        if( xResult != eAzureIoTSuccess )
        {
            LogError( ( "[CSR] Failed to resend CSR with replace: %d", xResult ) );
            return xResult;
        }

        xResult = prvWaitForCSRResponse( sampleazureiotCSR_INITIAL_RESPONSE_TIMEOUT_MS );

        if( xResult != eAzureIoTSuccess )
        {
            return xResult;
        }
    }

    if( xCSRError )
    {
        LogError( ( "[CSR] CSR request failed with status %u.",
                    ( unsigned ) usCSRErrorStatus ) );
        return eAzureIoTErrorFailed;
    }

    if( !xCSRAccepted && !xCSRCompleted )
    {
        LogError( ( "[CSR] Timed out waiting for initial response." ) );
        return eAzureIoTErrorFailed;
    }

    /* Step 3: Wait for 200 Completed. */
    if( !xCSRCompleted )
    {
        LogInfo( ( "[CSR] Accepted. Waiting for certificate (200 Completed)..." ) );

        /* Reset xCSRAccepted so the wait loop doesn't exit immediately
         * (it was set to true by the 202 response above). */
        xCSRAccepted = false;

        xResult = prvWaitForCSRResponse( sampleazureiotCSR_COMPLETION_TIMEOUT_MS );

        if( xResult != eAzureIoTSuccess )
        {
            return xResult;
        }
    }

    if( xCSRCompleted )
    {
        LogInfo( ( "[CSR] Certificate issued successfully!" ) );
        return eAzureIoTSuccess;
    }

    if( xCSRError )
    {
        LogError( ( "[CSR] Certificate issuance failed with status %u.",
                    ( unsigned ) usCSRErrorStatus ) );
    }
    else
    {
        LogError( ( "[CSR] Timed out waiting for certificate." ) );
    }

    return eAzureIoTErrorFailed;
}

#endif /* democonfigENABLE_IOT_HUB_CSR */
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
    #endif
    #ifdef democonfigCLIENT_PRIVATE_KEY_PEM
        pxNetworkCredentials->pucPrivateKey = ( const unsigned char * ) democonfigCLIENT_PRIVATE_KEY_PEM;
        pxNetworkCredentials->xPrivateKeySize = sizeof( democonfigCLIENT_PRIVATE_KEY_PEM );
    #elif defined( democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM ) && !defined( democonfigDEVICE_SYMMETRIC_KEY )
        /* For Certificate Signing Request (CSR) auth without symmetric key,
         * the CSR private key is used for the initial TLS connection. */
        pxNetworkCredentials->pucPrivateKey = ( const unsigned char * ) democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM;
        pxNetworkCredentials->xPrivateKeySize = sizeof( democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM );
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
    bool xPropertiesSubscribed;

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
            LogError( ( "Failed on sample_dps_entry!: error code = 0x%08x\r\n", ( uint16_t ) ulStatus ) );
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

            #if defined( democonfigDEVICE_SYMMETRIC_KEY ) && !defined( democonfigENABLE_DPS_CSR )
                /* When DPS CSR is enabled, IoT Hub auth uses the CSR-issued certificate
                 * instead of symmetric key. Symmetric key is only for DPS authentication. */
                xResult = AzureIoTHubClient_SetSymmetricKey( &xAzureIoTHubClient,
                                                             ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                             sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                             Crypto_HMAC );
                configASSERT( xResult == eAzureIoTSuccess );
            #endif /* democonfigDEVICE_SYMMETRIC_KEY && !democonfigENABLE_DPS_CSR */

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

            /* When IoT Hub CSR is enabled, defer twin/properties subscription until
             * after CSR reconnect. The DPS-issued certificate may not be authorized
             * for twin operations on the initial connection. */
            xPropertiesSubscribed = false;
            #ifndef democonfigENABLE_IOT_HUB_CSR
            xResult = AzureIoTHubClient_SubscribeProperties( &xAzureIoTHubClient, prvHandlePropertiesMessage,
                                                             &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
            configASSERT( xResult == eAzureIoTSuccess );
            xPropertiesSubscribed = true;
            #endif

            #ifdef democonfigENABLE_IOT_HUB_CSR
                xResult = AzureIoTHubClient_SubscribeCertificateSigningResponse( &xAzureIoTHubClient,
                                                                                  prvHandleCSRResponse,
                                                                                  &xAzureIoTHubClient,
                                                                                  sampleazureiotSUBSCRIBE_TIMEOUT );
                configASSERT( xResult == eAzureIoTSuccess );

                LogInfo( ( "[CSR] Sending certificate signing request..." ) );
                xResult = prvSendCSRAndWaitForCertificate();

                if( ( xResult == eAzureIoTSuccess ) && ( ulCSRIssuedCertLength > 0 ) )
                {
                    LogInfo( ( "[CSR] Certificate issued. Reconnecting with new certificate..." ) );

                    /* Unsubscribe and disconnect the current session. */
                    AzureIoTHubClient_UnsubscribeCertificateSigningResponse( &xAzureIoTHubClient );
                    AzureIoTHubClient_UnsubscribeCommand( &xAzureIoTHubClient );
                    AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( &xAzureIoTHubClient );
                    AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
                    AzureIoTHubClient_Deinit( &xAzureIoTHubClient );
                    TLS_Socket_Disconnect( &xNetworkContext );

                    /* Update TLS credentials with the issued certificate.
                     * The private key is the one that generated the CSR. */
                    xNetworkCredentials.pucClientCert = ucCSRIssuedCertBuffer;
                    xNetworkCredentials.xClientCertSize = ulCSRIssuedCertLength;
                    xNetworkCredentials.pucPrivateKey = ( const unsigned char * ) democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM;
                    xNetworkCredentials.xPrivateKeySize = sizeof( democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM );

                    LogInfo( ( "[CSR] Reconnecting to %s with issued certificate (%u bytes)...",
                               pucIotHubHostname, ( unsigned ) ulCSRIssuedCertLength ) );

                    /* Reconnect TLS with the new certificate. */
                    ulStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname,
                                                                     democonfigIOTHUB_PORT,
                                                                     &xNetworkCredentials, &xNetworkContext );
                    configASSERT( ulStatus == 0 );

                    /* Re-initialize transport and hub client for the new connection. */
                    xTransport.pxNetworkContext = &xNetworkContext;
                    xTransport.xSend = TLS_Socket_Send;
                    xTransport.xRecv = TLS_Socket_Recv;

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

                    xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                                         false, &xSessionPresent,
                                                         sampleazureiotCONNACK_RECV_TIMEOUT_MS );
                    configASSERT( xResult == eAzureIoTSuccess );

                    LogInfo( ( "[CSR] Reconnected with issued certificate." ) );

                    /* Re-subscribe to cloud features. */
                    xResult = AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xAzureIoTHubClient, prvHandleCloudMessage,
                                                                               &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
                    configASSERT( xResult == eAzureIoTSuccess );

                    xResult = AzureIoTHubClient_SubscribeCommand( &xAzureIoTHubClient, prvHandleCommand,
                                                                  &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
                    configASSERT( xResult == eAzureIoTSuccess );

                    xResult = AzureIoTHubClient_SubscribeProperties( &xAzureIoTHubClient, prvHandlePropertiesMessage,
                                                                     &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
                    configASSERT( xResult == eAzureIoTSuccess );
                    xPropertiesSubscribed = true;
                }
                else
                {
                    LogError( ( "[CSR] CSR workflow failed or no certificate received: %d", xResult ) );
                    AzureIoTHubClient_UnsubscribeCertificateSigningResponse( &xAzureIoTHubClient );
                }
            #endif /* democonfigENABLE_IOT_HUB_CSR */

            /* Get property document after initial connection */
            if( xPropertiesSubscribed )
            {
                xResult = AzureIoTHubClient_RequestPropertiesAsync( &xAzureIoTHubClient );
                configASSERT( xResult == eAzureIoTSuccess );
            }

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

                if( ( lPublishCount % 2 == 0 ) && xPropertiesSubscribed )
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

            if( xAzureSample_IsConnectedToInternet() )
            {
                if( xPropertiesSubscribed )
                {
                    xResult = AzureIoTHubClient_UnsubscribeProperties( &xAzureIoTHubClient );
                    configASSERT( xResult == eAzureIoTSuccess );
                }

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
            LogInfo( ( "Demo completed successfully.\r\n" ) );
        }

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

        #ifdef democonfigENABLE_DPS_CSR
            xResult = AzureIoTProvisioningClient_SetRegistrationCertificateSigningRequest( &xAzureIoTProvisioningClient,
                                                                  ( const uint8_t * ) democonfigCERTIFICATE_SIGNING_REQUEST_DATA,
                                                                  sizeof( democonfigCERTIFICATE_SIGNING_REQUEST_DATA ) - 1 );
            configASSERT( xResult == eAzureIoTSuccess );
        #endif /* democonfigENABLE_DPS_CSR */

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

        #ifdef democonfigENABLE_DPS_CSR
        {
            uint32_t ulChainLength = 0;
            uint32_t ulWritten = 0;

            xResult = AzureIoTProvisioningClient_GetIssuedCertificateChainLength(
                          &xAzureIoTProvisioningClient, &ulChainLength );
            configASSERT( xResult == eAzureIoTSuccess );
            configASSERT( ulChainLength > 0 );

            for( uint32_t i = 0; i < ulChainLength; i++ )
            {
                /* Write PEM header at the current offset, then let the API fill
                 * the base64 data immediately after the header, then append footer. */
                uint32_t ulHeaderLen = ( uint32_t ) ( sizeof( pcPemCertBegin ) - 1 );
                uint32_t ulFooterLen = ( uint32_t ) ( sizeof( pcPemCertEnd ) - 1 );

                /* Ensure space for at least header + footer + null term. */
                configASSERT( ( ulWritten + ulHeaderLen + ulFooterLen + 1 ) < sampleazureiotCSR_ISSUED_CERT_BUFFER_SIZE );

                /* Write header. */
                memcpy( ucCSRIssuedCertBuffer + ulWritten, pcPemCertBegin, ulHeaderLen );

                /* GetIssuedCertificate fills base64 data after the header. */
                uint32_t ulRawLen = sampleazureiotCSR_ISSUED_CERT_BUFFER_SIZE - ulWritten - ulHeaderLen - ulFooterLen - 1;
                xResult = AzureIoTProvisioningClient_GetIssuedCertificate(
                              &xAzureIoTProvisioningClient, i,
                              ucCSRIssuedCertBuffer + ulWritten + ulHeaderLen,
                              &ulRawLen );
                configASSERT( xResult == eAzureIoTSuccess );

                /* Append footer after the base64 data. */
                memcpy( ucCSRIssuedCertBuffer + ulWritten + ulHeaderLen + ulRawLen,
                        pcPemCertEnd, ulFooterLen );

                ulWritten += ulHeaderLen + ulRawLen + ulFooterLen;
            }

            /* Null-terminate (required by mbedTLS PEM parser). */
            ucCSRIssuedCertBuffer[ ulWritten ] = '\0';
            ulWritten++;

            ulCSRIssuedCertLength = ulWritten;

            pXNetworkCredentials->pucClientCert = ( const unsigned char * ) ucCSRIssuedCertBuffer;
            pXNetworkCredentials->xClientCertSize = ulCSRIssuedCertLength;
            pXNetworkCredentials->pucPrivateKey = ( const unsigned char * ) democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM;
            pXNetworkCredentials->xPrivateKeySize = sizeof( democonfigCERTIFICATE_SIGNING_REQUEST_PRIVATE_KEY_PEM );
        }
        #endif /* democonfigENABLE_DPS_CSR */

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
