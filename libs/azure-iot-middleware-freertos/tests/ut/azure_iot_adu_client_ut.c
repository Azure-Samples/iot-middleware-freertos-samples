/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_mqtt.h"
#include "azure_iot_adu_client.h"
/*-----------------------------------------------------------*/

#define testPROPERTY_CALLBACK_ID    ( 3 )
#define testDEVICE_MANUFACTURER     "Contoso"
#define testDEVICE_MODEL            "Foobar"
#define testUPDATE_PROVIDER         testDEVICE_MANUFACTURER
#define testUPDATE_NAME             testDEVICE_MODEL
#define testUPDATE_VERSION          "1.0"

static const uint8_t ucHostname[] = "unittest.azure-devices.net";
static const uint8_t ucDeviceId[] = "testiothub";
static uint8_t ucComponentName[] = "deviceUpdate";
static uint8_t ucSendStatePayload[] = "{\"deviceUpdate\":{\"__t\":\"c\",\"agent\":{\"deviceProperties\":{\"manufacturer\":\"Contoso\",\"model\":\"Foobar\",\"interfaceId\":\"dtmi:azure:iot:deviceUpdate;1\",\"aduVer\":\"DU;agent/0.8.0-rc1-public-preview\"},\"compatPropertyNames\":\"manufacturer,model\",\"state\":0,\"installedUpdateId\":\"{\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\",\\\"version\\\":\\\"1.0\\\"}\"}}}";
static uint8_t ucSendResponsePayload[] = "{\"deviceUpdate\":{\"__t\":\"c\",\"service\":{\"ac\":200,\"av\":1,\"value\":{}}}}";
static uint8_t ucHubClientBuffer[ 512 ];
static uint8_t ucScratchBuffer[ 8000 ];
static uint32_t ulReceivedCallbackFunctionId;

/*Request Values */
static uint8_t ucADURequestPayload[] = "{\"service\":{\"workflow\":{\"action\":3,\"id\":\"51552a54-765e-419f-892a-c822549b6f38\"},\"updateManifest\":\"{\\\"manifestVersion\\\":\\\"4\\\",\\\"updateId\\\":{\\\"provider\\\":\\\"PC\\\",\\\"name\\\":\\\"Linux\\\",\\\"version\\\":\\\"1.1\\\"},\\\"compatibility\\\":[{\\\"deviceManufacturer\\\":\\\"PC\\\",\\\"deviceModel\\\":\\\"Linux\\\"}],\\\"instructions\\\":{\\\"steps\\\":[{\\\"handler\\\":\\\"microsoft/swupdate:1\\\",\\\"files\\\":[\\\"f2f4a804ca17afbae\\\"],\\\"handlerProperties\\\":{\\\"installedCriteria\\\":\\\"1.0\\\"}}]},\\\"files\\\":{\\\"f2f4a804ca17afbae\\\":{\\\"fileName\\\":\\\"iot-middleware-sample-adu-v1.1\\\",\\\"sizeInBytes\\\":844976,\\\"hashes\\\":{\\\"sha256\\\":\\\"xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/WBi0=\\\"}}},\\\"createdDateTime\\\":\\\"2022-07-07T03:02:48.8449038Z\\\"}\",\"updateManifestSignature\":\"eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJiUlkrcis0MzdsYTV5d2hIeDdqVHhlVVRkeDdJdXQyQkNlcVpoQys5bmFNPSJ9.eYoBoq9EOiCebTJAMhRh9DARC69F3C4Qsia86no9YbMJzwKt-rH88Va4dL59uNTlPNBQid4u0RlXSUTuma_v-Sf4hyw70tCskwru5Fp41k9Ve3YSkulUKzctEhaNUJ9tUSA11Tz9HwJHOAEA1-S_dXWR_yuxabk9G_BiucsuKhoI0Bas4e1ydQE2jXZNdVVibrFSqxvuVZrxHKVhwm-G9RYHjZcoSgmQ58vWyaC2l8K8ZqnlQWmuLur0CZFQlanUVxDocJUtu1MnB2ER6emMRD_4Azup2K4apq9E1EfYBbXxOZ0N5jaSr-2xg8NVSow5NqNSaYYY43wy_NIUefRlbSYu5zOrSWtuIwRdsO-43Eo8b9vuJj1Qty9ee6xz1gdUNHnUdnM6dHEplZK0GZznsxRviFXt7yv8bVLd32Z7QDtFh3s17xlKulBZxWP-q96r92RoUTov2M3ynPZSDmc6Mz7-r8ioO5VHO5pAPCH-tF5zsqzipPJKmBMaf5gYk8wR\",\"fileUrls\":{\"f2f4a804ca17afbae\":\"http://dwalton-adu-instance--dwalton-adu.b.nlu.dl.adu.microsoft.com/westus2/dwalton-adu-instance--dwalton-adu/67c8d2ef5148403391bed74f51a28597/iot-middleware-sample-adu-v1.1\"}}}";

