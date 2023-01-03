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
#include "azure_iot_adu_client.h"
#include "azure_iot_flash_platform.h"
#include "azure_iot_http.h"

/* Azure JSON includes */
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
#include "transport_tls_socket.h"
#include "transport_socket.h"

/* Crypto helper header. */
#include "azure_sample_crypto.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* Data Interface Definition */
#include "sample_azure_iot_pnp_data_if.h"
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
#define sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS          ( 5000U )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotProvisioning_Registration_TIMEOUT_MS    ( 3 * 1000U )

/**
 * @brief Wait timeout for subscribe to finish.
 */
#define sampleazureiotSUBSCRIBE_TIMEOUT                       ( 10 * 1000U )

/**
 * @brief Timeout for downloading the update image and calling the
 * AzureIoTHubClient_ProcessLoop() function (in seconds)
 */
#define sampleazureiotADU_DOWNLOAD_TIMEOUT_SEC                ( 10 )

/**
 * @brief Buffer size for ADU HTTP download headers
 *
 */
#define ADU_HEADER_BUFFER_SIZE                                512

#define democonfigADU_UPDATE_ID                               "{\"provider\":\"" democonfigADU_UPDATE_PROVIDER "\",\"name\":\"" democonfigADU_UPDATE_NAME "\",\"version\":\"" democonfigADU_UPDATE_VERSION "\"}"

#ifdef democonfigADU_UPDATE_NEW_VERSION
    #define democonfigADU_SPOOFED_UPDATE_ID                   "{\"provider\":\"" democonfigADU_UPDATE_PROVIDER "\",\"name\":\"" democonfigADU_UPDATE_NAME "\",\"version\":\"" democonfigADU_UPDATE_NEW_VERSION "\"}"
#endif
/*-----------------------------------------------------------*/

/**
 * @brief Unix time.
 *
 * @return Time in seconds.
 */
uint64_t ullGetUnixTime( void );
/*-----------------------------------------------------------*/

/* Define buffer for IoT Hub info.  */
#ifdef democonfigENABLE_DPS_SAMPLE
    static uint8_t ucSampleIotHubHostname[ 128 ];
    static uint8_t ucSampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
    #define sampleazureiotMODEL_ID_STR    "modelId"
#endif /* democonfigENABLE_DPS_SAMPLE */

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    void * pParams;
};

AzureIoTHubClient_t xAzureIoTHubClient;
AzureIoTADUClient_t xAzureIoTADUClient;
AzureIoTADUUpdateRequest_t xAzureIoTAduUpdateRequest;
bool xProcessUpdateRequest = false;

AzureIoTADUClientDeviceProperties_t xADUDeviceProperties =
{
    .ucManufacturer                           = ( const uint8_t * ) democonfigADU_DEVICE_MANUFACTURER,
    .ulManufacturerLength                     = sizeof( democonfigADU_DEVICE_MANUFACTURER ) - 1,
    .ucModel                                  = ( const uint8_t * ) democonfigADU_DEVICE_MODEL,
    .ulModelLength                            = sizeof( democonfigADU_DEVICE_MODEL ) - 1,
    .ucCurrentUpdateId                        = ( const uint8_t * ) democonfigADU_UPDATE_ID,
    .ulCurrentUpdateIdLength                  = sizeof( democonfigADU_UPDATE_ID ) - 1,
    .ucDeliveryOptimizationAgentVersion       = NULL,
    .ulDeliveryOptimizationAgentVersionLength = 0
};

static AzureADUImage_t xImage;

/* Telemetry buffers */
static uint8_t ucScratchBuffer[ 700 ];

/* Command buffers */
static uint8_t ucCommandResponsePayloadBuffer[ 128 ];

/* Reported Properties buffers */
static uint8_t ucReportedPropertiesUpdate[ 1500 ];
static uint32_t ulReportedPropertiesUpdateLength;

static uint8_t ucAduDownloadBuffer[ democonfigCHUNK_DOWNLOAD_SIZE + 1024 ];
static uint8_t ucAduDownloadHeaderBuffer[ ADU_HEADER_BUFFER_SIZE ];

