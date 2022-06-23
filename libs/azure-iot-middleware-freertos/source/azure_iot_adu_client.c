/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_adu_client.h"

#include "azure_iot.h"
/* Kernel includes. */
#include "FreeRTOS.h"

#include "azure_iot_json_writer.h"
#include "azure_iot_flash_platform.h"
#include "azure_iot_private.h"
#include <azure/iot/az_iot_adu_client.h>

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

AzureIoTResult_t AzureIoTADUClient_OptionsInit( AzureIoTADUClientOptions_t * pxADUClientOptions )
{
    AzureIoTResult_t xResult;

    if( pxADUClientOptions == NULL )
    {
        AZLogError( ( "AzureIoTADUClient_OptionsInit failed: invalid argument" ) );
        xResult = eAzureIoTErrorInvalidArgument;
    }
    else
    {
        memset( pxADUClientOptions, 0, sizeof( AzureIoTADUClientOptions_t ) );

        xResult = eAzureIoTSuccess;
    }

    return xResult;
}

AzureIoTResult_t AzureIoTADUClient_Init( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                         AzureIoTADUClientOptions_t * pxADUClientOptions )
{
    az_result xCoreResult;
    az_iot_adu_client_options xADUOptions;

    if( ( pxAzureIoTADUClient == NULL ) )
    {
        AZLogError( ( "AzureIoTADUClient_Init failed: invalid argument" ) );
        return eAzureIoTErrorInvalidArgument;
    }

    memset( ( void * ) pxAzureIoTADUClient, 0, sizeof( AzureIoTADUClient_t ) );

    xADUOptions = az_iot_adu_client_options_default();

    if( pxADUClientOptions )
    {
        xADUOptions.unused = pxADUClientOptions->xUnused;
    }

    if( az_result_failed( xCoreResult = az_iot_adu_client_init( &pxAzureIoTADUClient->_internal.xADUClient, &xADUOptions ) ) )
    {
        AZLogError( ( "Failed to initialize az_iot_adu_client_init: core error=0x%08x", xCoreResult ) );
        return AzureIoT_TranslateCoreError( xCoreResult );
    }

    return eAzureIoTSuccess;
}

bool AzureIoTADUClient_IsADUComponent( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                       const char * pucComponentName,
                                       uint32_t ulComponentNameLength )
{
    return az_iot_adu_client_is_component_device_update( &pxAzureIoTADUClient->_internal.xADUClient,
                                                         az_span_create( ( uint8_t * ) pucComponentName, ( int32_t ) ulComponentNameLength ) );
}

