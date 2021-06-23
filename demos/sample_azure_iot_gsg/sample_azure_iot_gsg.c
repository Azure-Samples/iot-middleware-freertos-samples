/*REMOVE */

/*
 * uncrustify -c ./libs/azure-iot-middleware-freertos/uncrustify.cfg --no-backup --replace demos/sample_azure_iot_pnp/sample_azure_iot_pnp.c
 */

/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/* Standard includes. */
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"
#include "azure_iot_hub_client_properties.h"

/* Azure JSON includes */
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

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
#define sampleazureiotRETRY_MAX_ATTEMPTS                  ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS          ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define sampleazureiotRETRY_BACKOFF_BASE_MS               ( 500U )

/**
 * @brief Timeout for receiving CONNACK packet in milliseconds.
 */
#define sampleazureiotCONNACK_RECV_TIMEOUT_MS             ( 10 * 1000U )

/**
 * @brief The model id for this sample
 *
 */
#define sampleazureiotMODEL_ID                            "dtmi:azurertos:devkit:gsg;2"

/**
 * @brief Date-time to use for the model id
 */
#define sampleazureiotDATE_TIME_FORMAT                    "%Y-%m-%dT%H:%M:%S.000Z"

/**
 * @brief Telemetry values
 */
#define sampleazureiotTELEMETRY_NAME                      "temperature"

/**
 * @brief Property Values
 */
#define sampleazureiotPROPERTY_STATUS_SUCCESS             200
#define sampleazureiotPROPERTY_SUCCESS                    "success"
#define sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT    "targetTemperature"
#define sampleazureiotPROPERTY_MAX_TEMPERATURE_TEXT       "maxTempSinceLastReboot"
static uint8_t ucPropertyPayloadBuffer[ 256 ];

/**
 * @brief Command values
 */
#define sampleazureiotCOMMAND_MAX_MIN_REPORT    "getMaxMinReport"
#define sampleazureiotCOMMAND_SINCE             "since"
#define sampleazureiotCOMMAND_MAX_TEMP          "maxTemp"
#define sampleazureiotCOMMAND_MIN_TEMP          "minTemp"
#define sampleazureiotCOMMAND_AV_TEMP           "avgTemp"
#define sampleazureiotCOMMAND_START_TIME        "startTime"
#define sampleazureiotCOMMAND_END_TIME          "endTime"
#define sampleazureiotCOMMAND_EMPTY_PAYLOAD     "{}"

#define TELEMETRY_TEMPERATURE       "temperature"
#define TELEMETRY_INTERVAL_PROPERTY "telemetryInterval"
#define LED_STATE_PROPERTY          "ledState"
#define SET_LED_STATE_COMMAND       "setLedState"

//static uint8_t ucCommandPayloadBuffer[ 256 ];
//static uint8_t ucCommandStartTimeValueBuffer[ 32 ];
//static uint8_t ucCommandEndTimeValueBuffer[ 32 ];


/**
 *@brief The Telemetry message published in this example.
 */
#define sampleazureiotMESSAGE                        "{\"" sampleazureiotTELEMETRY_NAME "\":%0.2f}"

/**
 * @brief The reported property payload to send to IoT Hub
 */
#define sampleazureiotPROPERTY_MESSAGE               "{ \"PropertyIterationForCurrentConnection\": \"%u\" }"

/**
 * @brief Device values
 */
#define sampleazureiotDEFAULT_START_TEMP_COUNT       1
#define sampleazureiotDEFAULT_START_TEMP_CELSIUS     22.0
#define sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS    2
static double xDeviceCurrentTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
//static double xDeviceMaximumTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
//static double xDeviceMinimumTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
//static double xDeviceTemperatureSummation = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
//static uint32_t ulDeviceTemperatureCount = sampleazureiotDEFAULT_START_TEMP_COUNT;
//static double xDeviceAverageTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;

/**
 * @brief The payload to send to the Device Provisioning Service
 */
#define sampleazureiotPROVISIONING_PAYLOAD                    "{\"modelId\":\"" sampleazureiotMODEL_ID "\"}"

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

/* Define buffer for IoT Hub info.  */
#ifdef democonfigENABLE_DPS_SAMPLE
    static uint8_t ucSampleIotHubHostname[ 128 ];
    static uint8_t ucSampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
