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

#define azureiotaduWORKFLOW_ID_SIZE                             48
#define azureiotaduWORKFLOW_RETRY_TIMESTAMP_SIZE                80
#define azureiotaduSTEPS_MAX                                    2
#define azureiotaduAGENT_FILES_MAX                              2
#define azureiotaduRESULT_DETAILS_SIZE                          128
#define azureiotaduDEVICE_INFO_MANUFACTURER_SIZE                16
#define azureiotaduDEVICE_INFO_MODEL_SIZE                       24
#define azureiotaduDEVICE_INFO_LAST_INSTALLED_UPDATE_ID_SIZE    128
#define azureiotaduSTEP_ID_SIZE                                 32

/**
 * @brief ADU Device Information.
 *
 *  https://docs.microsoft.com/en-us/azure/iot-hub-device-update/understand-device-update#device-update-agent
 */
typedef struct AzureIoTHubClientADUDeviceInformation
{
    const uint8_t ucManufacturer[ azureiotaduDEVICE_INFO_MANUFACTURER_SIZE ];
    uint32_t ulManufacturerLength;

    const uint8_t ucModel[ azureiotaduDEVICE_INFO_MODEL_SIZE ];
    uint32_t ulModelLength;

    const uint8_t ucLastInstalledUpdateId[ azureiotaduDEVICE_INFO_LAST_INSTALLED_UPDATE_ID_SIZE ];
    uint32_t ulLastInstalledUpdateIdLength;
} AzureIoTHubClientADUDeviceInformation_t;

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
    int32_t ulAction;

    const uint8_t ucID[ azureiotaduWORKFLOW_ID_SIZE ];
    uint32_t ulIDLength;

    const uint8_t ucRetryTimestamp[ azureiotaduWORKFLOW_RETRY_TIMESTAMP_SIZE ];
    uint32_t ulRetryTimestampLength;
} AzureIoTHubClientADUWorkflow_t;

typedef struct AzureIoTHubClientADUStepResult
{
    const uint8_t ucStepId[ azureiotaduSTEP_ID_SIZE ];
    uint32_t ulStepIdLength;

    uint32_t ulResultCode;
    uint32_t ulExtendedResultCode;

    const uint8_t ucResultDetails[ azureiotaduRESULT_DETAILS_SIZE ];
    uint32_t ulResultDetailsLength;
} AzureIoTHubClientADUStepResult_t;

typedef struct AzureIoTHubClientADUInstallResult
{
    int32_t lResultCode;
    int32_t lExtendedResultCode;

    const uint8_t ucResultDetails[ azureiotaduRESULT_DETAILS_SIZE ];
    uint32_t ulResultDetailsLength;

    AzureIoTHubClientADUStepResult_t * pxStepResults;
    uint32_t ulStepResultsCount;
} AzureIoTHubClientADUInstallResult_t;

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

/**
 * @brief Azure IoT ADU Client (ADU agent) to handle stages of the ADU process.
 */
typedef struct AzureIoTADUClient
{
    AzureIoTHubClient_t * pxHubClient;
    AzureIoTADUState_t xState;
    AzureIoTADUUpdateStepState_t xUpdateStepState;
    AzureIoTHTTP_t xHTTP;
    AzureADUImage_t xImage;
    AzureIoT_TransportConnectCallback_t xHTTPConnectCallback;
    AzureIoTTransportInterface_t * pxHTTPTransport;
    uint8_t * pucAduContextBuffer;
    uint32_t ulAduContextBufferLength;
    az_span xAduContextAvailableBuffer;
    const AzureIoTHubClientADUDeviceInformation_t * pxDeviceInformation;
    const AzureIoTHubClientADUInstallResult_t * pxLastInstallResult;
    az_iot_adu_ota_update_request xUpdateRequest;
    az_iot_adu_ota_update_manifest xUpdateManifest;
    /* TODO: remove this hack, and use xState instead. */
    bool xSendProperties;
} AzureIoTADUClient_t;

/**
 * @brief Callback which will be invoked to connect to the HTTP endpoint to download the new image.
 *
 */