static void prvCastUpdateRequest( az_iot_adu_client_update_request * pxBaseUpdateRequest,
                                  az_iot_adu_client_update_manifest * pxBaseUpdateManifest,
                                  AzureIoTADUUpdateRequest_t * pxUpdateRequest )
{
    pxUpdateRequest->xWorkflow.pucID = az_span_ptr( pxBaseUpdateRequest->workflow.id );
    pxUpdateRequest->xWorkflow.ulIDLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->workflow.id );
    pxUpdateRequest->xWorkflow.xAction = ( AzureIoTADUAction_t ) pxBaseUpdateRequest->workflow.action;
    pxUpdateRequest->xWorkflow.pucRetryTimestamp = az_span_ptr( pxBaseUpdateRequest->workflow.retry_timestamp );
    pxUpdateRequest->xWorkflow.ulRetryTimestampLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->workflow.retry_timestamp );
    pxUpdateRequest->pucUpdateManifestSignature = az_span_ptr( pxBaseUpdateRequest->update_manifest_signature );
    pxUpdateRequest->ulUpdateManifestSignatureLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->update_manifest_signature );
    pxUpdateRequest->ulFileUrlCount = pxBaseUpdateRequest->file_urls_count;

    for( uint32_t ulFileUrlIndex = 0; ulFileUrlIndex < pxBaseUpdateRequest->file_urls_count; ulFileUrlIndex++ )
    {
        pxUpdateRequest->pxFileUrls[ ulFileUrlIndex ].pucId = az_span_ptr( pxBaseUpdateRequest->file_urls[ ulFileUrlIndex ].id );
        pxUpdateRequest->pxFileUrls[ ulFileUrlIndex ].ulIdLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->file_urls[ ulFileUrlIndex ].id );
        pxUpdateRequest->pxFileUrls[ ulFileUrlIndex ].pucUrl = az_span_ptr( pxBaseUpdateRequest->file_urls[ ulFileUrlIndex ].url );
        pxUpdateRequest->pxFileUrls[ ulFileUrlIndex ].ulUrlLength = ( uint32_t ) az_span_size( pxBaseUpdateRequest->file_urls[ ulFileUrlIndex ].url );
    }

    pxUpdateRequest->xUpdateManifest.xUpdateId.pucProvider = az_span_ptr( pxBaseUpdateManifest->update_id.provider );
    pxUpdateRequest->xUpdateManifest.xUpdateId.ulProviderLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->update_id.provider );
    pxUpdateRequest->xUpdateManifest.xUpdateId.pucName = az_span_ptr( pxBaseUpdateManifest->update_id.name );
    pxUpdateRequest->xUpdateManifest.xUpdateId.ulNameLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->update_id.name );
    pxUpdateRequest->xUpdateManifest.xUpdateId.pucVersion = az_span_ptr( pxBaseUpdateManifest->update_id.version );
    pxUpdateRequest->xUpdateManifest.xUpdateId.ulVersionLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->update_id.version );

    pxUpdateRequest->xUpdateManifest.xCompatibility.pucDeviceManufacturer = az_span_ptr( pxBaseUpdateManifest->compatibility.device_manufacturer );
    pxUpdateRequest->xUpdateManifest.xCompatibility.ulDeviceManufacturerLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->compatibility.device_manufacturer );
    pxUpdateRequest->xUpdateManifest.xCompatibility.pucDeviceModel = az_span_ptr( pxBaseUpdateManifest->compatibility.device_model );
    pxUpdateRequest->xUpdateManifest.xCompatibility.ulDeviceModelLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->compatibility.device_model );

    pxUpdateRequest->xUpdateManifest.xInstructions.ulStepsCount = pxBaseUpdateManifest->instructions.steps_count;

    for( uint32_t ulStepIndex = 0; ulStepIndex < pxBaseUpdateManifest->instructions.steps_count; ulStepIndex++ )
    {
        pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].pucHandler =
            az_span_ptr( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].handler );
        pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].ulHandlerLength =
            ( uint32_t ) az_span_size( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].handler );
        pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].pucInstalledCriteria =
            az_span_ptr( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].handler_properties.installed_criteria );
        pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].ulInstalledCriteriaLength =
            ( uint32_t ) az_span_size( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].handler_properties.installed_criteria );
        pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].ulFilesCount =
            pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].files_count;

        for( uint32_t ulFileIndex = 0; ulFileIndex < pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].files_count; ulFileIndex++ )
        {
            pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].pxFiles[ ulFileIndex ].pucFileName =
                az_span_ptr( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].files[ ulFileIndex ] );
            pxUpdateRequest->xUpdateManifest.xInstructions.pxSteps[ ulStepIndex ].pxFiles[ ulFileIndex ].ulFileNameLength =
                ( uint32_t ) az_span_size( pxBaseUpdateManifest->instructions.steps[ ulStepIndex ].files[ ulFileIndex ] );
        }
    }

    pxUpdateRequest->xUpdateManifest.ulFilesCount = pxBaseUpdateManifest->files_count;

    for( uint32_t ulFileIndex = 0; ulFileIndex < pxBaseUpdateManifest->files_count; ulFileIndex++ )
    {
        pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pucId =
            az_span_ptr( pxBaseUpdateManifest->files[ ulFileIndex ].id );
        pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].ulIdLength =
            ( uint32_t ) az_span_size( pxBaseUpdateManifest->files[ ulFileIndex ].id );
        pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pucFileName =
            az_span_ptr( pxBaseUpdateManifest->files[ ulFileIndex ].file_name );
        pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].ulFileNameLength =
            ( uint32_t ) az_span_size( pxBaseUpdateManifest->files[ ulFileIndex ].file_name );
        pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].ulSizeInBytes =
            pxBaseUpdateManifest->files[ ulFileIndex ].size_in_bytes;
        pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].ulHashesCount =
            pxBaseUpdateManifest->files[ ulFileIndex ].hashes_count;

        for( uint32_t ulFileHashIndex = 0; ulFileHashIndex < pxBaseUpdateManifest->files_count; ulFileHashIndex++ )
        {
            pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pxHashes[ ulFileHashIndex ].pucId =
                az_span_ptr( pxBaseUpdateManifest->files[ ulFileIndex ].hashes[ ulFileHashIndex ].hash_type );
            pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pxHashes[ ulFileHashIndex ].ulIdLength =
                ( uint32_t ) az_span_size( pxBaseUpdateManifest->files[ ulFileIndex ].hashes[ ulFileHashIndex ].hash_type );
            pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pxHashes[ ulFileHashIndex ].pucHash =
                az_span_ptr( pxBaseUpdateManifest->files[ ulFileIndex ].hashes[ ulFileHashIndex ].hash_value );
            pxUpdateRequest->xUpdateManifest.pxFiles[ ulFileIndex ].pxHashes[ ulFileHashIndex ].ulHashLength =
                ( uint32_t ) az_span_size( pxBaseUpdateManifest->files[ ulFileIndex ].hashes[ ulFileHashIndex ].hash_value );
        }
    }

    pxUpdateRequest->xUpdateManifest.pucManifestVersion = az_span_ptr( pxBaseUpdateManifest->manifest_version );
    pxUpdateRequest->xUpdateManifest.ulManifestVersionLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->manifest_version );
    pxUpdateRequest->xUpdateManifest.pucCreateDateTime = az_span_ptr( pxBaseUpdateManifest->create_date_time );
    pxUpdateRequest->xUpdateManifest.ulCreateDateTimeLength = ( uint32_t ) az_span_size( pxBaseUpdateManifest->create_date_time );
}