AzureIoTADUClientDeviceInformation_t xADUDeviceInformation =
{
    .ucManufacturer       = testDEVICE_MANUFACTURER,
    .ulManufacturerLength = sizeof( testDEVICE_MANUFACTURER ) - 1,
    .ucModel              = testDEVICE_MODEL,
    .ulModelLength        = sizeof( testDEVICE_MODEL ) - 1,
    .xCurrentUpdateId     =
    {
        .ucProvider       = testUPDATE_PROVIDER,
        .ulProviderLength = sizeof( testUPDATE_PROVIDER ) - 1,
        .ucName           = testUPDATE_NAME,
        .ulNameLength     = sizeof( testUPDATE_NAME ) - 1,
        .ucVersion        = testUPDATE_VERSION,
        .ulVersionLength  = sizeof( testUPDATE_VERSION ) - 1
    }
};

static AzureIoTTransportInterface_t xTransportInterface =
{
    .pxNetworkContext = NULL,
    .xSend            = ( AzureIoTTransportSend_t ) 0xA5A5A5A5,
    .xRecv            = ( AzureIoTTransportRecv_t ) 0xACACACAC
};

/* Data exported by cmocka port for MQTT */
extern AzureIoTMQTTPacketInfo_t xPacketInfo;
extern AzureIoTMQTTDeserializedInfo_t xDeserializedInfo;
extern uint16_t usTestPacketId;
extern uint32_t ulDelayReceivePacket;

/*-----------------------------------------------------------*/

TickType_t xTaskGetTickCount( void );
uint32_t ulGetAllTests();

TickType_t xTaskGetTickCount( void )
{
    return 1;
}
/*-----------------------------------------------------------*/

static uint64_t prvGetUnixTime( void )
{
    return 0xFFFFFFFFFFFFFFFF;
}

/*-----------------------------------------------------------*/

static void prvSetupTestIoTHubClient( AzureIoTHubClient_t * pxTestIoTHubClient )
{
    AzureIoTHubClientOptions_t xHubClientOptions = { 0 };

    will_return( AzureIoTMQTT_Init, eAzureIoTMQTTSuccess );
    assert_int_equal( AzureIoTHubClient_Init( pxTestIoTHubClient,
                                              ucHostname, sizeof( ucHostname ) - 1,
                                              ucDeviceId, sizeof( ucDeviceId ) - 1,
                                              &xHubClientOptions,
                                              ucHubClientBuffer,
                                              sizeof( ucHubClientBuffer ),
                                              prvGetUnixTime,
                                              &xTransportInterface ),
                      eAzureIoTSuccess );
}

/*-----------------------------------------------------------*/

static void prvTestProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                               void * pvContext )
{
    assert_true( pxMessage != NULL );
    assert_true( pvContext == NULL );


    assert_int_equal( pxMessage->xMessageType, eAzureIoTHubPropertiesReportedResponseMessage );

    ulReceivedCallbackFunctionId = testPROPERTY_CALLBACK_ID;
}

/*-----------------------------------------------------------*/

