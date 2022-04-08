/**
 * @file azure_iot_adu_client.h
 *
 * @brief
 *
 */
#ifndef AZURE_IOT_ADU_CLIENT_H
#define AZURE_IOT_ADU_CLIENT_H

#include <stdint.h>

#include "azure_iot_result.h"
#include "azure_iot_hub_client.h"
#include "azure_iot_json_reader.h"
#include "azure_iot_http.h"
#include <azure/iot/az_iot_adu_ota.h>
#include "azure_iot_flash_platform.h"


/**************************ADU SECTION******************************** // */

/* We have to parse out these values below. */

/*
 *
 * Device Twin for ADU
 *
 * {
 *  "deviceUpdate": {
 *      "__t": "c",
 *      "agent": {
 *          "deviceProperties": {
 *              "manufacturer": "contoso",
 *              "model": "virtual-vacuum-v1",
 *              "interfaceId": "dtmi:azure:iot:deviceUpdate;1",
 *              "aduVer": "DU;agent/0.8.0-rc1-public-preview",
 *              "doVer": "DU;lib/v0.6.0+20211001.174458.c8c4051,DU;agent/v0.6.0+20211001.174418.c8c4051"
 *          },
 *          "compatPropertyNames": "manufacturer,model",
 *          "lastInstallResult": {
 *              "resultCode": 700,
 *              "extendedResultCode": 0,
 *              "resultDetails": "",
 *              "stepResults": {
 *                  "step_0": {
 *                      "resultCode": 700,
 *                      "extendedResultCode": 0,
 *                      "resultDetails": ""
 *                  }
 *              }
 *          },
 *          "state": 0,
 *          "workflow": {
 *              "action": 3,
 *              "id": "11b6a7c3-6956-4b33-b5a9-87fdd79d2f01",
 *              "retryTimestamp": "2022-01-26T11:33:29.9680598Z"
 *          },
 *          "installedUpdateId": "{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"5.0\"}"
 *      }
 *  }
 * }
 */


/*Update Manifest */

/*
 *
 * {
 *  "manifestVersion": "4",
 *  "updateId": {
 *      "provider": "Contoso",
 *      "name": "Toaster",
 *      "version": "1.0"
 *  },
 *  "compatibility": [{
 *      "deviceManufacturer": "Contoso",
 *      "deviceModel": "Toaster"
 *  }],
 *  "instructions": {
 *      "steps": [{
 *          "handler": "microsoft/swupdate:1",
 *          "handlerProperties": {
 *              "installedCriteria": "1.0"
 *          },
 *          "files": [
 *              "fileId0"
 *          ]
 *      }]
 *  },
 *  "files": {
 *      "fileId0": {
 *          "filename": "contoso.toaster.1.0.swu",
 *          "sizeInBytes": 718,
 *          "hashes": {
 *              "sha256": "mcB5SexMU4JOOzqmlJqKbue9qMskWY3EI/iVjJxCtAs="
 *          }
 *      }
 *  },
 *  "createdDateTime": "2021-09-28T18:32:01.8404544Z"
 * }
 *
 */

#define azureiotaduWORKFLOW_ID_SIZE                 48
#define azureiotaduWORKFLOW_RETRY_TIMESTAMP_SIZE    80
#define azureiotaduSTEPS_MAX                        2
#define azureiotaduAGENT_FILES_MAX                  2

/**
 * @brief Actions requested by the ADU Service
 *
 * Used to have `Install` (1) and `Apply` (2)
 *
 * Previously in the public preview protocol, the cloud would push a separate UpdateAction for each
 * individual step (e.g. download, install, apply) and the agent would report to the cloud after
 * every workflow step completion.
 * Each of these steps is now a local workflow step in the agent's workflow processing state machine.
 *
 * https://github.com/danewalton/iot-hub-device-update/blob/main/docs/agent-reference/goal-state-support.md#agent-based-vs-cloud-based-orchestration
 *
 * https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-plug-and-play#action
 *
 */
typedef enum AzureIoTADUAction
{
    eAzureIoTADUActionDownload = 0,
    eAzureIoTADUActionApplyDownload = 3,
    eAzureIoTADUActionCancel = 255
} AzureIoTADUAction_t;

/**
 * @brief States of the ADU agent
 *
 * State is reported in response to an Action
 *
 * Used to have `DownloadSucceeded` and `InstallSucceeded` but not anymore
 *
 * State:
 * https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-plug-and-play#state
 *
 * Action:
 * https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-plug-and-play#action
 *
 */
