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
                                         const uint8_t * pucAduContextBuffer,
                                         uint32_t ulAduContextBuffer )
{
    pxAduClient->pxHubClient = pxAzureIoTHubClient;
    pxAduClient->pxHTTPTransport = pxAzureIoTTransport;
    pxAduClient->xHTTPConnectCallback = pxAzureIoTHTTPConnectCallback;
    pxAduClient->pucAduContextBuffer = pucAduContextBuffer;
    pxAduClient->ulAduContextBufferLength = ulAduContextBuffer;

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

/**
 * @brief Process the update manifest which contains details of the image to download.
 *
 * @param pxAzureIoTHubClient
 * @param pxReader
 * @return AzureIoTResult_t
 */
static AzureIoTResult_t prvAzureIoT_ADUProcessUpdateManifest( AzureIoTADUClient_t * pxAduClient,
                                                              AzureIoTJSONReader_t * pxReader )
{
    ( void ) pxAduClient;
    ( void ) pxReader;
    /*Use the context buffer which was passed in options and the */
    /*AzureIoTJSONReader_GetTokenString to copy out relevant values */
    /*(since we will copy values out and act on then outside of the twin message callback) */

    /*Find Update ID */

    /*Find Compatability */

    /*Find Instructions with Steps */

    /*Find Files */

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTADUClient_ADUProcessComponent( AzureIoTADUClient_t * pxAduClient,
                                                        AzureIoTJSONReader_t * pxReader,
                                                        uint32_t ulPropertyVersion,
                                                        uint8_t * pucWritablePropertyResponseBuffer,
                                                        uint32_t ulWritablePropertyResponseBufferSize,
                                                        uint32_t * pulWritablePropertyResponseBufferLength )
{
    /* Iterate through the JSON and pull out the components that are useful. */

    /* No-op right now */
    prvAzureIoT_ADUProcessUpdateManifest( pxAduClient, pxReader );

    if( az_result_failed(
            az_iot_adu_ota_parse_service_properties(
                &pxAduClient->pxHubClient->_internal.xAzureIoTHubClientCore,
                &pxReader->_internal.xCoreReader,
                pxAduClient->xAduContextBuffer,
                &pxAduClient->xUpdateRequest,
                &pxAduClient->xAduContextBuffer ) ) )
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

static AzureIoTResult_t prvAzureIoT_ADUSendPropertyUpdate( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                           AzureIoTJSONWriter_t * pxWriter )
{
    ( void ) pxAzureIoTHubClient;
    ( void ) pxWriter;
    /*Send state update about the stage of the ADU process we are in */
    return eAzureIoTSuccess;
}

static AzureIoTResult_t prvHandleSteps( AzureIoTADUClient_t * pxAduClient )
{
    AzureIoTResult_t xResult;
    AzureIoTHTTPResult_t xHttpResult;
    AzureIoTJSONWriter_t xWriter;
    char * pucHttpDataBufferPtr;
    uint32_t pulHttpDataLength;

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
            xResult = prvAzureIoT_ADUSendPropertyUpdate( pxAduClient->pxHubClient,
                                                         &xWriter );

            AZLogInfo( ( "[ADU] Invoke HTTP Connect Callback.\r\n" ) );
            xResult = pxAduClient->xHTTPConnectCallback( pxAduClient->pxHTTPTransport, ( const char * ) "dawalton.blob.core.windows.net" );

            /* Range Check */
            AzureIoTHTTP_RequestSizeInit( &pxAduClient->xHTTP, pxAduClient->pxHTTPTransport,
                                          "dawalton.blob.core.windows.net",
                                          strlen( "dawalton.blob.core.windows.net" ),
                                          "/adu/azure_iot_freertos_esp32.bin",
                                          strlen( "/adu/azure_iot_freertos_esp32.bin" ) );

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
                                   "dawalton.blob.core.windows.net",
                                   strlen( "dawalton.blob.core.windows.net" ),
                                   "/adu/azure_iot_freertos_esp32.bin",
                                   strlen( "/adu/azure_iot_freertos_esp32.bin" ) );

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
                AZLogError( "[ADU] Error with firmware install: memory offset doesn't match image size\r\n" );
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

            AZLogInfo( "[ADU] Enable the update image\r\n" );
            AzureIoTPlatform_EnableImage( &pxAduClient->xImage );

            pxAduClient->xUpdateStepState = eAzureIoTADUUpdateStepFirmwareApplySucceeded;
            break;

        case eAzureIoTADUUpdateStepFirmwareApplySucceeded:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFirmwareApplySucceeded\r\n" ) );

            AZLogInfo( "[ADU] Reset the device\r\n" );
            AzureIoTPlatform_ResetDevice( &pxAduClient->xImage );

            break;

        case eAzureIoTADUUpdateStepFailed:

            AZLogInfo( ( "[ADU] Step: eAzureIoTADUUpdateStepFailed\r\n" ) );

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

    /* Hardcode step to have deployment in progress */
    pxAduClient->xState = eAzureIoTADUStateDeploymentInProgress;

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