static void testAzureIoTADUClient_OptionsInit_InvalidArgFailure( void ** ppvState )
{
    ( void ) ppvState;

    assert_int_equal( AzureIoTADUClient_OptionsInit( NULL ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTADUClient_OptionsInit_Success( void ** ppvState )
{
    ( void ) ppvState;
    AzureIoTADUClientOptions_t xADUClientOptions;

    assert_int_equal( AzureIoTADUClient_OptionsInit( &xADUClientOptions ), eAzureIoTSuccess );
}

static void testAzureIoTADUClient_Init_InvalidArgFailure( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTADUClientOptions_t xADUClientOptions = { 0 };

    ( void ) ppvState;

    assert_int_equal( AzureIoTADUClient_Init( NULL, &xADUClientOptions ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTADUClient_Init_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTADUClientOptions_t xADUClientOptions = { 0 };

    ( void ) ppvState;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, &xADUClientOptions ), eAzureIoTSuccess );
}

static void testAzureIoTADUClient_IsADUComponent_InvalidArgFailure( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_false( AzureIoTADUClient_IsADUComponent( NULL,
                                                    ucComponentName,
                                                    sizeof( ucComponentName ) - 1 ) );
}

static void testAzureIoTADUClient_IsADUComponent_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_true( AzureIoTADUClient_IsADUComponent( &xTestIoTADUClient,
                                                   ucComponentName,
                                                   sizeof( ucComponentName ) - 1 ) );
}

static void testAzureIoTADUClient_ParseRequest_InvalidArgFailure( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTJSONReader_t xReader;
    AzureIoTADUUpdateRequest_t xRequest;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_ParseRequest( NULL,
                                                      &xReader,
                                                      &xRequest,
                                                      ucScratchBuffer,
                                                      sizeof( ucScratchBuffer ) ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      NULL,
                                                      &xRequest,
                                                      ucScratchBuffer,
                                                      sizeof( ucScratchBuffer ) ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      NULL,
                                                      ucScratchBuffer,
                                                      sizeof( ucScratchBuffer ) ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      &xRequest,
                                                      NULL,
                                                      sizeof( ucScratchBuffer ) ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      &xRequest,
                                                      ucScratchBuffer,
                                                      0 ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTADUClient_ParseRequest_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTJSONReader_t xReader;
    AzureIoTADUUpdateRequest_t xRequest;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTJSONReader_Init( &xReader, ucADURequestPayload, sizeof( ucADURequestPayload ) - 1 ), eAzureIoTSuccess );

    /* ParseRequest requires that the reader be placed on the "service" prop name */
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );
    assert_int_equal( AzureIoTJSONReader_NextToken( &xReader ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_ParseRequest( &xTestIoTADUClient,
                                                      &xReader,
                                                      &xRequest,
                                                      ucScratchBuffer,
                                                      sizeof( ucScratchBuffer ) ), eAzureIoTSuccess );

    /* Workflow */
    assert_int_equal( xRequest.xWorkflow.xAction, 3 );

    /* Update Manifest */
    assert_memory_equal( xRequest.xUpdateManifest.pucManifestVersion, "4", sizeof( "4" ) - 1 );
}

static void testAzureIoTADUClient_SendResponse_InvalidArgFailure( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTADURequestDecision_t xRequestDecision = eAzureIoTADURequestDecisionAccept;
    AzureIoTADUUpdateRequest_t xRequest;
    uint32_t ulPropertyVersion = 1;
    uint32_t ulRequestId;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_SendResponse( NULL,
                                                      &xTestIoTHubClient,
                                                      xRequestDecision,
                                                      ulPropertyVersion,
                                                      ucScratchBuffer,
                                                      sizeof( ucScratchBuffer ),
                                                      &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendResponse( &xTestIoTADUClient,
                                                      NULL,
                                                      xRequestDecision,
                                                      ulPropertyVersion,
                                                      ucScratchBuffer,
                                                      sizeof( ucScratchBuffer ),
                                                      &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendResponse( &xTestIoTADUClient,
                                                      &xTestIoTHubClient,
                                                      xRequestDecision,
                                                      ulPropertyVersion,
                                                      NULL,
                                                      sizeof( ucScratchBuffer ),
                                                      &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendResponse( &xTestIoTADUClient,
                                                      &xTestIoTHubClient,
                                                      xRequestDecision,
                                                      ulPropertyVersion,
                                                      ucScratchBuffer,
                                                      0,
                                                      &ulRequestId ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTADUClient_SendResponse_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTADURequestDecision_t xRequestDecision = eAzureIoTADURequestDecisionAccept;
    AzureIoTADUUpdateRequest_t xRequest;
    uint32_t ulPropertyVersion = 1;
    uint32_t ulRequestId;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );


    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );

    assert_int_equal( AzureIoTADUClient_SendResponse( &xTestIoTADUClient,
                                                      &xTestIoTHubClient,
                                                      xRequestDecision,
                                                      ulPropertyVersion,
                                                      ucScratchBuffer,
                                                      sizeof( ucScratchBuffer ),
                                                      &ulRequestId ), eAzureIoTSuccess );

    assert_memory_equal( ucScratchBuffer, ucSendResponsePayload, sizeof( ucSendResponsePayload ) - 1 );
}

static void testAzureIoTADUClient_SendAgentState_InvalidArgFailure( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTHubClient_t xTestIoTHubClient;
    AzureIoTADUClientDeviceInformation_t xDeviceInfo;
    AzureIoTADUUpdateRequest_t xRequest;
    AzureIoTADUAgentState_t xState;
    AzureIoTADUClientInstallResult_t xInstallResult;
    uint32_t ulPropertyVersion = 1;
    uint32_t ulRequestId;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_int_equal( AzureIoTADUClient_SendAgentState( NULL,
                                                        &xTestIoTHubClient,
                                                        &xDeviceInfo,
                                                        &xRequest,
                                                        xState,
                                                        &xInstallResult,
                                                        ucScratchBuffer,
                                                        sizeof( ucScratchBuffer ),
                                                        &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        NULL,
                                                        &xDeviceInfo,
                                                        &xRequest,
                                                        xState,
                                                        &xInstallResult,
                                                        ucScratchBuffer,
                                                        sizeof( ucScratchBuffer ),
                                                        &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        &xTestIoTHubClient,
                                                        NULL,
                                                        &xRequest,
                                                        xState,
                                                        &xInstallResult,
                                                        ucScratchBuffer,
                                                        sizeof( ucScratchBuffer ),
                                                        &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        &xTestIoTHubClient,
                                                        &xDeviceInfo,
                                                        &xRequest,
                                                        xState,
                                                        &xInstallResult,
                                                        NULL,
                                                        sizeof( ucScratchBuffer ),
                                                        &ulRequestId ), eAzureIoTErrorInvalidArgument );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        &xTestIoTHubClient,
                                                        &xDeviceInfo,
                                                        &xRequest,
                                                        xState,
                                                        &xInstallResult,
                                                        ucScratchBuffer,
                                                        0,
                                                        &ulRequestId ), eAzureIoTErrorInvalidArgument );
}

static void testAzureIoTADUClient_SendAgentState_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;
    AzureIoTHubClient_t xTestIoTHubClient;
    uint32_t ulPropertyVersion = 1;
    uint32_t ulRequestId;

    prvSetupTestIoTHubClient( &xTestIoTHubClient );

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    will_return( AzureIoTMQTT_Subscribe, eAzureIoTMQTTSuccess );
    will_return( AzureIoTMQTT_ProcessLoop, eAzureIoTMQTTSuccess );
    xPacketInfo.ucType = azureiotmqttPACKET_TYPE_SUBACK;
    xDeserializedInfo.usPacketIdentifier = usTestPacketId;
    ulDelayReceivePacket = 0;
    assert_int_equal( AzureIoTHubClient_SubscribeProperties( &xTestIoTHubClient,
                                                             prvTestProperties,
                                                             NULL, ( uint32_t ) -1 ),
                      eAzureIoTSuccess );


    will_return( AzureIoTMQTT_Publish, eAzureIoTMQTTSuccess );

    assert_int_equal( AzureIoTADUClient_SendAgentState( &xTestIoTADUClient,
                                                        &xTestIoTHubClient,
                                                        &xADUDeviceInformation,
                                                        NULL,
                                                        eAzureIoTADUAgentStateIdle,
                                                        NULL,
                                                        ucScratchBuffer,
                                                        sizeof( ucScratchBuffer ),
                                                        &ulRequestId ), eAzureIoTSuccess );
    assert_memory_equal( ucScratchBuffer, ucSendStatePayload, sizeof( ucSendStatePayload ) - 1 );
}


uint32_t ulGetAllTests()
{
    const struct CMUnitTest tests[] =
    {
        cmocka_unit_test( testAzureIoTADUClient_OptionsInit_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_OptionsInit_Success ),
        cmocka_unit_test( testAzureIoTADUClient_Init_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_Init_Success ),
        cmocka_unit_test( testAzureIoTADUClient_IsADUComponent_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_IsADUComponent_Success ),
        cmocka_unit_test( testAzureIoTADUClient_ParseRequest_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_ParseRequest_Success ),
        cmocka_unit_test( testAzureIoTADUClient_SendResponse_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_SendResponse_Success ),
        cmocka_unit_test( testAzureIoTADUClient_SendAgentState_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_SendAgentState_Success )
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_hub_client_ut", tests, NULL, NULL );
}
/*-----------------------------------------------------------*/
