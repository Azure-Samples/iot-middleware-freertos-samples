#include "azure_iot_adu_client.h"

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

/**
 * @brief Process and copy out fields in the Device Twin for this update.
 *
 */
AzureIoTResult_t prvAzureIoT_ADUProcessProperties( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                   AzureIoTJSONReader_t * pxReader )
{
    /* Go through Device Twin */
    // TODO: review logic and assign result properly.
    return eAzureIoTErrorFailed;
}

/**
 * @brief Process the update manifest which contains details of the image to download.
 *
 * @param pxAzureIoTHubClient
 * @param pxReader
 * @return AzureIoTResult_t
 */
AzureIoTResult_t prvAzureIoT_ADUProcessUpdateManifest( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                       AzureIoTJSONReader_t * pxReader )
{
    /*Use the context buffer which was passed in options and the */
    /*AzureIoTJSONReader_GetTokenString to copy out relevant values */
    /*(since we will copy values out and act on then outside of the twin message callback) */

    /*Find Update ID */

    /*Find Compatability */

    /*Find Instructions with Steps */

    /*Find Files */

    // TODO: review logic and assign result properly.
    return eAzureIoTErrorFailed;
}

AzureIoTResult_t AzureIoTHubClient_ADUProcessComponent( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                        AzureIoTJSONReader_t * pxReader )
{
    /* Iterate through the JSON and pull out the components that are useful. */
    prvAzureIoT_ADUProcessProperties(pxAzureIoTHubClient, pxReader);

    prvAzureIoT_ADUProcessUpdateManifest(pxAzureIoTHubClient, pxReader);

    // TODO: fix 'error: 'struct <anonymous>' has no member named 'pxADUContext'; did you mean 'xMQTTContext'?' and uncomment.
    // pxAzureIoTHubClient->_internal.pxADUContext.xState = eAzureIoTADUStateDeploymentInProgress;

    // TODO: review logic and assign result properly.
    return eAzureIoTErrorFailed;
}

AzureIoTResult_t prvAzureIoT_ADUSendPropertyUpdate( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                    AzureIoTJSONReader_t * pxReader )
{
    /*Send state update about the stage of the ADU process we are in */
    // TODO: review logic and assign result properly.
    return eAzureIoTErrorFailed;
}

// TODO: validate this.
//       It was failing with 'error: unknown type name 'AzureIoT_ADUContext'; did you mean 'AzureIoT_ADUClient_t'?'.
//       Renamed AzureIoT_ADUContext to AzureIoT_ADUClient_t as it seems to be the expected type. 
AzureIoTResult_t prvHandleSteps( AzureIoT_ADUClient_t * pxAduContext )
{
    switch( pxAduContext->xUpdateStepState )
    {
        case eAzureIoTADUUpdateStepIdle:
        /* We are at the beginning. Kick off the update. */
        case eAzureIoTADUUpdateStepManifestDownloadStarted:
        /* Only used in proxy update */
        case eAzureIoTADUUpdateStepManifestDownloadSucceeded:
        /* Only used in proxy update */
        case eAzureIoTADUUpdateStepFirmwareDownloadStarted:
            // TODO: fix xHTTP issue and uncomment these lines.
            // ulAzureIoTHTTP_Init( pxAduContext->xHTTP, AzureIoTHubClient_ADUManifest_t.pxFiles[ 0 ].pucFileURL );
            // ulAzureIoTHTTP_Request( pxAduContext->xHTTP );
            break;

        case eAzureIoTADUUpdateStepFirmwareDownloadSucceeded:

        case eAzureIoTADUUpdateStepFirmwareInstallStarted:
            // TODO: fix error below and uncomment.
            //       error: implicit declaration of function 'ulAzureIoTPlatform_WriteBlock'
            // ulAzureIoTPlatform_WriteBlock();
            break;

        case eAzureIoTADUUpdateStepFirmwareInstallSucceeded:

        case eAzureIoTADUUpdateStepFirmwareApplyStarted:
            // TODO: fix error below and uncomment.
            //       error: implicit declaration of function 'ulAzureIoTPlatform_EnableImage'
            // ulAzureIoTPlatform_EnableImage();
            break;

        case eAzureIoTADUUpdateStepFirmwareApplySucceeded:

            // TODO: fix error below and uncomment.
            //       error: implicit declaration of function 'ulAzureIoTPlatform_ResetDevice'
            // ulAzureIoTPlatform_ResetDevice();
            break;

        case eAzureIoTADUUpdateStepFailed:
            break;
        default:
            break;
    }

    // TODO: review logic and assign result properly.
    return eAzureIoTErrorFailed;
}

AzureIoTResult_t AzureIoTHubClient_ADUProcessLoop( AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                   uint32_t ulTimeoutMilliseconds )
{
    AzureIoTResult_t xResult;

    // TODO: fix issue below and uncomment
    xResult = eAzureIoTErrorFailed;
    //       error: 'struct <anonymous>' has no member named 'pxADUContext'; did you mean 'xMQTTContext'?
    // switch( pxAzureIoTHubClient->_internal.pxADUContext.xState )
    // {
    //     case eAzureIoTADUStateIdle:
    //         /*Do Nothing */
    //         break;

    //     case eAzureIoTADUStateDeploymentInProgress:
    //         /* The incoming payload has been parsed and the AzureIoT_ADUContext_t is valid. */
    //         prvHandleSteps( pxAzureIoTHubClient->_internal.pxADUContext );
    //         break;

    //     case eAzureIoTADUStateError:
    //         xResult = eAzureIoTErrorFailed;
    //         break;
    // }

    return xResult;
}
