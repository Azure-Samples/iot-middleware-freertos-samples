/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include <cmocka.h>

#include "azure_iot_adu_client.h"
/*-----------------------------------------------------------*/

static uint8_t pucComponentName[] = "deviceUpdate";

static uint8_t ucScratchBuffer[ 4000 ];

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
                                                    pucComponentName,
                                                    sizeof( pucComponentName ) - 1 ) );
}

static void testAzureIoTADUClient_IsADUComponent_Success( void ** ppvState )
{
    AzureIoTADUClient_t xTestIoTADUClient;

    assert_int_equal( AzureIoTADUClient_Init( &xTestIoTADUClient, NULL ), eAzureIoTSuccess );

    assert_true( AzureIoTADUClient_IsADUComponent( &xTestIoTADUClient,
                                                   pucComponentName,
                                                   sizeof( pucComponentName ) - 1 ) );
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
        cmocka_unit_test( testAzureIoTADUClient_SendResponse_InvalidArgFailure ),
        cmocka_unit_test( testAzureIoTADUClient_SendAgentState_InvalidArgFailure ),
    };

    return ( uint32_t ) cmocka_run_group_tests_name( "azure_iot_hub_client_ut", tests, NULL, NULL );
}
/*-----------------------------------------------------------*/