#endif /* democonfigENABLE_DPS_SAMPLE */

static uint8_t ucScratchBuffer[ 128 ];

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    TlsTransportParams_t * pParams;
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
 * @brief Generate max min payload.
 */
// int32_t prvInvokeMaxMinCommand( const uint8_t * pucPayload,
//                                 uint32_t ulPayloadLength )
// {
//     AzureIoTResult_t xResult;
//     AzureIoTJSONReader_t xReader;
//     uint32_t ulSinceTimeLength;
//     time_t xRawTime;
//     struct tm * pxTimeInfo;
//     size_t xEndTimeLength;

//     /* Get the start time */
//     xResult = AzureIoTJSONReader_Init( &xReader, pucPayload, ulPayloadLength );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONReader_NextToken( &xReader );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONReader_TokenIsTextEqual( &xReader, sampleazureiotCOMMAND_SINCE, strlen( sampleazureiotCOMMAND_SINCE ) );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONReader_NextToken( &xReader );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONReader_GetTokenString( &xReader,
//                                                  ucCommandStartTimeValueBuffer,
//                                                  sizeof( ucCommandStartTimeValueBuffer ),
//                                                  &ulSinceTimeLength );
//     configASSERT( xResult == eAzureIoTSuccess );


//     /* Get the current time as a string. */
//     time( &xRawTime );
//     pxTimeInfo = localtime( &xRawTime );
//     xEndTimeLength = strftime(
//         ucCommandEndTimeValueBuffer,
//         sizeof( ucCommandEndTimeValueBuffer ),
//         sampleazureiotDATE_TIME_FORMAT,
//         pxTimeInfo );

//     AzureIoTJSONWriter_t xWriter;

//     xResult = AzureIoTJSONWriter_Init( &xWriter, ucCommandPayloadBuffer, sizeof( ucCommandPayloadBuffer ) );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, sampleazureiotCOMMAND_MAX_TEMP, strlen( sampleazureiotCOMMAND_MAX_TEMP ), xDeviceMaximumTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, sampleazureiotCOMMAND_MIN_TEMP, strlen( sampleazureiotCOMMAND_MIN_TEMP ), xDeviceMinimumTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONWriter_AppendPropertyWithDoubleValue( &xWriter, sampleazureiotCOMMAND_AV_TEMP, strlen( sampleazureiotCOMMAND_AV_TEMP ), xDeviceAverageTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, sampleazureiotCOMMAND_START_TIME, strlen( sampleazureiotCOMMAND_START_TIME ), ucCommandStartTimeValueBuffer, ulSinceTimeLength );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONWriter_AppendPropertyWithStringValue( &xWriter, sampleazureiotCOMMAND_END_TIME, strlen( sampleazureiotCOMMAND_END_TIME ), ucCommandEndTimeValueBuffer, xEndTimeLength );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
//     configASSERT( xResult == eAzureIoTSuccess );

//     return AzureIoTJSONWriter_GetBytesUsed( &xWriter );
// }

/**
 * @brief Cloud message callback handler
 */
// static void prvHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
//                                    void * pvContext )
// {
//     ( void ) pvContext;

//     LogInfo( ( "Cloud message payload : %.*s \r\n",
//                pxMessage->ulPayloadLength,
//                pxMessage->pvMessagePayload ) );
// }
/*-----------------------------------------------------------*/

/**
 * @brief Command message callback handler
 */