/**
 * @brief Initializes the Azure IoT ADU Agent Client.
 * 
 * @param[in] pxAduClient            The pointer to the #AzureIoTADUClient_t
 *                                   instance to initialize.
 * @param[in] pxAzureIoTHubClient    A pointer to the #AzureIoTHubClient_t,
 *                                   already initialized.
 * @param[in] pxHTTPTransport        An instance of #AzureIoTTransportInterface_t
 *                                   defining the I/O interface for the internal
 *                                   HTTP client used to download image files.  
 * @param[in] pxAzureIoTHTTPConnectCallback
 *                                  A callback invoked by the ADU client API
 *                                  requesting the socket connection to be
 *                                  established for the internal HTTP client.
 * @param[in] pxDeviceInformation   A pointer to a
 *                                  #AzureIoTHubClientADUDeviceInformation_t
 *                                  structure with all the details of the device,
 *                                  as required by the ADU service.
 * @param[in] pxLastInstallResult   A pointer to a
 *                                  #AzureIoTHubClientADUInstallResult_t
 *                                  containing the results of the current or last
 *                                  update and its steps.
 * @param[in] pucAduContextBuffer   A pointer to the memory buffer to be used
 *                                  for parsing ADU service requests and
 *                                  composing the ADU agent reports.  
 * @param[in] ulAduContextBuffer    The size of `pucAduContextBuffer`.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTADUClient_Init( AzureIoTADUClient_t * pxAduClient,
                                         AzureIoTHubClient_t * pxAzureIoTHubClient,
                                         AzureIoTTransportInterface_t * pxHTTPTransport,
                                         AzureIoT_TransportConnectCallback_t pxAzureIoTHTTPConnectCallback,
                                         const AzureIoTHubClientADUDeviceInformation_t * pxDeviceInformation,
                                         const AzureIoTHubClientADUInstallResult_t * pxLastInstallResult,
                                         uint8_t * pucAduContextBuffer,
                                         uint32_t ulAduContextBuffer );

/**
 * @brief Process ADU Messages and iterate through ADU state machine.
 * @remark This function must be called frequently enough to properly
 *         allow the ADU client and state machine to perform the
 *         device updates when they are received. This is usually called
 *         side-by-side with AzureIoTHubClient_ProcessLoop.
 *
 * @param[in] pxAduClient The #AzureIoTADUClient_t * to use for this call.
 * @param[in] ulTimeoutMilliseconds Minimum time (in milliseconds) for the
 *                                  loop to run. If `0` is passed,
 *                                  it will only run once.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTADUClient_ADUProcessLoop( AzureIoTADUClient_t * pxAduClient,
                                                   uint32_t ulTimeoutMilliseconds );

/**
 * @brief Updates the ADU Agent Client with ADU service device update properties.
 * @remark It must be called whenever writable properties are received containing
 *         ADU service properties (verified with AzureIoTADUClient_IsADUComponent).
 *         It effectively parses the properties (aka, the device update request)
 *         from ADU and sets the state machine to perform the update process if the
 *         the update request is applicable (e.g., if the version is not already
 *         installed).
 *         This function also provides the payload to acknowledge the ADU service
 *         Azure Plug-and-Play writable properties.   
 * 
 * @param[in] pxAduClient The #AzureIoTADUClient_t * to use for this call.
 * @param[in] pxReader  A #AzureIoTJSONReader_t initialized with the ADU
 *                      service writable properties json, set to the
 *                      beginning of the json object that is the value
 *                      of the ADU component.
 * @param[in] ulPropertyVersion Version of the writable properties.
 * @param[in] pucWritablePropertyResponseBuffer
 *              An pointer to the memory buffer where to
 *              write the resulting Azure Plug-and-Play properties acknoledgement
 *              payload.
 * @param[in] ulWritablePropertyResponseBufferSize
 *              Size of `pucWritablePropertyResponseBuffer`
 * @param[out] pulWritablePropertyResponseBufferLength
 *              Length of content writen into `pucWritablePropertyResponseBuffer`.
 * @return An #AzureIoTResult_t with the result of the operation.
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
 *       AzureIoTHubClient_ADUProcessComponent() call. The properties will be
 *       processed into the AzureIoTADUClient.
 *
 * @param[in] pxAduClient The pointer to the instance of #AzureIoTADUClient_t
 *                        previously initialized.
 * @param[in] pucComponentName Name of writable properties component to be
 *                             checked.
 * @param[in] ulComponentNameLength    Length of `pucComponentName`.
 * @return A boolean value indicating if the writable properties component
 *         is from ADU service.
 */
bool AzureIoTADUClient_IsADUComponent( AzureIoTADUClient_t * pxAduClient,
                                       const char * pucComponentName,
                                       uint32_t ulComponentNameLength );

/**
 * @brief Get the current state of the ADU Client.
 *
 * @param[in] pxAduClient The pointer to the instance of #AzureIoTADUClient_t
 *                        previously initialized.
 * @return A value of #AzureIoTADUUpdateStepState_t indicating the current state
 *         of the ADU client.
 */
AzureIoTADUUpdateStepState_t AzureIoTADUClient_GetState( AzureIoTADUClient_t * pxAduClient );

#endif /* AZURE_IOT_ADU_CLIENT_H */