const uint8_t sampleaduDEFAULT_RESULT_DETAILS[] = "Ok";

#define sampleaduPNP_COMPONENTS_LIST_LENGTH    1
static AzureIoTHubClientComponent_t pnp_components[ sampleaduPNP_COMPONENTS_LIST_LENGTH ] =
{
    azureiothubCREATE_COMPONENT( AZ_IOT_ADU_CLIENT_PROPERTIES_COMPONENT_NAME )
};
#define sampleaduPNP_COMPONENTS_LIST    pnp_components

/* This is an user-defined code to be reported as the result of the */
/* OTA update on the Azure Device Update portal. */
#define sampleaduSAMPLE_EXTENDED_RESULT_CODE    1234

/* This does not affect devices that actually implement the ADU process */
/* as they will reboot before getting to the place where this is used. */
bool xDidDeviceUpdate = false;

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
        LogInfo( ( "Successfully sent command response %u", ( uint16_t ) ulResponseStatus ) );
    }
}


static void prvDispatchPropertiesUpdate( AzureIoTHubClientPropertiesResponse_t * pxMessage )
{
    vHandleWritableProperties( pxMessage,
                               ucReportedPropertiesUpdate,
                               sizeof( ucReportedPropertiesUpdate ),
                               &ulReportedPropertiesUpdateLength );
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

    LogDebug( ( "Property document payload : %.*s ",
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

static void prvConnectHTTP( AzureIoTTransportInterface_t * pxHTTPTransport,
                            const char * pucURL )
{
    SocketTransportStatus_t xStatus;
    TickType_t xRecvTimeout = sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS;
    TickType_t xSendTimeout = sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS;

    LogInfo( ( "Connecting socket to %s", pucURL ) );
    xStatus = Azure_Socket_Connect( pxHTTPTransport->pxNetworkContext, pucURL, 80, xRecvTimeout, xSendTimeout );

    LogInfo( ( " xStatus: %i", xStatus ) );

    configASSERT( xStatus == eSocketTransportSuccess );
}

/**
 * @brief Parses the full ADU file URL into a host (FQDN) and its path.
 *
 * @param xFileUrl ADU file Url to be parsed.
 * @param pucBuffer Buffer to be used for pxHost and pxPath.
 * @param ulBufferSize Size of pucBuffer
 * @param pucHost Where the host part of the url is stored, including a null terminator.
 *               The size of this span is equal to the length of the host address plus the
 *               size of the null-terminator char.
 * @param pulHostLength The length of the content pointed by pucHost.
 * @param pucPath Where the path part of the url is stored.
 * @param pulPathLength The length of the content pointed by pucPath.
 */
static void prvParseAduFileUrl( AzureIoTADUUpdateManifestFileUrl_t xFileUrl,
                                uint8_t * pucBuffer,
                                uint32_t ulBufferSize,
                                uint8_t ** pucHost,
                                uint32_t * pulHostLength,
                                uint8_t ** pucPath,
                                uint32_t * pulPathLength )
{
    configASSERT( ulBufferSize >= xFileUrl.ulUrlLength );

    /* Skipping the http protocol prefix. */
    uint8_t * pucUrl = xFileUrl.pucUrl + sizeof( "http://" ) - 1;
    char * pcPathStart = strstr( ( const char * ) pucUrl, "/" );
    configASSERT( pcPathStart != NULL );

    *pucHost = pucBuffer;
    /* Adding an extra space for the null-terminator. */
    *pulHostLength = pcPathStart - ( char * ) pucUrl + 1;
    *pucPath = pucBuffer + *pulHostLength;

    /* Discouting the size of host and http prefix from ulUrlLength */
    *pulPathLength = xFileUrl.ulUrlLength - ( *pulHostLength - 1 ) - ( sizeof( "http://" ) - 1 );

    ( void ) memcpy( *pucHost, pucUrl, *pulHostLength - 1 );
    ( void ) memset( *pucHost + *pulHostLength - 1, 0, 1 );
    ( void ) memcpy( *pucPath, pcPathStart, *pulPathLength );
}

static AzureIoTResult_t prvDownloadUpdateImageIntoFlash( int32_t ullTimeoutInSec )
{
    AzureIoTResult_t xResult;
    AzureIoTHTTPResult_t xHttpResult;
    AzureIoTHTTP_t xHTTP;
    char * pucOutDataPtr;
    uint32_t ulOutHttpDataBufferLength;
    uint8_t * pucFileUrlHost;
    uint32_t ulFileUrlHostLength;
    uint8_t * pucFileUrlPath;
    uint32_t ulFileUrlPathLength;
    uint64_t ullPreviousTimeout;
    uint64_t ullCurrentTime;

    /*HTTP Connection */
    AzureIoTTransportInterface_t xHTTPTransport;
    NetworkContext_t xHTTPNetworkContext = { 0 };
    SocketTransportParams_t xHTTPSocketTransportParams = { 0 };

    /* Fill in Transport Interface send and receive function pointers. */
    xHTTPTransport.pxNetworkContext = &xHTTPNetworkContext;
    xHTTPTransport.xSend = Azure_Socket_Send;
    xHTTPTransport.xRecv = Azure_Socket_Recv;

    xHTTPNetworkContext.pParams = &xHTTPSocketTransportParams;

    AzureIoTPlatform_Init( &xImage );

    LogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFirmwareDownloadStarted" ) );

    LogInfo( ( "[ADU] Send property update." ) );

    xResult = AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                                &xAzureIoTHubClient,
                                                &xADUDeviceProperties,
                                                &xAzureIoTAduUpdateRequest,
                                                eAzureIoTADUAgentStateDeploymentInProgress,
                                                NULL,
                                                ucScratchBuffer,
                                                sizeof( ucScratchBuffer ),
                                                NULL );

    LogInfo( ( "[ADU] Invoke HTTP Connect Callback." ) );

    prvParseAduFileUrl(
        xAzureIoTAduUpdateRequest.pxFileUrls[ 0 ],
        ucScratchBuffer, sizeof( ucScratchBuffer ),
        &pucFileUrlHost, &ulFileUrlHostLength,
        &pucFileUrlPath, &ulFileUrlPathLength );

    prvConnectHTTP( &xHTTPTransport, ( const char * ) pucFileUrlHost );

    /* Range Check */
    xHttpResult = AzureIoTHTTP_RequestSizeInit( &xHTTP, &xHTTPTransport,
                                                ( const char * ) pucFileUrlHost,
                                                ulFileUrlHostLength - 1, /* minus the null-terminator. */
                                                ( const char * ) pucFileUrlPath,
                                                ulFileUrlPathLength,
                                                ( char * ) ucAduDownloadHeaderBuffer,
                                                sizeof( ucAduDownloadHeaderBuffer ) );

    if( xHttpResult != eAzureIoTHTTPSuccess )
    {
        return eAzureIoTErrorFailed;
    }

    if( ( xImage.ulImageFileSize = AzureIoTHTTP_RequestSize( &xHTTP, ( char * ) ucAduDownloadBuffer,
                                                             sizeof( ucAduDownloadBuffer ) ) ) != -1 )
    {
        LogInfo( ( "[ADU] HTTP Range Request was successful: size %u bytes", ( uint16_t ) xImage.ulImageFileSize ) );
    }
    else
    {
        LogError( ( "[ADU] Error getting the headers. " ) );
        return eAzureIoTErrorFailed;
    }

    LogInfo( ( "[ADU] Send HTTP request." ) );

    ullPreviousTimeout = ullGetUnixTime();

    while( xImage.ulCurrentOffset < xImage.ulImageFileSize )
    {
        ullCurrentTime = ullGetUnixTime();

        if( ullCurrentTime - ullPreviousTimeout > ullTimeoutInSec )
        {
            LogInfo( ( "%u second timeout. Taking a break from downloading image.", ( uint16_t ) ullTimeoutInSec ) );
            LogInfo( ( "Receiving messages from IoT Hub." ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                     sampleazureiotPROCESS_LOOP_TIMEOUT_MS );

            ullPreviousTimeout = ullGetUnixTime();

            if( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionCancel )
            {
                LogInfo( ( "Deployment was cancelled" ) );
                break;
            }
        }

        AzureIoTHTTP_Init( &xHTTP, &xHTTPTransport,
                           ( const char * ) pucFileUrlHost,
                           ulFileUrlHostLength - 1, /* minus the null-terminator. */
                           ( const char * ) pucFileUrlPath,
                           ulFileUrlPathLength,
                           ( char * ) ucAduDownloadHeaderBuffer,
                           sizeof( ucAduDownloadHeaderBuffer ) );

        if( ( xHttpResult = AzureIoTHTTP_Request( &xHTTP, xImage.ulCurrentOffset,
                                                  xImage.ulCurrentOffset + democonfigCHUNK_DOWNLOAD_SIZE - 1,
                                                  ( char * ) ucAduDownloadBuffer,
                                                  sizeof( ucAduDownloadBuffer ),
                                                  &pucOutDataPtr,
                                                  &ulOutHttpDataBufferLength ) ) == eAzureIoTHTTPSuccess )
        {
            /* Write bytes to the flash */
            xResult = AzureIoTPlatform_WriteBlock( &xImage,
                                                   ( uint32_t ) xImage.ulCurrentOffset,
                                                   ( uint8_t * ) pucOutDataPtr,
                                                   ulOutHttpDataBufferLength );

            if( xResult != eAzureIoTSuccess )
            {
                LogError( ( "[ADU] Error writing to flash." ) );
                return eAzureIoTErrorFailed;
            }

            /* Advance the offset */
            xImage.ulCurrentOffset += ( int32_t ) ulOutHttpDataBufferLength;
        }
        else if( xHttpResult == eAzureIoTHTTPNoResponse )
        {
            LogInfo( ( "[ADU] Reconnecting..." ) );
            LogInfo( ( "[ADU] Invoke HTTP Connect Callback." ) );
            prvConnectHTTP( &xHTTPTransport, ( const char * ) pucFileUrlHost );

            if( xResult != eAzureIoTSuccess )
            {
                LogError( ( "[ADU] Failed to reconnect to HTTP server!" ) );
                return eAzureIoTErrorFailed;
            }
        }
        else
        {
            break;
        }
    }

    AzureIoTHTTP_Deinit( &xHTTP );

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvEnableImageAndResetDevice()
{
    AzureIoTResult_t xResult;
    AzureIoTADUClientInstallResult_t xUpdateResults;

    /* Call into platform specific image verification */
    LogInfo( ( "[ADU] Image validated against hash from ADU" ) );

    if( AzureIoTPlatform_VerifyImage(
            &xImage,
            xAzureIoTAduUpdateRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].pucHash,
            xAzureIoTAduUpdateRequest.xUpdateManifest.pxFiles[ 0 ].pxHashes[ 0 ].ulHashLength
            ) != eAzureIoTSuccess )
    {
        LogError( ( "[ADU] File hash from ADU did not match calculated hash" ) );
        return eAzureIoTErrorFailed;
    }

    LogInfo( ( "[ADU] Enable the update image" ) );

    if( AzureIoTPlatform_EnableImage( &xImage ) != eAzureIoTSuccess )
    {
        LogError( ( "[ADU] Image could not be enabled" ) );
        return eAzureIoTErrorFailed;
    }

    /*
     * In a production implementation the application would fill the final lResultCode
     * (and optionally lExtendedResultCode) at the end of the update, and the results
     * of each step as they are processed by the application.
     * This result is then reported to the Azure Device Update service, allowing it
     * to assess if the update succeeded.
     * Optional details of the steps and overall installation results can be provided
     * through pucResultDetails.
     */
    xUpdateResults.lResultCode = 0;
    xUpdateResults.lExtendedResultCode = sampleaduSAMPLE_EXTENDED_RESULT_CODE;
    xUpdateResults.pucResultDetails = sampleaduDEFAULT_RESULT_DETAILS;
    xUpdateResults.ulResultDetailsLength = sizeof( sampleaduDEFAULT_RESULT_DETAILS ) - 1;
    xUpdateResults.ulStepResultsCount =
        xAzureIoTAduUpdateRequest.xUpdateManifest.xInstructions.ulStepsCount;

    /*
     * The order of the step results must match order of the steps
     * in the the update manifest instructions.
     */
    for( int32_t ulStepIndex = 0; ulStepIndex < xUpdateResults.ulStepResultsCount; ulStepIndex++ )
    {
        xUpdateResults.pxStepResults[ ulStepIndex ].ulResultCode = 0;
        xUpdateResults.pxStepResults[ ulStepIndex ].ulExtendedResultCode = sampleaduSAMPLE_EXTENDED_RESULT_CODE;
        xUpdateResults.pxStepResults[ ulStepIndex ].pucResultDetails = sampleaduDEFAULT_RESULT_DETAILS;
        xUpdateResults.pxStepResults[ ulStepIndex ].ulResultDetailsLength = sizeof( sampleaduDEFAULT_RESULT_DETAILS ) - 1;
    }

    LogInfo( ( "[ADU] Send property update." ) );

    xResult = AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                                &xAzureIoTHubClient,
                                                &xADUDeviceProperties,
                                                &xAzureIoTAduUpdateRequest,
                                                eAzureIoTADUAgentStateDeploymentInProgress,
                                                &xUpdateResults,
                                                ucScratchBuffer,
                                                sizeof( ucScratchBuffer ),
                                                NULL );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "[ADU] Failed sending agent state." ) );
        return xResult;
    }

    LogInfo( ( "[ADU] Reset the device" ) );

    if( AzureIoTPlatform_ResetDevice( &xImage ) != eAzureIoTSuccess )
    {
        LogError( ( "[ADU] Failed resetting the device." ) );
        return eAzureIoTErrorFailed;
    }

    /* If a device resets, it will not get here. */
    /* For linux devices, this will mark the device as updated and we will change the version as if */
    /* it did update. */
    LogInfo( ( "[ADU] DEVICE HAS UPDATED" ) );
    xDidDeviceUpdate = true;

    return eAzureIoTSuccess;
}

