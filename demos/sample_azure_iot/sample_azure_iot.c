/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo Specific configs. */
#include "demo_config.h"
#include "az_iot_hfsm_sync_adapter.h"

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
#include "transport_tls_socket.h"

/* Crypto helper header. */
#include "crypto.h"
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
#define sampleazureiotMESSAGE                                 "Hello World : %d !"

/**
 * @brief The reported property payload to send to IoT Hub
 */
#define sampleazureiotTWIN_PROPERTY                           "{ \"TwinIterationForCurrentConnection\": \"%d\" }"

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
#define sampleazureiotProvisioning_Registration_TIMEOUT_MS    ( 20U )

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

/**
 * @brief BSP error handler.
 * 
 */
void Error_Handler( void );
/*-----------------------------------------------------------*/

/* Define buffer for IoT Hub info.  */
#ifdef democonfigENABLE_DPS_SAMPLE
    static uint8_t ucSampleIotHubHostname[ 128 ];
    static uint8_t ucSampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
#endif /* democonfigENABLE_DPS_SAMPLE */

static uint8_t ucPropertyBuffer[ 32 ];
static uint8_t ucScratchBuffer[ 128 ];

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    TlsTransportParams_t * pParams;
};

static AzureIoTHubClient_t xAzureIoTHubClient;
/*-----------------------------------------------------------*/

NetworkCredentials_t xNetworkCredentials = { 0 };

#ifdef democonfigENABLE_DPS_SAMPLE
    static uint8_t * pucIotHubHostname = NULL;
    static uint8_t * pucIotHubDeviceId = NULL;
    static uint32_t ulIothubHostnameLength = 0;
    static uint32_t ulIothubDeviceIdLength = 0;
#else
    static uint8_t * pucIotHubHostname = ( uint8_t * ) democonfigHOSTNAME;
    static uint8_t * pucIotHubDeviceId = ( uint8_t * ) democonfigDEVICE_ID;
    static uint32_t ulIothubHostnameLength = sizeof( democonfigHOSTNAME ) - 1;
    static uint32_t ulIothubDeviceIdLength = sizeof( democonfigDEVICE_ID ) - 1;
#endif /* democonfigENABLE_DPS_SAMPLE */

#ifdef democonfigDEVICE_SYMMETRIC_KEY
    static uint8_t * pucSymmetricKey = democonfigDEVICE_SYMMETRIC_KEY;
    static uint32_t ulSymmetricKeyLength = sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1;
#endif

#ifdef democonfigENABLE_DPS_SAMPLE

/**
 * @brief Gets the IoT Hub endpoint and deviceId from Provisioning service.
 *   This function will block for Provisioning service for result or return failure.
 */
az_iot_hfsm_event_data_error prvDeviceProvisioningRun();

#endif /* democonfigENABLE_DPS_SAMPLE */

az_iot_hfsm_event_data_error prvIoTHubRun();

/**
 * @brief The task used to demonstrate the MQTT API.
 *
 * @param[in] pvParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 */
static void prvAzureDemoTask( void * pvParameters );

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
               pxMessage->pvMessagePayload ) );
}
/*-----------------------------------------------------------*/

/**
 * @brief Direct method message callback handler
 */