AzureIoTResult_t AzureIoTADUClient_ParseRequest( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                                 AzureIoTJSONReader_t * pxReader,
                                                 AzureIoTADUUpdateRequest_t * pxAduUpdateRequest,
                                                 uint8_t * pucBuffer,
                                                 uint32_t ulBufferSize )
{
    az_iot_adu_client_update_request xBaseUpdateRequest;
    az_iot_adu_client_update_manifest xBaseUpdateManifest;
    az_result xAzResult;
    az_span xBufferSpan = az_span_create( pucBuffer, ( int32_t ) ulBufferSize );

    xAzResult = az_iot_adu_client_parse_service_properties(
        &pxAzureIoTADUClient->_internal.xADUClient,
        &pxReader->_internal.xCoreReader,
        xBufferSpan,
        &xBaseUpdateRequest,
        &xBufferSpan );

    if( az_result_failed( xAzResult ) )
    {
        AZLogError( ( "az_iot_adu_client_parse_service_properties failed" ) );
        /* TODO: return individualized/specific errors. */
        return eAzureIoTErrorFailed;
    }
    else
    {
        xAzResult = az_iot_adu_client_parse_update_manifest(
            &pxAzureIoTADUClient->_internal.xADUClient,
            xBaseUpdateRequest.update_manifest,
            &xBaseUpdateManifest );

        if( az_result_failed( xAzResult ) )
        {
            AZLogError( ( "az_iot_adu_client_parse_update_manifest failed: 0x%08x", xAzResult ) );
            /* TODO: return individualized/specific errors. */
            return eAzureIoTErrorFailed;
        }

        prvCastUpdateRequest( &xBaseUpdateRequest, &xBaseUpdateManifest, pxAduUpdateRequest );
    }

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTADUClient_SendResponse( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                                 AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                 AzureIoTADURequestDecision_t xRequestDecision,
                                                 uint32_t ulPropertyVersion,
                                                 uint8_t * pucWritablePropertyResponseBuffer,
                                                 uint32_t ulWritablePropertyResponseBufferSize,
                                                 uint32_t * pulRequestId )
{
    az_span xWritablePropertyResponse = az_span_create(
        pucWritablePropertyResponseBuffer,
        ( int32_t ) ulWritablePropertyResponseBufferSize );

    /* TODO: consume xRequestDecision in az_iot_adu_client_get_service_properties_response */
    ( void ) xRequestDecision;
    az_result xAzResult = az_iot_adu_client_get_service_properties_response(
        &pxAzureIoTADUClient->_internal.xADUClient,
        ( int32_t ) ulPropertyVersion,
        200,
        xWritablePropertyResponse,
        &xWritablePropertyResponse );

    if( az_result_failed( xAzResult ) )
    {
        AZLogError( ( "az_iot_adu_client_get_service_properties_response failed: 0x%08x (%d)", xAzResult, ulWritablePropertyResponseBufferSize ) );
        /* TODO: return individualized/specific errors. */
        return eAzureIoTErrorFailed;
    }

    xAzResult = AzureIoTHubClient_SendPropertiesReported(
        pxAzureIoTHubClient,
        az_span_ptr( xWritablePropertyResponse ),
        ( uint32_t ) az_span_size( xWritablePropertyResponse ),
        pulRequestId );

    if( xAzResult != eAzureIoTSuccess )
    {
        AZLogError( ( "[ADU] Failed sending ADU writable properties response: 0x%08x", xAzResult ) );
        return eAzureIoTErrorPublishFailed;
    }

    return eAzureIoTSuccess;
}

static void prvFillBaseDeviceInformation( AzureIoTHubClientADUDeviceInformation_t * pxDeviceInformation,
                                          az_iot_adu_client_device_information * pxBaseDeviceInformation )
{
    pxBaseDeviceInformation->manufacturer = az_span_create(
        ( uint8_t * ) pxDeviceInformation->ucManufacturer,
        ( int32_t ) pxDeviceInformation->ulManufacturerLength );
    pxBaseDeviceInformation->model = az_span_create(
        ( uint8_t * ) pxDeviceInformation->ucModel,
        ( int32_t ) pxDeviceInformation->ulModelLength );
    pxBaseDeviceInformation->update_id.name = az_span_create(
        ( uint8_t * ) pxDeviceInformation->xCurrentUpdateId.ucName,
        ( int32_t ) pxDeviceInformation->xCurrentUpdateId.ulNameLength );
    pxBaseDeviceInformation->update_id.provider = az_span_create(
        ( uint8_t * ) pxDeviceInformation->xCurrentUpdateId.ucProvider,
        ( int32_t ) pxDeviceInformation->xCurrentUpdateId.ulProviderLength );
    pxBaseDeviceInformation->update_id.version = az_span_create(
        ( uint8_t * ) pxDeviceInformation->xCurrentUpdateId.ucVersion,
        ( int32_t ) pxDeviceInformation->xCurrentUpdateId.ulVersionLength );
    pxBaseDeviceInformation->adu_version = AZ_SPAN_FROM_STR( AZ_IOT_ADU_CLIENT_AGENT_VERSION );
    pxBaseDeviceInformation->do_version = AZ_SPAN_EMPTY;
}

static void prvFillBaseAduWorkflow( AzureIoTADUUpdateRequest_t * pxAduUpdateRequest,
                                    az_iot_adu_client_workflow * pxBaseWorkflow )
{
    if( pxAduUpdateRequest != NULL )
    {
        pxBaseWorkflow->action = ( int32_t ) pxAduUpdateRequest->xWorkflow.xAction;
        pxBaseWorkflow->id = az_span_create(
            ( uint8_t * ) pxAduUpdateRequest->xWorkflow.pucID,
            ( int32_t ) pxAduUpdateRequest->xWorkflow.ulIDLength );
        pxBaseWorkflow->retry_timestamp = az_span_create(
            ( uint8_t * ) pxAduUpdateRequest->xWorkflow.pucRetryTimestamp,
            ( int32_t ) pxAduUpdateRequest->xWorkflow.ulRetryTimestampLength );
    }
}

static void prvFillBaseAduInstallResults( AzureIoTHubClientADUInstallResult_t * pxUpdateResults,
                                          az_iot_adu_client_install_result * pxBaseInstallResults )
{
    memset( pxBaseInstallResults, 0, sizeof( *pxBaseInstallResults ) );

    if( pxUpdateResults != NULL )
    {
        pxBaseInstallResults->result_code = pxUpdateResults->lResultCode;
        pxBaseInstallResults->extended_result_code = pxUpdateResults->lExtendedResultCode;

        if( ( pxUpdateResults->pucResultDetails != NULL ) &&
            ( pxUpdateResults->ulResultDetailsLength > 0 ) )
        {
            pxBaseInstallResults->result_details = az_span_create(
                ( uint8_t * ) pxUpdateResults->pucResultDetails,
                ( int32_t ) pxUpdateResults->ulResultDetailsLength );
        }
        else
        {
            pxBaseInstallResults->result_details = AZ_SPAN_EMPTY;
        }

        pxBaseInstallResults->step_results_count = ( int32_t ) pxUpdateResults->ulStepResultsCount;

        for( int lIndex = 0; lIndex < ( int32_t ) pxUpdateResults->ulStepResultsCount; lIndex++ )
        {
            pxBaseInstallResults->step_results[ lIndex ].result_code =
                ( int32_t ) pxUpdateResults->pxStepResults[ lIndex ].ulResultCode;
            pxBaseInstallResults->step_results[ lIndex ].extended_result_code =
                ( int32_t ) pxUpdateResults->pxStepResults[ lIndex ].ulExtendedResultCode;

            if( ( pxUpdateResults->pxStepResults[ lIndex ].pucResultDetails != NULL ) &&
                ( pxUpdateResults->pxStepResults[ lIndex ].ulResultDetailsLength > 0 ) )
            {
                pxBaseInstallResults->step_results[ lIndex ].result_details = az_span_create(
                    ( uint8_t * ) pxUpdateResults->pxStepResults[ lIndex ].pucResultDetails,
                    ( int32_t ) pxUpdateResults->pxStepResults[ lIndex ].ulResultDetailsLength
                    );
            }
            else
            {
                pxBaseInstallResults->step_results[ lIndex ].result_details = AZ_SPAN_EMPTY;
            }
        }
    }
}

AzureIoTResult_t AzureIoTADUClient_SendAgentState( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                                   AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                   AzureIoTHubClientADUDeviceInformation_t * pxDeviceInformation,
                                                   AzureIoTADUUpdateRequest_t * pxAduUpdateRequest,
                                                   AzureIoTADUAgentState_t xAgentState,
                                                   AzureIoTHubClientADUInstallResult_t * pxUpdateResults,
                                                   uint8_t * pucBuffer,
                                                   uint32_t ulBufferSize,
                                                   uint32_t * pulRequestId )
{
    az_iot_adu_client_device_information xBaseDeviceInformation;
    az_iot_adu_client_workflow xBaseWorkflow;
    az_iot_adu_client_install_result xInstallResult; /* TODO: fill up. */
    az_span xPropertiesPayload = az_span_create( pucBuffer, ( int32_t ) ulBufferSize );

    prvFillBaseDeviceInformation( pxDeviceInformation, &xBaseDeviceInformation );
    prvFillBaseAduWorkflow( pxAduUpdateRequest, &xBaseWorkflow );
    prvFillBaseAduInstallResults( pxUpdateResults, &xInstallResult );

    az_result xAzResult = az_iot_adu_client_get_agent_state_payload(
        &pxAzureIoTADUClient->_internal.xADUClient,
        &xBaseDeviceInformation,
        ( int32_t ) xAgentState,
        pxAduUpdateRequest != NULL ? &xBaseWorkflow : NULL,
        pxUpdateResults != NULL ? &xInstallResult : NULL,
        xPropertiesPayload,
        &xPropertiesPayload
        );

    if( az_result_failed( xAzResult ) )
    {
        AZLogError( ( "az_iot_adu_client_get_agent_state_payload failed: 0x%08x", xAzResult ) );
        /* TODO: return individualized/specific errors. */
        return eAzureIoTErrorFailed;
    }

    if( AzureIoTHubClient_SendPropertiesReported(
            pxAzureIoTHubClient,
            az_span_ptr( xPropertiesPayload ),
            ( uint32_t ) az_span_size( xPropertiesPayload ),
            pulRequestId ) != eAzureIoTSuccess )
    {
        AZLogError( ( "[ADU] Failed sending ADU agent state." ) );
        return eAzureIoTErrorPublishFailed;
    }

    /*Send state update about the stage of the ADU process we are in */
    return eAzureIoTSuccess;
}
