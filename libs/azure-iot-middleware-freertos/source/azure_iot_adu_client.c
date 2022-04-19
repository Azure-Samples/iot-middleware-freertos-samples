/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_adu_client.h"

#include "azure_iot.h"
/* Kernel includes. */
#include "FreeRTOS.h"

#include "azure_iot_json_writer.h"
#include "azure_iot_flash_platform.h"
#include <azure/iot/az_iot_adu_ota.h>

/*
 *
 * Missing features
 *
 * - Payload verification (SHA)
 * - How to download
 *
 */

/*
 *
 * Example Request
 *
 * {
 *  "deviceUpdate": {
 *      "__t": "c",
 *      "service": {
 *          "workflow": {
 *              "action": 3,
 *              "id": "d65e53b8-c718-4c01-b08e-e143e8dc5148"
 *          },
 *          "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"ESPRESSIF\",\"name\":\"ESP32-Azure-IoT-Kit\",\"version\":\"1.1\"},\"compatibility\":[{\"deviceManufacturer\":\"ESPRESSIF\",\"deviceModel\":\"ESP32-Azure-IoT-Kit\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:1\",\"files\":[\"fe82f37291b532f4a\"],\"handlerProperties\":{\"installedCriteria\":\"1.0\"}}]},\"files\":{\"fe82f37291b532f4a\":{\"fileName\":\"image.bin\",\"sizeInBytes\":9,\"hashes\":{\"sha256\":\"ohJJaB4M4iQyuge6YXkWUd/7aON3nTvTwbA0gDXyMyg=\"}}},\"createdDateTime\":\"2022-03-01T02:43:46.5927524Z\"}",
 *          "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJzaDRIcjZSNGxXNEtVRGdDNkJEQUo0dTNZMys1Z0srOXgxQVFSNnBTWEpRPSJ9.dSQKe7CPdj44VUSLlghLy-kSE5gsWEjPTLwdyGsgQTrxfk5fn-AJPe7VR5hVscfClPkI3EVpEbMN8rNDwDQupNnl0fp1phPr6kBj_j30UfRIvvd4lcoVYUABw-s8GhmuxShXrTPJWUSTGlTPlN1PdTvxIaRH2hoaMc6G2mZwoEQyjkAw_jqxXhYqYNXBl94AfcSTxqdGzHuEMfan7utaZgkS5HiGK_OFVlWXYFr3FpxoCcFmQDh-ANZJil25K4lngBuYTE5PgGguAD_ZvDBsyzw99ParYbgYXLk0wGagwPGhu2FANLmQuCsgYtXKt5lFs_hxJGEUTSslod6oXaw0utQlxRWgwMMIKTWAspzAm1XBsbpzB3FCF-97Qg5BzVexsRY3gtmKKp90eM0y5O_PXd7lAooPHf11BfQXJ2TCqcU9SdkPpQ4zZZILy5b2VP8vIgRL-VFwxzdyZpT3639t1oq1fnEtCBi91vLcXPMGq68iI7TrZH3slvTK3lSLlQO7",
 *          "fileUrls": {
 *              "fe82f37291b532f4a": "http://adu-ewertons-2--adu-ewertons-2.b.nlu.dl.adu.microsoft.com/westus2/adu-ewertons-2--adu-ewertons-2/260c33ee559a4671bedf9515652e4371/image.bin"
 *          }
 *      }
 *  },
 *  "$version": 2
 * }
 *
 */

/* Example Manifest
 * {
 * "manifestVersion": "4",
 * "updateId": {
 *  "provider": "Contoso",
 *  "name": "Toaster",
 *  "version": "1.0"
 * },
 * "compatibility": [
 *  {
 *    "deviceManufacturer": "Contoso",
 *    "deviceModel": "Toaster"
 *  }
 * ],
 * "instructions": {
 *  "steps": [
 *    {
 *      "handler": "microsoft/swupdate:1",
 *      "handlerProperties": {
 *        "installedCriteria": "1.0"
 *      },
 *      "files": [
 *        "fileId0"
 *      ]
 *    }
 *  ]
 * },
 * "files": {
 *  "fileId0": {
 *    "filename": "contoso.toaster.1.0.swu",
 *    "sizeInBytes": 718,
 *    "hashes": {
 *      "sha256": "mcB5SexMU4JOOzqmlJqKbue9qMskWY3EI/iVjJxCtAs="
 *    }
 *  }
 * },
 * "createdDateTime": "2021-09-28T18:32:01.8404544Z"
 * }
 */