static void prvHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                              void * pvContext )
{
    AzureIoTHubClient_t * xHandle = ( AzureIoTHubClient_t * ) pvContext;

    LogInfo( ( "Command payload : %.*s \r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );

//    int32_t ulCommandPayloadLength;


    // if( strncmp( sampleazureiotCOMMAND_MAX_MIN_REPORT, pxMessage->pucCommandName, strlen( sampleazureiotCOMMAND_MAX_MIN_REPORT ) ) == 0 )
    // {
    //     /* Is for max min report */
    //     ulCommandPayloadLength = prvInvokeMaxMinCommand( pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );

    //     if( ulCommandPayloadLength > 0 )
    //     {
    //         if( AzureIoTHubClient_SendCommandResponse( xHandle, pxMessage, 200,
    //                                                    ucCommandPayloadBuffer,
    //                                                    ulCommandPayloadLength ) != eAzureIoTSuccess )
    //         {
    //             LogInfo( ( "Error sending command response" ) );
    //         }
    //     }
    //     else
    //     {
    //         LogInfo( ( "Error generating command payload" ) );
    //     }
    // }
    
    if ( strncmp( SET_LED_STATE_COMMAND, ( const char * )pxMessage->pucCommandName, strlen(SET_LED_STATE_COMMAND ) ) == 0 )
    {
        // process LED thing here
        LogInfo( ( "Received SetLedState" ) );
    }
    else
    {
        /* Not for max min report (not for this device) */
        LogInfo( ( "Received command is not for this device" ) );

        if( AzureIoTHubClient_SendCommandResponse( xHandle, pxMessage, 404,
                                                   ( uint8_t * ) sampleazureiotCOMMAND_EMPTY_PAYLOAD,
                                                   sizeof( sampleazureiotCOMMAND_EMPTY_PAYLOAD ) - 1 ) != eAzureIoTSuccess )
        {
            LogError( ( "Error sending command response" ) );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Properties callback handler
 */
static AzureIoTResult_t prvProcessProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                              AzureIoTHubClientPropertyType_t xPropertyType,
                                              uint32_t * ulOutVersion )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONReader_t xReader;
    AzureIoTJSONTokenType_t xTokenType;
    const uint8_t * pucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;
    int32_t ulTelemetryInterval;

//    *xTelemetryInterval = 0.0;

    xResult = AzureIoTJSONReader_Init( &xReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTHubClientProperties_GetPropertiesVersion( &xAzureIoTHubClient, &xReader, pxMessage->xMessageType, ulOutVersion );

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
            xResult = AzureIoTJSONReader_TokenType( &xReader, &xTokenType );
            configASSERT( xResult == eAzureIoTSuccess );

            if( AzureIoTJSONReader_TokenIsTextEqual( &xReader,
                                                     ( uint8_t * ) TELEMETRY_INTERVAL_PROPERTY,
                                                     sizeof( TELEMETRY_INTERVAL_PROPERTY ) - 1 ) )
            {
                xResult = AzureIoTJSONReader_NextToken( &xReader );
                configASSERT( xResult == eAzureIoTSuccess );

                xResult = AzureIoTJSONReader_TokenType( &xReader, &xTokenType );
                configASSERT( xResult == eAzureIoTSuccess );

                if( xTokenType != eAzureIoTJSONTokenNUMBER )
                {
                    LogError( ( "Invalid value for telemetry interval" ) );
                    xResult = eAzureIoTErrorFailed;
                    break;
                }

                /* Get telemetry interval */
                xResult = AzureIoTJSONReader_GetTokenInt32( &xReader, &ulTelemetryInterval );
                configASSERT( xResult == eAzureIoTSuccess );

                xResult = AzureIoTJSONReader_NextToken( &xReader );
                configASSERT( xResult == eAzureIoTSuccess );

                // TODO: update the telemetry interval here somehow
            }
            else
            {
                xResult = AzureIoTJSONReader_NextToken( &xReader );
                configASSERT( xResult == eAzureIoTSuccess );

                xResult = AzureIoTJSONReader_SkipChildren( &xReader );
                configASSERT( xResult == eAzureIoTSuccess );

                xResult = AzureIoTJSONReader_NextToken( &xReader );
                configASSERT( xResult == eAzureIoTSuccess );
            }

        //     if( AzureIoTJSONReader_TokenIsTextEqual( &xReader,
        //                                              sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT,
        //                                              strlen( sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT ) ) )
        //     {
        //         xResult = AzureIoTJSONReader_NextToken( &xReader );
        //         configASSERT( xResult == eAzureIoTSuccess );

        //         xResult = AzureIoTJSONReader_TokenType( &xReader, &xTokenType );
        //         configASSERT( xResult == eAzureIoTSuccess );

        //         if( xTokenType != eAzureIoTJSONTokenNUMBER )
        //         {
        //             LogInfo( ( "Invalid value for target temperature reached" ) );
        //             xResult = eAzureIoTErrorFailed;
        //             break;
        //         }

        //         /* Get desired temperature */
        //         xResult = AzureIoTJSONReader_GetTokenDouble( &xReader, xOutTemperature );
        //         configASSERT( xResult == eAzureIoTSuccess );

        //         xResult = AzureIoTJSONReader_NextToken( &xReader );
        //         configASSERT( xResult == eAzureIoTSuccess );
        //     }
        //     else
        //     {
        //         xResult = AzureIoTJSONReader_NextToken( &xReader );
        //         configASSERT( xResult == eAzureIoTSuccess );

        //         xResult = AzureIoTJSONReader_SkipChildren( &xReader );
        //         configASSERT( xResult == eAzureIoTSuccess );

        //         xResult = AzureIoTJSONReader_NextToken( &xReader );
        //         configASSERT( xResult == eAzureIoTSuccess );
        //     }
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
 * @brief Update local device temperature values based on new requested temperature.
 */
// static void prvUpdateLocalProperties( double xNewTemperatureValue,
//                                       uint32_t ulPropertyVersion,
//                                       bool * pxOutMaxTempChanged )
// {
//     *pxOutMaxTempChanged = false;
//     xDeviceCurrentTemperature = xNewTemperatureValue;

//     /* Update maximum or minimum temperatures. */
//     if( xDeviceCurrentTemperature > xDeviceMaximumTemperature )
//     {
//         xDeviceMaximumTemperature = xDeviceCurrentTemperature;
//         *pxOutMaxTempChanged = true;
//     }
//     else if( xDeviceCurrentTemperature < xDeviceMinimumTemperature )
//     {
//         xDeviceMinimumTemperature = xDeviceCurrentTemperature;
//     }

//     /* Calculate the new average temperature. */
//     ulDeviceTemperatureCount++;
//     xDeviceTemperatureSummation += xDeviceCurrentTemperature;
//     xDeviceAverageTemperature = xDeviceTemperatureSummation / ulDeviceTemperatureCount;

//     LogDebug( ( "Client updated desired temperature variables locally." ) );
//     LogDebug( ( "Current Temperature: %2f", xDeviceCurrentTemperature ) );
//     LogDebug( ( "Maximum Temperature: %2f", xDeviceMaximumTemperature ) );
//     LogDebug( ( "Minimum Temperature: %2f", xDeviceMinimumTemperature ) );
//     LogDebug( ( "Average Temperature: %2f", xDeviceAverageTemperature ) );
// }
/*-----------------------------------------------------------*/

/**
 * @brief Send updated maximum temperature value to IoT Hub.
 */
static void prvSendNewMaxTemp( double xUpdatedTemperature )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    xResult = AzureIoTJSONWriter_Init( &xWriter, ucPropertyPayloadBuffer, sizeof( ucPropertyPayloadBuffer ) );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendPropertyName( &xWriter, ( uint8_t * ) sampleazureiotPROPERTY_MAX_TEMPERATURE_TEXT,
                                                     sizeof( sampleazureiotPROPERTY_MAX_TEMPERATURE_TEXT ) - 1 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendDouble( &xWriter, xUpdatedTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS );
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
        xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient, ucPropertyPayloadBuffer, lBytesWritten, NULL );

        if( xResult != eAzureIoTSuccess )
        {
            LogError( ( "There was an error sending the reported properties: 0x%08x", xResult ) );
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Send acknowledgment of requested target temperature value to IoT Hub.
 */
// static void prvAckIncomingTemperature( double xUpdatedTemperature,
//                                        uint32_t ulVersion )
// {
//     AzureIoTResult_t xResult;
//     AzureIoTJSONWriter_t xWriter;
//     int32_t lBytesWritten;

//     xResult = AzureIoTJSONWriter_Init( &xWriter, ucPropertyPayloadBuffer, sizeof( ucPropertyPayloadBuffer ) );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xAzureIoTHubClient,
//                                                                       &xWriter,
//                                                                       sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT,
//                                                                       strlen( sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT ),
//                                                                       sampleazureiotPROPERTY_STATUS_SUCCESS,
//                                                                       ulVersion,
//                                                                       sampleazureiotPROPERTY_SUCCESS,
//                                                                       strlen( sampleazureiotPROPERTY_SUCCESS ) );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONWriter_AppendDouble( &xWriter, xUpdatedTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTHubClientProperties_BuilderEndResponseStatus( &xAzureIoTHubClient,
//                                                                     &xWriter );
//     configASSERT( xResult == eAzureIoTSuccess );

//     xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
//     configASSERT( xResult == eAzureIoTSuccess );

//     lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );

//     if( lBytesWritten < 0 )
//     {
//         LogError( ( "Error getting the bytes written for the properties confirmation JSON" ) );
//     }
//     else
//     {
//         LogDebug( ( "Sending acknowledged writable property. Payload: %.*s", lBytesWritten, ucPropertyPayloadBuffer ) );
//         xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient, ucPropertyPayloadBuffer, lBytesWritten, NULL );

//         if( xResult != eAzureIoTSuccess )
//         {
//             LogError( ( "There was an error sending the reported properties: 0x%08x", xResult ) );
//         }
//     }
// }
/*-----------------------------------------------------------*/

/**
 * @brief Property mesage callback handler
 */
static void prvHandleProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                 void * pvContext )
{
    ( void ) pvContext;

    AzureIoTResult_t xResult;
    double xIncomingTemperature;
    uint32_t ulVersion;
    bool xWasMaxTemperatureChanged = false;

    LogInfo( ( "Property document payload : %.*s \r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubPropertiesGetMessage:
            LogInfo( ( "Device property document GET received" ) );
            xResult = prvProcessProperties( pxMessage, eAzureIoTHubClientPropertyWritable, &ulVersion );

            if( xResult == eAzureIoTSuccess )
            {
  //              prvUpdateLocalProperties( xIncomingTemperature, ulVersion, &xWasMaxTemperatureChanged );
//                prvAckIncomingTemperature( xIncomingTemperature, ulVersion );
            }
            else
            {
                LogError( ( "There was an error processing incoming properties" ) );
            }

            if( xWasMaxTemperatureChanged )
            {
                prvSendNewMaxTemp( xIncomingTemperature );
            }

            break;

        case eAzureIoTHubPropertiesWritablePropertyMessage:
            LogInfo( ( "Device writeable property received" ) );
            xResult = prvProcessProperties( pxMessage, eAzureIoTHubClientPropertyWritable, &ulVersion );

            if( xResult == eAzureIoTSuccess )
            {
//                prvUpdateLocalProperties( xIncomingTemperature, ulVersion, &xWasMaxTemperatureChanged );
//                prvAckIncomingTemperature( xIncomingTemperature, ulVersion );
            }
            else
            {
                LogError( ( "There was an error processing incoming properties" ) );
            }

            if( xWasMaxTemperatureChanged )
            {
                prvSendNewMaxTemp( xIncomingTemperature );
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
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub.
 */
static void prvAzureDemoTask( void * pvParameters )
{
    uint32_t ulPublishCount = 0U;
    uint32_t ulScratchBufferLength = 0U;
    const uint32_t ulMaxPublishCount = 10UL;
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
        xHubOptions.pucModelID = ( const uint8_t *) sampleazureiotMODEL_ID;
        xHubOptions.ulModelIDLength = sizeof( sampleazureiotMODEL_ID ) - 1;

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

        /* Publish messages with QoS1, send and process Keep alive messages. */
        for( ulPublishCount = 0; ulPublishCount < ulMaxPublishCount; ulPublishCount++ )
        {
            ulScratchBufferLength = snprintf( ( char * ) ucScratchBuffer, sizeof( ucScratchBuffer ),
                                              sampleazureiotMESSAGE, xDeviceCurrentTemperature );
            xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                       ucScratchBuffer, ulScratchBufferLength,
                                                       NULL, eAzureIoTHubMessageQoS1, NULL );
            configASSERT( xResult == eAzureIoTSuccess );

            LogInfo( ( "Attempt to receive publish message from IoT Hub.\r\n" ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                     sampleazureiotPROCESS_LOOP_TIMEOUT_MS );
            configASSERT( xResult == eAzureIoTSuccess );

            /* Leave Connection Idle for some time. */
            LogInfo( ( "Keeping Connection Idle...\r\n\r\n" ) );
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

        xResult = AzureIoTProvisioningClient_SetRegistrationPayload( &xAzureIoTProvisioningClient,
                                                                     sampleazureiotPROVISIONING_PAYLOAD,
                                                                     strlen( sampleazureiotPROVISIONING_PAYLOAD ) );
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