static void prvHandleDirectMethod( AzureIoTHubClientMethodRequest_t * pxMessage,
                                   void * pvContext )
{
    LogInfo( ( "Method payload : %.*s \r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );

    AzureIoTHubClient_t * xHandle = ( AzureIoTHubClient_t * ) pvContext;

    if( AzureIoTHubClient_SendMethodResponse( xHandle, pxMessage, 200,
                                              NULL, 0 ) != eAzureIoTHubClientSuccess )
    {
        LogInfo( ( "Error sending method response\r\n" ) );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Twin mesage callback handler
 */
static void prvHandleDeviceTwinMessage( AzureIoTHubClientTwinResponse_t * pxMessage,
                                        void * pvContext )
{
    ( void ) pvContext;

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubTwinGetMessage:
            LogInfo( ( "Device twin document GET received" ) );
            break;

        case eAzureIoTHubTwinReportedResponseMessage:
            LogInfo( ( "Device twin reported property response received" ) );
            break;

        case eAzureIoTHubTwinDesiredPropertyMessage:
            LogInfo( ( "Device twin desired property received" ) );
            break;

        default:
            LogError( ( "Unknown twin message" ) );
    }

    LogInfo( ( "Twin document payload : %.*s \r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );
}
/*-----------------------------------------------------------*/

/**
 * @brief Setup transport credentials.
 */
void prvSetupNetworkCredentials( bool use_secondary )
{
    xNetworkCredentials.xDisableSni = pdFALSE;
    /* Set the credentials for establishing a TLS connection. */
    xNetworkCredentials.pucRootCa = ( const unsigned char * ) democonfigROOT_CA_PEM;
    xNetworkCredentials.xRootCaSize = sizeof( democonfigROOT_CA_PEM );
    
    #ifdef democonfigCLIENT_CERTIFICATE_PEM
        #ifdef democonfigSECONDARY_CREDENTIALS
        if (!use_secondary)
        {
        #endif // democonfigSECONDARY_CREDENTIALS
            xNetworkCredentials.pucClientCert = ( const unsigned char * ) democonfigCLIENT_CERTIFICATE_PEM;
            xNetworkCredentials.xClientCertSize = sizeof( democonfigCLIENT_CERTIFICATE_PEM );
            xNetworkCredentials.pucPrivateKey = ( const unsigned char * ) democonfigCLIENT_PRIVATE_KEY_PEM;
            xNetworkCredentials.xPrivateKeySize = sizeof( democonfigCLIENT_PRIVATE_KEY_PEM );
        #ifdef democonfigSECONDARY_CREDENTIALS
        }
        else
        {
            xNetworkCredentials.pucClientCert = ( const unsigned char * ) democonfigSECONDARY_CLIENT_CERTIFICATE_PEM;
            xNetworkCredentials.xClientCertSize = sizeof( democonfigSECONDARY_CLIENT_CERTIFICATE_PEM );
            xNetworkCredentials.pucPrivateKey = ( const unsigned char * ) democonfigSECONDARY_CLIENT_PRIVATE_KEY_PEM;
            xNetworkCredentials.xPrivateKeySize = sizeof( democonfigSECONDARY_CLIENT_PRIVATE_KEY_PEM );
        }
        #endif // democonfigSECONDARY_CREDENTIALS
    #endif // democonfigCLIENT_CERTIFICATE_PEM

    #ifdef democonfigDEVICE_SYMMETRIC_KEY
        #ifdef democonfigSECONDARY_CREDENTIALS
        if (!use_secondary)
        {
        #endif // democonfigSECONDARY_CREDENTIALS
            pucSymmetricKey = democonfigDEVICE_SYMMETRIC_KEY;
            ulSymmetricKeyLength = sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1;
        #ifdef democonfigSECONDARY_CREDENTIALS
        }
        else
        {
            pucSymmetricKey = democonfigSECONDARY_DEVICE_SYMMETRIC_KEY;
            ulSymmetricKeyLength = sizeof( democonfigSECONDARY_DEVICE_SYMMETRIC_KEY ) - 1;
        }
        #endif // democonfigSECONDARY_CREDENTIALS
    #endif
}
/*-----------------------------------------------------------*/

// AzureIoT HFSM Sync PAL implementation

/**
 * @brief Configure credentials
 * 
 * @param use_secondary true if the secondary credentials should be used.
 */
void az_iot_hfsm_sync_adapter_pal_set_credentials( bool use_secondary )
{
    prvSetupNetworkCredentials( use_secondary );
}

/**
 * @brief Run Device Provisioning syncrhonously.
 * 
 * @return az_iot_hfsm_event_data_error 
 */
az_iot_hfsm_event_data_error az_iot_hfsm_sync_adapter_pal_run_provisioning( )
{
    return prvDeviceProvisioningRun();
}

/**
 * @brief Run IoT Hub operations synchronously.
 * 
 * @return az_iot_hfsm_event_data_error 
 */
az_iot_hfsm_event_data_error az_iot_hfsm_sync_adapter_pal_run_hub( )
{
    return prvIoTHubRun();
}

/**
 * @brief Sleep
 * 
 * @param milliseconds time the task should be blocked.
 */
void az_iot_hfsm_sync_adapter_sleep( int32_t milliseconds )
{
    vTaskDelay (pdMS_TO_TICKS(milliseconds));
}

/**
 * @brief Get the number of milliseconds elapsed.
 * 
 * @return uint64_t the number of milliseconds.
 */
uint64_t az_hfsm_pal_timer_get_milliseconds()
{
    return ullGetUnixTime() * 1000;
}

/**
 * @brief Critical error. This function should not return.
 * 
 * @param me The calling HFSM object.
 */
void az_iot_hfsm_pal_critical_error(az_hfsm* caller)
{
    (void) caller;
    Error_Handler();
}

int32_t az_iot_hfsm_pal_get_random_jitter_msec(az_hfsm* hfsm)
{
    return configRAND32() % AZ_IOT_HFSM_MAX_RETRY_JITTER_MSEC;
}

az_iot_hfsm_event_data_error_type error_adapter_tlstransportstatus(TlsTransportStatus_t status)
{
    az_iot_hfsm_event_data_error_type ret;

    switch (status)
    {
        case eTLSTransportSuccess:
            ret = AZ_IOT_OK;
            break;
  
        case eTLSTransportConnectFailure:
            ret = AZ_IOT_ERROR_TYPE_NETWORK;
            break;
  
        case eTLSTransportInvalidParameter:
        case eTLSTransportInSufficientMemory:
        case eTLSTransportInvalidCredentials:
        case eTLSTransportHandshakeFailed:
        case eTLSTransportInternalError:
        default:
            ret = AZ_IOT_ERROR_TYPE_SECURITY;
    }

    return ret;
}

az_iot_hfsm_event_data_error_type error_adapter_azureiotprovisioningclientresult(AzureIoTProvisioningClientResult_t status)
{
    az_iot_hfsm_event_data_error_type ret;

    switch (status)
    {
        case eAzureIoTProvisioningSuccess:
          ret = AZ_IOT_OK;
          break;
        
        case eAzureIoTProvisioningServerError:
          ret = AZ_IOT_ERROR_TYPE_SERVICE;
          break;

        case eAzureIoTProvisioningSubscribeFailed:
        case eAzureIoTProvisioningPublishFailed:
          ret = AZ_IOT_ERROR_TYPE_NETWORK;
          break;

        case eAzureIoTProvisioningInvalidArgument:
        case eAzureIoTProvisioningPending:
        case eAzureIoTProvisioningOutOfMemory:
        case eAzureIoTProvisioningInitFailed:
        case eAzureIoTProvisioningTokenGenerationFailed:
        case eAzureIoTProvisioningFailed:
        default:
          ret = AZ_IOT_ERROR_TYPE_SECURITY;
    }

    return ret;
}

az_iot_hfsm_event_data_error_type error_adapter_azureiothubclientresult(AzureIoTHubClientResult_t status)
{
    az_iot_hfsm_event_data_error_type ret;

    switch (status)
    {
        case eAzureIoTHubClientSuccess:
          ret = AZ_IOT_OK;
          break;
        
        case eAzureIoTHubClientSubackWaitTimeout:
        case eAzureIoTHubClientTopicNotSubscribed:
        case eAzureIoTHubClientPublishFailed:
        case eAzureIoTHubClientSubscribeFailed:
        case eAzureIoTHubClientUnsubscribeFailed:
        case eAzureIoTHubClientTopicNoMatch:
          ret = AZ_IOT_ERROR_TYPE_NETWORK;
          break;

        case eAzureIoTHubClientInvalidArgument:
        case eAzureIoTHubClientPending:
        case eAzureIoTHubClientOutOfMemory:
        case eAzureIoTHubClientInitFailed:
        case eAzureIoTHubClientFailed:
        default:
          ret = AZ_IOT_ERROR_TYPE_SECURITY;
    }

    return ret;
}

/*-----------------------------------------------------------*/
/**
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub.
 */
static void prvAzureDemoTask( void * pvParameters )
{
    ( void ) pvParameters;

    /* Initialize Azure IoT Middleware.  */
    configASSERT( AzureIoT_Init() == eAzureIoTSuccess );

    configASSERT ( !az_iot_hfsm_sync_adapter_sync_initialize() );

    // This function will never exit.
    az_iot_hfsm_sync_adapter_sync_do_work();
    configASSERT(0);
}

/*-----------------------------------------------------------*/

#ifdef democonfigENABLE_DPS_SAMPLE

/**
* @brief Get IoT Hub endpoint and device Id info, when Provisioning service is used.
*   This function will block for Provisioning service for result or return failure.
*/
az_iot_hfsm_event_data_error prvDeviceProvisioningRun()
{
    az_iot_hfsm_event_data_error xProvisioningStatus = { AZ_IOT_OK, AZ_IOT_STATUS_UNKNOWN };
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    TlsTransportStatus_t xTransportStatus;
    AzureIoTTransportInterface_t xTransport;
    AzureIoTProvisioningClientResult_t xResult;

    uint32_t ucSamplepIothubHostnameLength = sizeof( ucSampleIotHubHostname );
    uint32_t ucSamplepIothubDeviceIdLength = sizeof( ucSampleIotHubDeviceId );

    /* Set the pParams member of the network context with desired transport. */
    xNetworkContext.pParams = &xTlsTransportParams;

    xTransportStatus = prvConnectToServerWithBackoffRetries( democonfigENDPOINT, democonfigIOTHUB_PORT,
                                                        &xNetworkCredentials, &xNetworkContext );

    xProvisioningStatus.type = error_adapter_tlstransportstatus(xTransportStatus);

    if (xProvisioningStatus.type == AZ_IOT_OK)
    {
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

        xProvisioningStatus.type = error_adapter_azureiotprovisioningclientresult(xResult);
    }
    
    #ifdef democonfigDEVICE_SYMMETRIC_KEY
    if (xProvisioningStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoTProvisioningClient_SetSymmetricKey( &xAzureIoTProvisioningClient,
                                                            ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                            sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                            Crypto_HMAC );
        
        xProvisioningStatus.type = error_adapter_azureiotprovisioningclientresult(xResult);
    }
    #endif // democonfigDEVICE_SYMMETRIC_KEY

    if (xProvisioningStatus.type == AZ_IOT_OK)
    {
        do
        {
            xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                        sampleazureiotProvisioning_Registration_TIMEOUT_MS );
        } while( xResult == eAzureIoTProvisioningPending );

        xProvisioningStatus.type = error_adapter_azureiotprovisioningclientresult(xResult);
    }

    if (xProvisioningStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoTProvisioningClient_GetDeviceAndHub( &xAzureIoTProvisioningClient,
                                                            ucSampleIotHubHostname, &ucSamplepIothubHostnameLength,
                                                            ucSampleIotHubDeviceId, &ucSamplepIothubDeviceIdLength );

        xProvisioningStatus.type = error_adapter_azureiotprovisioningclientresult(xResult);
    }

    if (xProvisioningStatus.type == AZ_IOT_ERROR_TYPE_SERVICE)
    {
        uint32_t extended_status;
        if (AzureIoTProvisioningClient_GetExtendedCode(
            &xAzureIoTProvisioningClient, &extended_status) == eAzureIoTProvisioningSuccess)
        {
            xProvisioningStatus.iot_status = (az_iot_status)(extended_status / 1000);
        }
    }

    AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

    /* Close the network connection.  */
    TLS_Socket_Disconnect( &xNetworkContext );

    pucIotHubHostname = ucSampleIotHubHostname;
    ulIothubHostnameLength = ucSamplepIothubHostnameLength;
    pucIotHubDeviceId = ucSampleIotHubDeviceId;
    ulIothubDeviceIdLength = ucSamplepIothubDeviceIdLength;

    return xProvisioningStatus;
}

#endif // democonfigENABLE_DPS_SAMPLE
/*-----------------------------------------------------------*/

/**
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub.
 */
az_iot_hfsm_event_data_error prvIoTHubRun( )
{
    
    az_iot_hfsm_event_data_error xHubStatus = { AZ_IOT_OK, AZ_IOT_STATUS_UNKNOWN };
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    xNetworkContext.pParams = &xTlsTransportParams;

    AzureIoTHubClientResult_t xResult;
    TlsTransportStatus_t xTransportStatus;
    AzureIoTHubClientOptions_t xHubOptions = { 0 };
    AzureIoTMessageProperties_t xPropertyBag;
    bool xSessionPresent;
    int lPublishCount = 0;
    uint32_t ulScratchBufferLength = 0U;
    const int lMaxPublishCount = 5;

    /* Attempt to establish TLS session with IoT Hub. If connection fails,
        * retry after a timeout. Timeout value will be exponentially increased
        * until  the maximum number of attempts are reached or the maximum timeout
        * value is reached. The function returns a failure status if the TCP
        * connection cannot be established to the IoT Hub after the configured
        * number of attempts. */
    xTransportStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname,
                                                        democonfigIOTHUB_PORT,
                                                        &xNetworkCredentials, &xNetworkContext );

    xHubStatus.type = error_adapter_tlstransportstatus(xTransportStatus);

    if (xHubStatus.type == AZ_IOT_OK)
    {
        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_Socket_Send;
        xTransport.xRecv = TLS_Socket_Recv;

        /* Init IoT Hub option */
        xResult = AzureIoTHubClient_OptionsInit( &xHubOptions );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    if (xHubStatus.type == AZ_IOT_OK)
    {
        xHubOptions.pucModuleID = ( const uint8_t * ) democonfigMODULE_ID;
        xHubOptions.ulModuleIDLength = sizeof( democonfigMODULE_ID ) - 1;

        xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                            pucIotHubHostname, ulIothubHostnameLength,
                                            pucIotHubDeviceId, ulIothubDeviceIdLength,
                                            &xHubOptions,
                                            ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                            ullGetUnixTime,
                                            &xTransport );

        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }
    

    #ifdef democonfigDEVICE_SYMMETRIC_KEY
    if (xHubStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoTHubClient_SetSymmetricKey( &xAzureIoTHubClient,
                                                        pucSymmetricKey,
                                                        ulSymmetricKeyLength,
                                                        Crypto_HMAC );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }
    #endif /* democonfigDEVICE_SYMMETRIC_KEY */


    if (xHubStatus.type == AZ_IOT_OK)
    {
        /* Sends an MQTT Connect packet over the already established TLS connection,
            * and waits for connection acknowledgment (CONNACK) packet. */
        LogInfo( ( "Creating an MQTT connection to %s.\r\n", pucIotHubHostname ) );

        xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                                false, &xSessionPresent,
                                                sampleazureiotCONNACK_RECV_TIMEOUT_MS );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    if (xHubStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xAzureIoTHubClient, prvHandleCloudMessage,
                                                                    &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    if (xHubStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoTHubClient_SubscribeDirectMethod( &xAzureIoTHubClient, prvHandleDirectMethod,
                                                        &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    if (xHubStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoTHubClient_SubscribeDeviceTwin( &xAzureIoTHubClient, prvHandleDeviceTwinMessage,
                                                            &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    if (xHubStatus.type == AZ_IOT_OK)
    {
        /* Get twin document after initial connection */
        xResult = AzureIoTHubClient_GetDeviceTwin( &xAzureIoTHubClient );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    if (xHubStatus.type == AZ_IOT_OK)
    {
        /* Create a bag of properties for the telemetry */
        xResult = AzureIoT_MessagePropertiesInit( &xPropertyBag, ucPropertyBuffer, 0, sizeof( xPropertyBag ) );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    if (xHubStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoT_MessagePropertiesAppend( &xPropertyBag, ( uint8_t * ) "name", sizeof( "name" ) - 1,
                                                    ( uint8_t * ) "value", sizeof( "value" ) - 1 );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }


    if (xHubStatus.type == AZ_IOT_OK)
    {
        /* Publish messages with QoS1, send and process Keep alive messages. */
        for( lPublishCount = 0; lPublishCount < lMaxPublishCount; lPublishCount++ )
        {
            ulScratchBufferLength = snprintf( ( char * ) ucScratchBuffer, sizeof( ucScratchBuffer ),
                                                sampleazureiotMESSAGE, lPublishCount );
            xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                        ucScratchBuffer, ulScratchBufferLength,
                                                        &xPropertyBag, eAzureIoTHubMessageQoS1, NULL );
            xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
            if (xHubStatus.type != AZ_IOT_OK)
            {
                break;
            }

            LogInfo( ( "Attempt to receive publish message from IoT Hub.\r\n" ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                        sampleazureiotPROCESS_LOOP_TIMEOUT_MS );

            xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
            if (xHubStatus.type != AZ_IOT_OK)
            {
                break;
            }            

            if( lPublishCount % 2 == 0 )
            {
                /* Send reported property every other cycle */
                ulScratchBufferLength = snprintf( ( char * ) ucScratchBuffer, sizeof( ucScratchBuffer ),
                                                    sampleazureiotTWIN_PROPERTY, lPublishCount/2 + 1 );
                xResult = AzureIoTHubClient_SendDeviceTwinReported( &xAzureIoTHubClient,
                                                                    ucScratchBuffer, ulScratchBufferLength,
                                                                    NULL );
                xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
                if (xHubStatus.type != AZ_IOT_OK)
                {
                    break;
                }                
            }

            /* Leave Connection Idle for some time. */
            LogInfo( ( "Keeping Connection Idle...\r\n\r\n" ) );
            vTaskDelay( sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS );
        }
    }

    if (xHubStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoTHubClient_UnsubscribeDeviceTwin( &xAzureIoTHubClient );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    if (xHubStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoTHubClient_UnsubscribeDirectMethod( &xAzureIoTHubClient );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    if (xHubStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( &xAzureIoTHubClient );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    /* Send an MQTT Disconnect packet over the already connected TLS over
        * TCP connection. There is no corresponding response for the disconnect
        * packet. After sending disconnect, client must close the network
        * connection. */
    if (xHubStatus.type == AZ_IOT_OK)
    {
        xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
        xHubStatus.type = error_adapter_azureiothubclientresult(xResult);
    }

    /* Close the network connection.  */
    TLS_Socket_Disconnect( &xNetworkContext );

    return xHubStatus;
}

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

        if( xNetworkStatus == eTLSTransportConnectFailure )
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