AzureIoTResult_t AzureIoTADUClient_Init( AzureIoTADUClient_t * pxAduClient,
                                         AzureIoTHubClient_t * pxAzureIoTHubClient,
                                         AzureIoTTransportInterface_t * pxAzureIoTTransport,
                                         AzureIoT_TransportConnectCallback_t pxAzureIoTHTTPConnectCallback,
                                         const AzureIoTHubClientADUDeviceInformation_t * pxDeviceInformation,
                                         const AzureIoTHubClientADUInstallResult_t * pxLastInstallResult,
                                         uint8_t * pucAduContextBuffer,
                                         uint32_t ulAduContextBuffer )
{
    /* TODO: arg checks. Btw, device information is required. */
    /*       Last install workflow and results are optional (for a device that was never updated). */

    pxAduClient->pxHubClient = pxAzureIoTHubClient;
    pxAduClient->pxHTTPTransport = pxAzureIoTTransport;
    pxAduClient->xHTTPConnectCallback = pxAzureIoTHTTPConnectCallback;
    pxAduClient->pxDeviceInformation = pxDeviceInformation;
    pxAduClient->pxLastInstallResult = pxLastInstallResult;
    pxAduClient->pucAduContextBuffer = pucAduContextBuffer;
    pxAduClient->ulAduContextBufferLength = ulAduContextBuffer;
    pxAduClient->xAduContextAvailableBuffer =
        az_span_create( pxAduClient->pucAduContextBuffer, pxAduClient->ulAduContextBufferLength );
    pxAduClient->xSendProperties = true;

    memset( &pxAduClient->xUpdateRequest, 0, sizeof( pxAduClient->xUpdateRequest ) );
    memset( &pxAduClient->xUpdateManifest, 0, sizeof( pxAduClient->xUpdateManifest ) );

    return eAzureIoTSuccess;
}

bool AzureIoTADUClient_IsADUComponent( AzureIoTADUClient_t * pxAduClient,
                                       const char * pucComponentName,
                                       uint32_t ulComponentNameLength )
{
    ( void ) pxAduClient;

    return az_iot_adu_ota_is_component_device_update(
        az_span_create( ( uint8_t * ) pucComponentName, ( int32_t ) ulComponentNameLength ) );
}