typedef enum AzureIoTADUState
{
    eAzureIoTADUStateIdle = 0,
    eAzureIoTADUStateDeploymentInProgress = 6,
    eAzureIotADUStateFailed = 255,
    eAzureIoTADUStateError,
} AzureIoTADUState_t;

/**
 * @brief Steps taken once the update is requested.
 *
 * Internal (not relevant to server) steps taken to go through update process.
 *
 */
typedef enum AzureIoTADUUpdateStepState
{
    eAzureIoTADUUpdateStepIdle = 0,
    eAzureIoTADUUpdateStepManifestDownloadStarted = 1,
    eAzureIoTADUUpdateStepManifestDownloadSucceeded = 2,
    eAzureIoTADUUpdateStepFirmwareDownloadStarted = 3,
    eAzureIoTADUUpdateStepFirmwareDownloadSucceeded = 4,
    eAzureIoTADUUpdateStepFirmwareInstallStarted = 5,
    eAzureIoTADUUpdateStepFirmwareInstallSucceeded = 6,
    eAzureIoTADUUpdateStepFirmwareApplyStarted = 7,
    eAzureIoTADUUpdateStepFirmwareApplySucceeded = 8,
    eAzureIoTADUUpdateStepFailed = 255
} AzureIoTADUUpdateStepState_t;

typedef AzureIoTResult_t (* AzureIoT_TransportConnectCallback_t)( AzureIoTTransportInterface_t * pxAzureIoTTransport,
                                                                  const char * pucURL );


typedef struct AzureIoTUpdateID
{
    uint32_t xUnused;
} AzureIoTUpdateID_t;

/**
 * @brief ADU workflow struct.
 * Format:
 *    {
 *      "action": 3,
 *      "id": "someguid",
 *      "retryTimestamp": "2020-04-22T12:12:12.0000000+00:00"
 *  }
 *
 *  https://docs.microsoft.com/en-us/azure/iot-hub-device-update/understand-device-update#device-update-agent
 */
typedef struct AzureIoTHubClientADUWorkflow
{
    uint32_t ulAction;

    const uint8_t ucID[ azureiotaduWORKFLOW_ID_SIZE ];
    uint32_t ulIDLength;

    const uint8_t ucRetryTimestamp[ azureiotaduWORKFLOW_RETRY_TIMESTAMP_SIZE ];
    uint32_t ulRetryTimestampLength;
} AzureIoTHubClientADUWorkflow_t;

typedef struct AzureIoTHubClientADUStepResult
{
    uint32_t ulResultCode;
    uint32_t ulExtendedResultCode;
} AzureIoTHubClientADUStepResult_t;

/**
 * @brief ADU update manifest steps struct.
 */
typedef struct AzureIoTHubClientADUStep_t
{
    /* Type.  */
    const uint8_t * pucType;
    uint32_t ulTypeLength;

    /* Handler.  */
    const uint8_t * pucHandler;
    uint32_t ulHandlerLength;

    /* File id. */
    const uint8_t * pucFileID;
    uint32_t ulFileIDLength;

    /* Step state.  */
    uint32_t ulState;

    /* Result.  */
    AzureIoTHubClientADUStepResult_t xResult;
} AzureIoTHubClientADUStep_t_t;

typedef struct AzureIoTHubClientADUFile
{
    /* File number.  */
    const uint8_t * pucFileID;
    uint32_t ulFileIDLength;

    /* File name.  */
    const uint8_t * pucFileName;
    uint32_t ulFileNameLength;

    /* File size in bytes.  */
    uint32_t ulFileSizeInBytes;

    /* File sha256.  */
    const uint8_t * pucFileSHA256;
    uint32_t ulFileSHA256Length;

    /* File url.  */
    const uint8_t * pucFileURL;
    uint32_t ulFileURLLength;
} AzureIoTHubClientADUFile_t;