/* This code is only run on the simulator. Devices will not reach this code since they reboot. */
static AzureIoTResult_t prvSpoofNewVersion( void )
{
    #ifdef democonfigADU_UPDATE_NEW_VERSION
        xADUDeviceProperties.ucCurrentUpdateId = ( const uint8_t * ) democonfigADU_SPOOFED_UPDATE_ID;
        xADUDeviceProperties.ulCurrentUpdateIdLength = strlen( democonfigADU_SPOOFED_UPDATE_ID );
    #else
        LogError( ( "[ADU] New ADU update version for simulator not given." ) );
    #endif
    LogInfo( ( "[ADU] Device Version %.*s",
               ( int16_t ) xADUDeviceProperties.ulCurrentUpdateIdLength, xADUDeviceProperties.ucCurrentUpdateId ) );
    return AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                             &xAzureIoTHubClient,
                                             &xADUDeviceProperties,
                                             NULL,
                                             eAzureIoTADUAgentStateIdle,
                                             NULL,
                                             ucScratchBuffer,
                                             sizeof( ucScratchBuffer ),
                                             NULL );
}


/*-----------------------------------------------------------*/

/**
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub and
 *  function to adhere to the Plug and Play device convention.
 */
static void prvAzureDemoTask( void * pvParameters )
{
    LogInfo( ( "------------------------------------------------------------------------------" ) );
    LogInfo( ( "ADU SAMPLE" ) );
    LogInfo( ( "Version: " democonfigADU_UPDATE_VERSION ) );
    LogInfo( ( "------------------------------------------------------------------------------" ) );

    uint32_t ulScratchBufferLength = 0U;
    /* MQTT Connection */
    NetworkCredentials_t xNetworkCredentials = { 0 };
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };

    AzureIoTResult_t xResult;
    uint32_t ulStatus;
    AzureIoTHubClientOptions_t xHubOptions = { 0 };
    AzureIoTADUClientOptions_t xADUOptions = { 0 };
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
            LogError( ( "Failed on sample_dps_entry!: error code = 0x%08x", ulStatus ) );
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
        xHubOptions.pucModelID = ( const uint8_t * ) AzureIoTADUModelID;
        xHubOptions.ulModelIDLength = AzureIoTADUModelIDLength;

        #ifdef sampleaduPNP_COMPONENTS_LIST_LENGTH
            #if sampleaduPNP_COMPONENTS_LIST_LENGTH > 0
                xHubOptions.pxComponentList = sampleaduPNP_COMPONENTS_LIST;
                xHubOptions.ulComponentListLength = sampleaduPNP_COMPONENTS_LIST_LENGTH;
            #endif /* > 0 */
        #endif /* sampleaduPNP_COMPONENTS_LIST_LENGTH */

        xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                          pucIotHubHostname, pulIothubHostnameLength,
                                          pucIotHubDeviceId, pulIothubDeviceIdLength,
                                          &xHubOptions,
                                          ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                          ullGetUnixTime,
                                          &xTransport );
        configASSERT( xResult == eAzureIoTSuccess );

        /*Init Azure IoT ADU Client Options */
        xResult = AzureIoTADUClient_OptionsInit( &xADUOptions );
        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTADUClient_Init( &xAzureIoTADUClient, &xADUOptions );
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
        LogInfo( ( "Creating an MQTT connection to %s.", pucIotHubHostname ) );

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

        xResult = AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                                    &xAzureIoTHubClient,
                                                    &xADUDeviceProperties,
                                                    NULL,
                                                    eAzureIoTADUAgentStateIdle,
                                                    NULL,
                                                    ucScratchBuffer,
                                                    sizeof( ucScratchBuffer ),
                                                    NULL );
        configASSERT( xResult == eAzureIoTSuccess );

        /* Get property document after initial connection */
        xResult = AzureIoTHubClient_RequestPropertiesAsync( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTSuccess );

        /* Publish messages with QoS1, send and process Keep alive messages. */
        for( ; ; )
        {
            /* Hook for sending Telemetry */
            if( ( ulCreateTelemetry( ucScratchBuffer, sizeof( ucScratchBuffer ), &ulScratchBufferLength ) == 0 ) &&
                ( ulScratchBufferLength > 0 ) )
            {
                xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                           ucScratchBuffer, ulScratchBufferLength,
                                                           NULL, eAzureIoTHubMessageQoS1, NULL );
                configASSERT( xResult == eAzureIoTSuccess );
            }

            /* Hook for sending update to reported properties */
            ulReportedPropertiesUpdateLength = ulCreateReportedPropertiesUpdate( ucReportedPropertiesUpdate, sizeof( ucReportedPropertiesUpdate ) );

            if( ulReportedPropertiesUpdateLength > 0 )
            {
                xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient, ucReportedPropertiesUpdate, ulReportedPropertiesUpdateLength, NULL );
                configASSERT( xResult == eAzureIoTSuccess );
            }

            LogInfo( ( "Attempt to receive publish message from IoT Hub." ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                     sampleazureiotPROCESS_LOOP_TIMEOUT_MS );
            configASSERT( xResult == eAzureIoTSuccess );

            if( xProcessUpdateRequest && !xDidDeviceUpdate )
            {
                if( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionCancel )
                {
                    xResult = AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                                                &xAzureIoTHubClient,
                                                                &xADUDeviceProperties,
                                                                &xAzureIoTAduUpdateRequest,
                                                                eAzureIoTADUAgentStateIdle,
                                                                NULL,
                                                                ucScratchBuffer,
                                                                sizeof( ucScratchBuffer ),
                                                                NULL );

                    xProcessUpdateRequest = false;
                }
                else if( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionApplyDownload )
                {
                    xResult = prvDownloadUpdateImageIntoFlash( sampleazureiotADU_DOWNLOAD_TIMEOUT_SEC );
                    configASSERT( xResult == eAzureIoTSuccess );

                    LogInfo( ( "Checking for ADU twin updates one more time before committing to update." ) );
                    xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                             sampleazureiotPROCESS_LOOP_TIMEOUT_MS );
                    configASSERT( xResult == eAzureIoTSuccess );

                    /* In prvDownloadUpdateImageIntoFlash, we make a call to the _ProcessLoop() function */
                    /* which could bring in a new or cancelled update. */
                    /* Check xProcessUpdateRequest and the action again in case a new version came in that was invalid. */
                    if( xProcessUpdateRequest && ( xAzureIoTAduUpdateRequest.xWorkflow.xAction == eAzureIoTADUActionApplyDownload ) )
                    {
                        xResult = prvEnableImageAndResetDevice();
                        configASSERT( xResult == eAzureIoTSuccess );

                        xResult = prvSpoofNewVersion();
                        configASSERT( xResult == eAzureIoTSuccess );
                    }
                    else
                    {
                        xResult = AzureIoTADUClient_SendAgentState( &xAzureIoTADUClient,
                                                                    &xAzureIoTHubClient,
                                                                    &xADUDeviceProperties,
                                                                    &xAzureIoTAduUpdateRequest,
                                                                    eAzureIoTADUAgentStateIdle,
                                                                    NULL,
                                                                    ucScratchBuffer,
                                                                    sizeof( ucScratchBuffer ),
                                                                    NULL );

                        xProcessUpdateRequest = false;
                    }
                }
                else
                {
                    LogInfo( ( "Unknown action received: %i", xAzureIoTAduUpdateRequest.xWorkflow.xAction ) );
                }
            }

            /* Leave Connection Idle for some time. */
            LogInfo( ( "Keeping Connection Idle..." ) );
            vTaskDelay( sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS );
        }

        xResult = AzureIoTHubClient_UnsubscribeProperties( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTHubClient_UnsubscribeCommand( &xAzureIoTHubClient );
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
        LogInfo( ( "Demo completed successfully." ) );
        LogInfo( ( "Short delay before starting the next iteration.... " ) );
        vTaskDelay( sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS );
    }
}
/*-----------------------------------------------------------*/