AzureIoTResult_t AzureIoTADUClient_ADUProcessComponent( AzureIoTADUClient_t * pxAduClient,
                                                        AzureIoTJSONReader_t * pxReader,
                                                        uint32_t ulPropertyVersion,
                                                        uint8_t * pucWritablePropertyResponseBuffer,
                                                        uint32_t ulWritablePropertyResponseBufferSize,
                                                        uint32_t * pulWritablePropertyResponseBufferLength )
{
    /* Iterate through the JSON and pull out the components that are useful. */
    if( az_result_failed(
            az_iot_adu_ota_parse_service_properties(
                &pxAduClient->pxHubClient->_internal.xAzureIoTHubClientCore,
                &pxReader->_internal.xCoreReader,
                pxAduClient->xAduContextAvailableBuffer,
                &pxAduClient->xUpdateRequest,
                &pxAduClient->xAduContextAvailableBuffer ) ) )
    {
        AZLogError( ( "az_iot_adu_ota_parse_service_properties failed" ) );
        /* TODO: return individualized/specific errors. */
        return eAzureIoTErrorFailed;
    }
    else
    {
        az_span xWritablePropertyResponse = az_span_create(
            pucWritablePropertyResponseBuffer,
            ( int32_t ) ulWritablePropertyResponseBufferSize );

        az_result azres = az_iot_adu_ota_get_service_properties_response(
            &pxAduClient->pxHubClient->_internal.xAzureIoTHubClientCore,
            &pxAduClient->xUpdateRequest,
            ( int32_t ) ulPropertyVersion,
            200,
            xWritablePropertyResponse,
            &xWritablePropertyResponse );

        if( az_result_failed( azres ) )
        {
            AZLogError( ( "az_iot_adu_ota_get_service_properties_response failed: 0x%08x (%d)", azres, ulWritablePropertyResponseBufferSize ) );
            /* TODO: return individualized/specific errors. */
            return eAzureIoTErrorFailed;
        }
        else
        {
            azres = az_iot_adu_ota_parse_update_manifest(
                pxAduClient->xUpdateRequest.update_manifest,
                &pxAduClient->xUpdateManifest );

            if( az_result_failed( azres ) )
            {
                AZLogError( ( "az_iot_adu_ota_parse_update_manifest failed: 0x%08x (%d)", azres, ulWritablePropertyResponseBufferSize ) );
                /* TODO: return individualized/specific errors. */
                return eAzureIoTErrorFailed;
            }
            else
            {
                *pulWritablePropertyResponseBufferLength = ( uint32_t ) az_span_size( xWritablePropertyResponse );
                pxAduClient->xState = eAzureIoTADUStateDeploymentInProgress;
            }
        }
    }

    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvAzureIoT_ADUSendPropertyUpdate( AzureIoTADUClient_t * pxAduClient )
{
    az_iot_adu_ota_device_information xDeviceInformation;
    az_iot_adu_ota_install_result xLastInstallResult;
    az_span xPropertiesPayload = pxAduClient->xAduContextAvailableBuffer;

    xDeviceInformation.manufacturer =
        az_span_create( ( uint8_t * ) pxAduClient->pxDeviceInformation->ucManufacturer, pxAduClient->pxDeviceInformation->ulManufacturerLength );
    xDeviceInformation.model =
        az_span_create( ( uint8_t * ) pxAduClient->pxDeviceInformation->ucModel, pxAduClient->pxDeviceInformation->ulModelLength );
    xDeviceInformation.last_installed_update_id =
        az_span_create( ( uint8_t * ) pxAduClient->pxDeviceInformation->ucLastInstalledUpdateId, pxAduClient->pxDeviceInformation->ulLastInstalledUpdateIdLength );
    xDeviceInformation.adu_version = AZ_SPAN_FROM_STR( AZ_IOT_ADU_AGENT_VERSION );
    xDeviceInformation.do_version = AZ_SPAN_EMPTY;

    if( pxAduClient->pxLastInstallResult != NULL )
    {
        if( pxAduClient->pxLastInstallResult->ulStepResultsCount > MAX_INSTRUCTIONS_STEPS )
        {
            AZLogError( ( "[ADU] Last install result exceeds maximum number of supported steps." ) );
            return eAzureIoTErrorOutOfMemory;
        }

        xLastInstallResult.result_code = pxAduClient->pxLastInstallResult->lResultCode;
        xLastInstallResult.extended_result_code = pxAduClient->pxLastInstallResult->lExtendedResultCode;

        if( ( pxAduClient->pxLastInstallResult->ucResultDetails != NULL ) && ( pxAduClient->pxLastInstallResult->ulResultDetailsLength > 0 ) )
        {
            xLastInstallResult.result_details = az_span_create( ( uint8_t * ) pxAduClient->pxLastInstallResult->ucResultDetails, pxAduClient->pxLastInstallResult->ulResultDetailsLength );
        }
        else
        {
            xLastInstallResult.result_details = AZ_SPAN_EMPTY;
        }

        xLastInstallResult.step_results_count = pxAduClient->pxLastInstallResult->ulStepResultsCount;

        for( int lIndex = 0; lIndex < pxAduClient->pxLastInstallResult->ulStepResultsCount; lIndex++ )
        {
            xLastInstallResult.step_results[ lIndex ].step_id = az_span_create(
                ( uint8_t * ) pxAduClient->pxLastInstallResult->pxStepResults[ lIndex ].ucStepId,
                pxAduClient->pxLastInstallResult->pxStepResults[ lIndex ].ulStepIdLength );

            xLastInstallResult.step_results[ lIndex ].result_code =
                pxAduClient->pxLastInstallResult->pxStepResults[ lIndex ].ulResultCode;
            xLastInstallResult.step_results[ lIndex ].extended_result_code =
                pxAduClient->pxLastInstallResult->pxStepResults[ lIndex ].ulExtendedResultCode;

            if( ( pxAduClient->pxLastInstallResult->pxStepResults[ lIndex ].ucResultDetails != NULL ) &&
                ( pxAduClient->pxLastInstallResult->pxStepResults[ lIndex ].ulResultDetailsLength > 0 ) )
            {
                xLastInstallResult.step_results[ lIndex ].result_details = az_span_create(
                    ( uint8_t * ) pxAduClient->pxLastInstallResult->pxStepResults[ lIndex ].ucResultDetails,
                    pxAduClient->pxLastInstallResult->pxStepResults[ lIndex ].ulResultDetailsLength
                    );
            }
            else
            {
                xLastInstallResult.step_results[ lIndex ].result_details = AZ_SPAN_EMPTY;
            }
        }
    }

    if( az_result_failed(
            az_iot_adu_ota_get_properties_payload(
                &pxAduClient->pxHubClient->_internal.xAzureIoTHubClientCore,
                &xDeviceInformation,
                ( int32_t ) pxAduClient->xState,
                &pxAduClient->xUpdateRequest.workflow,
                ( pxAduClient->pxLastInstallResult != NULL ) ? &xLastInstallResult : NULL,
                xPropertiesPayload,
                &xPropertiesPayload ) ) )
    {
        AZLogError( ( "[ADU] Failed creating ADU agent report." ) );
        return eAzureIoTErrorFailed;
    }
    else if( AzureIoTHubClient_SendPropertiesReported(
                 pxAduClient->pxHubClient,
                 az_span_ptr( xPropertiesPayload ),
                 az_span_size( xPropertiesPayload ),
                 NULL ) != eAzureIoTSuccess )
    {
        AZLogError( ( "[ADU] Failed sending ADU agent state." ) );
        return eAzureIoTErrorPublishFailed;
    }

    /*Send state update about the stage of the ADU process we are in */
    return eAzureIoTSuccess;
}

/**
 * @brief This is a hack. TODO: Replace with a proper url-parsing api.
 */
static void prvParseAduUrl( az_span xUrl,
                            az_span * pxHost,
                            az_span * pxPath )
{
    xUrl = az_span_slice_to_end( xUrl, sizeof( "http://" ) - 1 );
    int32_t lPathPosition = az_span_find( xUrl, AZ_SPAN_FROM_STR( "/" ) );
    *pxHost = az_span_slice( xUrl, 0, lPathPosition );
    *pxPath = az_span_slice_to_end( xUrl, lPathPosition );
}

static AzureIoTResult_t prvHandleSteps( AzureIoTADUClient_t * pxAduClient )
{
    AzureIoTResult_t xResult;
    AzureIoTHTTPResult_t xHttpResult;
    char * pucHttpDataBufferPtr;
    uint32_t pulHttpDataLength;
    uint8_t ucSHA256Buffer[ 32 ];

    switch( pxAduClient->xUpdateStepState )
    {
        case eAzureIoTADUUpdateStepIdle:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepIdle\r\n" ) );

            pxAduClient->xUpdateStepState = eAzureIoTADUUpdateStepFirmwareDownloadStarted;

            break;

        /* We are at the beginning. Kick off the update. */
        case eAzureIoTADUUpdateStepManifestDownloadStarted:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepManifestDownloadStarted\r\n" ) );

            break;

        /* Only used in proxy update */
        case eAzureIoTADUUpdateStepManifestDownloadSucceeded:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepManifestDownloadSucceeded\r\n" ) );

            break;

        /* Only used in proxy update */
        case eAzureIoTADUUpdateStepFirmwareDownloadStarted:

            AzureIoTPlatform_Init( &pxAduClient->xImage );

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFirmwareDownloadStarted\r\n" ) );

            AZLogInfo( ( "[ADU] Send property update.\r\n" ) );
            xResult = prvAzureIoT_ADUSendPropertyUpdate( pxAduClient );

            AZLogInfo( ( "[ADU] Invoke HTTP Connect Callback.\r\n" ) );

            /* TODO: remove this and use proper URL parsing API. */
            /* TODO: cycle through all files of the update instead of hardcoding just the first one. */
            az_span xUrlHost;
            az_span xUrlPath;
            prvParseAduUrl( pxAduClient->xUpdateRequest.file_urls[ 0 ].url, &xUrlHost, &xUrlPath );

            /* TODO: remove this hack. */
            char pcNullTerminatedHost[ 128 ];
            ( void ) memcpy( pcNullTerminatedHost, az_span_ptr( xUrlHost ), az_span_size( xUrlHost ) );
            pcNullTerminatedHost[ az_span_size( xUrlHost ) ] = '\0';

            xResult = pxAduClient->xHTTPConnectCallback( pxAduClient->pxHTTPTransport, ( const char * ) pcNullTerminatedHost );

            /* Range Check */
            AzureIoTHTTP_RequestSizeInit( &pxAduClient->xHTTP, pxAduClient->pxHTTPTransport,
                                          ( const char * ) az_span_ptr( xUrlHost ),
                                          az_span_size( xUrlHost ),
                                          ( const char * ) az_span_ptr( xUrlPath ),
                                          az_span_size( xUrlPath ) );

            if( ( pxAduClient->xImage.ulImageFileSize = AzureIoTHTTP_RequestSize( &pxAduClient->xHTTP ) ) != -1 )
            {
                AZLogInfo( ( "[ADU] HTTP Range Request was successful: size %d bytes\r\n", pxAduClient->xImage.ulImageFileSize ) );
            }
            else
            {
                AZLogError( ( "[ADU] Error getting the headers.\r\n " ) );
                pxAduClient->xUpdateStepState = eAzureIoTADUUpdateStepFailed;
                break;
            }

            AZLogInfo( ( "[ADU] Send HTTP request.\r\n" ) );

            while( pxAduClient->xImage.ulCurrentOffset < pxAduClient->xImage.ulImageFileSize )
            {
                AZLogInfo( ( "[ADU] Initialize HTTP client.\r\n" ) );
                AzureIoTHTTP_Init( &pxAduClient->xHTTP, pxAduClient->pxHTTPTransport,
                                   ( const char * ) az_span_ptr( xUrlHost ),
                                   az_span_size( xUrlHost ),
                                   ( const char * ) az_span_ptr( xUrlPath ),
                                   az_span_size( xUrlPath ) );

                AZLogInfo( ( "[ADU] HTTP Requesting | %d:%d\r\n",
                             pxAduClient->xImage.ulCurrentOffset,
                             pxAduClient->xImage.ulCurrentOffset + azureiothttpCHUNK_DOWNLOAD_SIZE - 1 ) );

                if( ( xHttpResult = AzureIoTHTTP_Request( &pxAduClient->xHTTP, pxAduClient->xImage.ulCurrentOffset,
                                                          pxAduClient->xImage.ulCurrentOffset + azureiothttpCHUNK_DOWNLOAD_SIZE - 1,
                                                          &pucHttpDataBufferPtr,
                                                          &pulHttpDataLength ) ) == eAzureIoTHTTPSuccess )
                {
                    AZLogInfo( ( "[ADU] HTTP Request was successful | %d:%d\r\n",
                                 pxAduClient->xImage.ulCurrentOffset,
                                 pxAduClient->xImage.ulCurrentOffset + azureiothttpCHUNK_DOWNLOAD_SIZE - 1 ) );

                    /* Write bytes to the flash */
                    AZLogInfo( ( "[ADU] Write bytes to flash\r\n" ) );
                    xResult = AzureIoTPlatform_WriteBlock( &pxAduClient->xImage,
                                                           ( uint32_t ) pxAduClient->xImage.ulCurrentOffset,
                                                           ( uint8_t * ) pucHttpDataBufferPtr,
                                                           pulHttpDataLength );

                    /* Advance the offset */
                    pxAduClient->xImage.ulCurrentOffset += ( int32_t ) pulHttpDataLength;
                }
                else if( xHttpResult == eAzureIoTHTTPNoResponse )
                {
                    AZLogInfo( ( "[ADU] Reconnecting...\r\n" ) );
                    AZLogInfo( ( "[ADU] Invoke HTTP Connect Callback.\r\n" ) );
                    xResult = pxAduClient->xHTTPConnectCallback( pxAduClient->pxHTTPTransport, ( const char * ) "dawalton.blob.core.windows.net" );

                    if( xResult != eAzureIoTSuccess )
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            pxAduClient->xUpdateStepState = eAzureIoTADUUpdateStepFirmwareDownloadSucceeded;

            break;

        case eAzureIoTADUUpdateStepFirmwareDownloadSucceeded:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFirmwareDownloadSucceeded\r\n" ) );

            pxAduClient->xUpdateStepState = eAzureIoTADUUpdateStepFirmwareInstallStarted;

            break;

        case eAzureIoTADUUpdateStepFirmwareInstallStarted:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFirmwareInstallStarted\r\n" ) );

            if( pxAduClient->xImage.ulCurrentOffset == pxAduClient->xImage.ulImageFileSize )
            {
                /*We are done writing the whole image */

                /*Should we write block and then loop back to the initiate download with a certain range? */
                /*We would then move on to the install succeeded if all the parts are correctly written. */
                pxAduClient->xUpdateStepState = eAzureIoTADUUpdateStepFirmwareInstallSucceeded;
            }
            else
            {
                /* There was an error writing to the flash */
                AZLogError( ( "[ADU] Error with firmware install: memory offset doesn't match image size\r\n" ) );
                pxAduClient->xUpdateStepState = eAzureIoTADUUpdateStepFailed;
            }

            break;

        case eAzureIoTADUUpdateStepFirmwareInstallSucceeded:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFirmwareInstallSucceeded\r\n" ) );

            ulAzureIoTHTTP_Deinit( &pxAduClient->xHTTP );

            pxAduClient->xUpdateStepState = eAzureIoTADUUpdateStepFirmwareApplyStarted;

            break;

        case eAzureIoTADUUpdateStepFirmwareApplyStarted:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFirmwareApplyStarted\r\n" ) );

            AZLogInfo( ( "[ADU] Verify the image SHA256 against manifest signature\r\n" ) );

            /* File hash to compare against installed hash */
            az_span xHashSpan = pxAduClient->xUpdateManifest.files[ 0 ].hashes[ 0 ].hash;

            /* Call into platform specific image verification */
            xResult = AzureIoTPlatform_VerifyImage( &pxAduClient->xImage, ucSHA256Buffer );

            if( xResult == eAzureIoTSuccess )
            {
                AZLogInfo( ( "[ADU] Image validated against hash from ADU\r\n" ) );
            }
            else
            {
                AZLogError( ( "[ADU] File hash from ADU did not match calculated hash\r\n" ) );
            }

            AZLogInfo( ( "[ADU] Enable the update image\r\n" ) );
            AzureIoTPlatform_EnableImage( &pxAduClient->xImage );

            pxAduClient->xUpdateStepState = eAzureIoTADUUpdateStepFirmwareApplySucceeded;
            break;

        case eAzureIoTADUUpdateStepFirmwareApplySucceeded:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFirmwareApplySucceeded\r\n" ) );

            pxAduClient->xState = eAzureIoTADUStateIdle;
            pxAduClient->xSendProperties = true;

            AZLogInfo( ( "[ADU] Reset the device\r\n" ) );
            AzureIoTPlatform_ResetDevice( &pxAduClient->xImage );

            break;

        case eAzureIoTADUUpdateStepFailed:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFailed\r\n" ) );

            pxAduClient->xState = eAzureIotADUStateFailed;
            pxAduClient->xSendProperties = true;

            break;

        default:
            break;
    }

    ( void ) xResult;

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTADUClient_ADUProcessLoop( AzureIoTADUClient_t * pxAduClient,
                                                   uint32_t ulTimeoutMilliseconds )
{
    AzureIoTResult_t xResult;

    ( void ) ulTimeoutMilliseconds;

    if( pxAduClient->xSendProperties )
    {
        prvAzureIoT_ADUSendPropertyUpdate( pxAduClient );
        pxAduClient->xSendProperties = false;
    }

    switch( pxAduClient->xState )
    {
        case eAzureIoTADUStateIdle:
            /*Do Nothing */
            break;

        case eAzureIoTADUStateDeploymentInProgress:
            /* The incoming payload has been parsed and the AzureIoT_ADUContext_t is valid. */
            prvHandleSteps( pxAduClient );
            break;

        case eAzureIoTADUStateError:
            xResult = eAzureIoTErrorFailed;
            break;

        case eAzureIotADUStateFailed:
            /* According to ADU state transitions, the agent state shall */
            /* change to Idle after reporting an error state. */
            pxAduClient->xState = eAzureIoTADUStateIdle;
            pxAduClient->xSendProperties = true;
            break;

        default:
            break;
    }

    xResult = eAzureIoTSuccess;

    return xResult;
}

AzureIoTADUUpdateStepState_t AzureIoTADUClient_GetState( AzureIoTADUClient_t * pxAduClient )
{
    return pxAduClient->xUpdateStepState;
}