typedef struct AzureIoTADUClientADUManifest
{
    /* Manifest version.  */
    const uint8_t * pucManifestVersion;
    uint32_t pulManifestVersionLength;

    /* Update Id.  */
    AzureIoTUpdateID_t xUpdateID;

    /* Compatibility: deviceManufacturer.  */
    const uint8_t * pucDeviceManufacturer;
    uint32_t pulDeviceManufacturerLength;

    /* Compatibility: deviceModel.  */
    const uint8_t * pucDeviceModel;
    uint32_t pulDeviceModelLength;

    /* Compatibility: group.  */
    const uint8_t * pucGroup;
    uint32_t pulGroupLength;

    /* Instructions: steps.  */
    AzureIoTHubClientADUStep_t_t pxSteps[ azureiotaduSTEPS_MAX ];
    uint32_t ulStepsCount;

    /* Files.  */
    AzureIoTHubClientADUFile_t pxFiles[ azureiotaduAGENT_FILES_MAX ];
    uint32_t ulFilesCount;
} AzureIoTADUClientADUManifest_t;

/**
 * @brief ADU Client to handle stages of the ADU process.
 *
 */
typedef struct AzureIoTADUClient
{
    AzureIoTHubClient_t * pxHubClient;
    AzureIoTADUState_t xState;
    AzureIoTADUUpdateStepState_t xUpdateStepState;
    AzureIoTADUClientADUManifest_t xManifest;
    AzureIoTHTTP_t xHTTP;
    AzureADUImage_t xImage;
    AzureIoT_TransportConnectCallback_t xHTTPConnectCallback;
    AzureIoTTransportInterface_t * pxHTTPTransport;
    const uint8_t * pucAduContextBuffer;
    uint32_t ulAduContextBufferLength;
    az_iot_adu_ota_update_request xUpdateRequest;
    az_span xAduContextBuffer;
    az_iot_adu_ota_update_manifest xUpdateManifest;
} AzureIoTADUClient_t;

/**
 * @brief Callback which will be invoked to connect to the HTTP endpoint to download the new image.
 *
 */

/**
 * @brief Initialize Azure IoT ADU Client
 *
 * @param pxAduClient
 * @param pxAzureIoTHubClient
 * @param pucAduContextBuffer
 * @param ulAduContextBuffer
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTADUClient_Init( AzureIoTADUClient_t * pxAduClient,
                                         AzureIoTHubClient_t * pxAzureIoTHubClient,
                                         AzureIoTTransportInterface_t * pxAzureIoTTransport,
                                         AzureIoT_TransportConnectCallback_t pxAzureIoTHTTPConnectCallback,
                                         const uint8_t * pucAduContextBuffer,
                                         uint32_t ulAduContextBuffer );

/**
 * @brief Process ADU Messages and iterate through ADU state machine.
 *
 * @param[in] pxAduClient The #AzureIoTADUClient_t * to use for this call.
 * @param[in] ulTimeoutMilliseconds Minimum time (in milliseconds) for the loop to run. If `0` is passed, it will only run once.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTADUClient_ADUProcessLoop( AzureIoTADUClient_t * pxAduClient,
                                                   uint32_t ulTimeoutMilliseconds );

/**
 * @brief Process the ADU subcomponent into the AzureIoTADUClient
 *
 * @param pxAduClient
 * @param pxReader
 * @param ulPropertyVersion
 * @param pucWritablePropertyResponseBuffer
 * @param ulWritablePropertyResponseBufferSize
 * @param pulWritablePropertyResponseBufferLength
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTADUClient_ADUProcessComponent( AzureIoTADUClient_t * pxAduClient,
                                                        AzureIoTJSONReader_t * pxReader,
                                                        uint32_t ulPropertyVersion,
                                                        uint8_t * pucWritablePropertyResponseBuffer,
                                                        uint32_t ulWritablePropertyResponseBufferSize,
                                                        uint32_t * pulWritablePropertyResponseBufferLength );

/**
 * @brief Returns whether the component is the ADU component
 *
 * @note If it is, user should follow by parsing the component with the
 * AzureIoTHubClient_ADUProcessComponent() call. The properties will be
 * processed into the AzureIoTADUClient.
 *
 * @param pucComponentName
 * @param ulComponentNameLength
 * @return true
 * @return false
 */
bool AzureIoTADUClient_IsADUComponent( AzureIoTADUClient_t * pxAduClient,
                                       const char * pucComponentName,
                                       uint32_t ulComponentNameLength );

/**
 * @brief Get the state of the ADU Client
 *
 * @param pxAduClient
 * @return AzureIoTADUUpdateStepState_t
 */
AzureIoTADUUpdateStepState_t AzureIoTADUClient_GetState( AzureIoTADUClient_t * pxAduClient );

#endif /* AZURE_IOT_ADU_CLIENT_H */