#ifdef democonfigENABLE_DPS_SAMPLE

/*-----------------------------------------------------------*/

    static AzureIoTResult_t prvCreateProvisioningPayload( uint8_t * pucBuffer,
                                                          uint32_t ulBufferLength,
                                                          int32_t * plOutBufferLength )
    {
        AzureIoTResult_t xResult;
        AzureIoTJSONWriter_t xWriter;

        if( ( xResult = AzureIoTJSONWriter_Init( &xWriter,
                                                 pucBuffer,
                                                 ulBufferLength ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Error initializing JSON writer: result 0x%08x", ( uint16_t ) xResult ) );
            return xResult;
        }
        else if( ( xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Error appending begin object: result 0x%08x", ( uint16_t ) xResult ) );
            return xResult;
        }
        else if( ( xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter,
                                                                               ( uint8_t * ) sampleazureiotMODEL_ID_STR,
                                                                               sizeof( sampleazureiotMODEL_ID_STR ) - 1,
                                                                               AzureIoTADUModelID,
                                                                               AzureIoTADUModelIDLength ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Error appending property name and string value: result 0x%08x", ( uint16_t ) xResult ) );
            return xResult;
        }
        else if( ( xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter ) ) != eAzureIoTSuccess )
        {
            LogError( ( "Error appending end object: result 0x%08x", ( uint16_t ) xResult ) );
            return xResult;
        }

        *plOutBufferLength = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

        return xResult;
    }

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
        int32_t lOutProvisioningPayloadLength;
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

        xResult = prvCreateProvisioningPayload( ucScratchBuffer,
                                                sizeof( ucScratchBuffer ),
                                                &lOutProvisioningPayloadLength );
        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTProvisioningClient_SetRegistrationPayload( &xAzureIoTProvisioningClient,
                                                                     ( const uint8_t * ) ucScratchBuffer,
                                                                     ( uint32_t ) lOutProvisioningPayloadLength );
        configASSERT( xResult == eAzureIoTSuccess );

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
            LogInfo( ( "Error getting IoT Hub name and Device ID: 0x%08x", ( uint16_t ) xResult ) );
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
        LogInfo( ( "Creating a TLS connection to %s:%u.", pcHostName, ( uint16_t ) port ) );
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
