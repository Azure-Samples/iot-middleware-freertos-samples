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
#include <azure/iot/az_iot_adu_client.h>


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

/* TODO: repurpose these, setting them to the values from azure-sdk-for-c */
#define azureiotaduSTEPS_MAX                        2
#define azureiotaduAGENT_FILES_MAX                  2
#define azureiotaduDEVICE_INFO_MANUFACTURER_SIZE    16
#define azureiotaduDEVICE_INFO_MODEL_SIZE           24
#define azureiotaduUPDATE_PROVIDER_SIZE             16
#define azureiotaduUPDATE_NAME_SIZE                 24
#define azureiotaduUPDATE_VERSION_SIZE              10

/**
 * @brief ADU Update ID.
 *
 *  https://docs.microsoft.com/en-us/azure/iot-hub-device-update/understand-device-update#device-update-agent
 */
/* TODO: remove this in favor of AzureIoTADUUpdateId_t */
typedef struct AzureIoTHubClientADUUpdateId
{
    const uint8_t ucProvider[ azureiotaduUPDATE_PROVIDER_SIZE ];
    uint32_t ulProviderLength;

    const uint8_t ucName[ azureiotaduUPDATE_NAME_SIZE ];
    uint32_t ulNameLength;

    const uint8_t ucVersion[ azureiotaduUPDATE_VERSION_SIZE ];
    uint32_t ulVersionLength;
} AzureIoTHubClientADUUpdateId_t;

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

    AzureIoTHubClientADUUpdateId_t xCurrentUpdateId;
} AzureIoTHubClientADUDeviceInformation_t;

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
    AzureIoTADUAction_t xAction;

    const uint8_t * pucID;
    uint32_t ulIDLength;

    const uint8_t * pucRetryTimestamp;
    uint32_t ulRetryTimestampLength;
} AzureIoTHubClientADUWorkflow_t;

typedef struct AzureIoTHubClientADUStepResult
{
    uint32_t ulResultCode;
    uint32_t ulExtendedResultCode;

    const uint8_t * pucResultDetails;
    uint32_t ulResultDetailsLength;
} AzureIoTHubClientADUStepResult_t;

typedef struct AzureIoTHubClientADUInstallResult
{
    int32_t lResultCode;
    int32_t lExtendedResultCode;

    const uint8_t * pucResultDetails;
    uint32_t ulResultDetailsLength;

    AzureIoTHubClientADUStepResult_t pxStepResults[ AZ_IOT_ADU_CLIENT_MAX_INSTRUCTIONS_STEPS ];
    uint32_t ulStepResultsCount;
} AzureIoTHubClientADUInstallResult_t;

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
typedef enum AzureIoTADUAgentState
{
    eAzureIoTADUAgentStateIdle = 0,
    eAzureIoTADUAgentStateDeploymentInProgress = 6,
    eAzureIoTADUAgentStateFailed = 255,
    eAzureIoTADUAgentStateError,
} AzureIoTADUAgentState_t;

/* TODO: clean everything that can be removed from this point above. */

typedef enum AzureIoTADURequestDecision
{
    eAzureIoTADURequestDecisionAccept,
    eAzureIoTADURequestDecisionReject
} AzureIoTADURequestDecision_t;

typedef struct AzureIoTADUUpdateManifestFileUrl
{
    uint8_t * pucId;
    uint32_t ulIdLength;
    uint8_t * pucUrl;
    uint32_t ulUrlLength;
} AzureIoTADUUpdateManifestFileUrl_t;

typedef struct AzureIoTADUUpdateId
{
    uint8_t * pucProvider;
    uint32_t ulProviderLength;
    uint8_t * pucName;
    uint32_t ulNameLength;
    uint8_t * pucVersion;
    uint32_t ulVersionLength;
} AzureIoTADUUpdateId_t;

typedef struct AzureIoTADUCompatibility
{
    uint8_t * pucCompatibilityPropertiesNames[ AZ_IOT_ADU_CLIENT_MAX_COMPATIBILITY_PROPERTIES_COUNT ];
    uint32_t pulCompatibilityPropertiesNamesLengths[ AZ_IOT_ADU_CLIENT_MAX_COMPATIBILITY_PROPERTIES_COUNT ];
    uint8_t * pucCompatibilityPropertiesValues[ AZ_IOT_ADU_CLIENT_MAX_COMPATIBILITY_PROPERTIES_COUNT ];
    uint32_t pulCompatibilityPropertiesValuesLengths[ AZ_IOT_ADU_CLIENT_MAX_COMPATIBILITY_PROPERTIES_COUNT ];
    uint32_t ulCompatibilityPropertiesLength;
} AzureIoTADUCompatibility_t;

typedef struct AzureIoTADUInstructionStepFile
{
    uint8_t * pucFileName;
    uint32_t ulFileNameLength;
} AzureIoTADUUpdateManifestInstructionStepFile_t;

typedef struct AzureIoTADUUpdateManifestFileHash
{
    uint8_t * pucId;
    uint32_t ulIdLength;
    uint8_t * pucHash;
    uint32_t ulHashLength;
} AzureIoTADUUpdateManifestFileHash_t;

typedef struct AzureIoTADUUpdateManifestFile
{
    uint8_t * pucId;
    uint32_t ulIdLength;
    uint8_t * pucFileName;
    uint32_t ulFileNameLength;
    uint32_t ulSizeInBytes;
    uint32_t ulHashesCount;
    AzureIoTADUUpdateManifestFileHash_t pxHashes[ AZ_IOT_ADU_CLIENT_MAX_FILE_HASH_COUNT ];
} AzureIoTADUUpdateManifestFile_t;

typedef struct AzureIoTADUInstructionStep
{
    uint8_t * pucHandler;
    uint32_t ulHandlerLength;
    uint8_t * pucInstalledCriteria;
    uint32_t ulInstalledCriteriaLength;
    uint32_t ulFilesCount;
    AzureIoTADUUpdateManifestInstructionStepFile_t pxFiles[ AZ_IOT_ADU_CLIENT_MAX_FILE_URL_COUNT ];
} AzureIoTADUInstructionStep_t;

typedef struct AzureIoTADUInstructions
{
    uint32_t ulStepsCount;
    AzureIoTADUInstructionStep_t pxSteps[ AZ_IOT_ADU_CLIENT_MAX_INSTRUCTIONS_STEPS ];
} AzureIoTADUInstructions_t;

typedef struct AzureIoTADUUpdateManifest
{
    AzureIoTADUUpdateId_t xUpdateId;
    AzureIoTADUCompatibility_t pxCompatibility[ AZ_IOT_ADU_CLIENT_MAX_COMPATIBILITY_PROPERTIES_BUNDLE_COUNT ];
    uint32_t ulCompatibilityLength;
    AzureIoTADUInstructions_t xInstructions;
    uint32_t ulFilesCount;
    AzureIoTADUUpdateManifestFile_t pxFiles[ AZ_IOT_ADU_CLIENT_MAX_FILE_URL_COUNT ];
    uint8_t * pucManifestVersion;
    uint32_t ulManifestVersionLength;
    uint8_t * pucCreateDateTime;
    uint32_t ulCreateDateTimeLength;
} AzureIoTADUUpdateManifest_t;

/**
 * @brief Azure IoT ADU Client (ADU agent) to handle stages of the ADU process.
 */
typedef struct AzureIoTADUUpdateRequest
{
    AzureIoTHubClientADUWorkflow_t xWorkflow;
    uint8_t * pucUpdateManifestSignature;
    uint32_t ulUpdateManifestSignatureLength;
    uint32_t ulFileUrlCount;
    AzureIoTADUUpdateManifestFileUrl_t pxFileUrls[ AZ_IOT_ADU_CLIENT_MAX_FILE_URL_COUNT ];
    AzureIoTADUUpdateManifest_t xUpdateManifest;
} AzureIoTADUUpdateRequest_t;

typedef struct AzureIoTADUClientOptions
{
    void * xUnused;
}
AzureIoTADUClientOptions_t;

typedef struct AzureIoTADUClient
{
    struct
    {
        az_iot_adu_client xADUClient;
    } _internal;
} AzureIoTADUClient_t;

/**
 * @brief Initialize the Azure IoT ADU Options with default values.
 *
 * @param[out] pxADUClientOptions The #AzureIoTADUClientOptions_t instance to set with default values.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTADUClient_OptionsInit( AzureIoTADUClientOptions_t * pxADUClientOptions );

/**
 * @brief Initialize the Azure IoT ADU Client.
 *
 * @param pxAzureIoTADUClient The #AzureIoTADUClient_t * to use for this call.
 * @param pxADUClientOptions The #AzureIoTADUClientOptions_t for the IoT ADU client instance.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTADUClient_Init( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                         AzureIoTADUClientOptions_t * pxADUClientOptions );

/**
 * @brief Returns whether the component is the ADU component
 *
 * @note If it is, user should follow by parsing the component with the
 *       AzureIoTHubClient_ADUProcessComponent() call. The properties will be
 *       processed into the AzureIoTADUClient.
 *
 * @param[in] pxAzureIoTADUClient The #AzureIoTADUClient_t * to use for this call.
 * @param[in] pucComponentName Name of writable properties component to be
 *                             checked.
 * @param[in] ulComponentNameLength Length of `pucComponentName`.
 * @return A boolean value indicating if the writable properties component
 *         is from ADU service.
 */
bool AzureIoTADUClient_IsADUComponent( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                       const char * pucComponentName,
                                       uint32_t ulComponentNameLength );

/**
 * @brief Parse the ADU update request into the requisite structure.
 *
 * The JSON reader returned to the caller from AzureIoTHubClientProperties_GetNextComponentProperty()
 * should be passed to this API.
 *
 * @param pxAzureIoTADUClient The #AzureIoTADUClient_t * to use for this call.
 * @param pxReader The initialized JSON reader positioned at the beginning of the ADU subcomponent
 * property.
 * @param pxAduUpdateRequest The #AzureIoTADUUpdateRequest_t into which the properties will be parsed.
 * @param pucBuffer Scratch space for the property values to be copied into. The current recommmended size for this
 * buffer is at least 8KB.
 * @param ulBufferSize The size of \p pucBuffer.
 * @return AzureIoTResult_t An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTADUClient_ParseRequest( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                                 AzureIoTJSONReader_t * pxReader,
                                                 AzureIoTADUUpdateRequest_t * pxAduUpdateRequest,
                                                 uint8_t * pucBuffer,
                                                 uint32_t ulBufferSize );

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
 * @param[in] pxAzureIoTADUClient The #AzureIoTADUClient_t * to use for this call.
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param[in] pxReader  A #AzureIoTJSONReader_t initialized with the ADU
 *                      service writable properties json, set to the
 *                      beginning of the json object that is the value
 *                      of the ADU component.
 * @param[in] ulPropertyVersion Version of the writable properties.
 * @param[in] pucWritablePropertyResponseBuffer
 *              An pointer to the memory buffer where to
 *              write the resulting Azure Plug-and-Play properties acknowledgement
 *              payload.
 * @param[in] ulWritablePropertyResponseBufferSize
 *              Size of `pucWritablePropertyResponseBuffer`
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTADUClient_SendResponse( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                                 AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                 AzureIoTADURequestDecision_t xRequestDecision,
                                                 uint32_t ulPropertyVersion,
                                                 uint8_t * pucWritablePropertyResponseBuffer,
                                                 uint32_t ulWritablePropertyResponseBufferSize,
                                                 uint32_t * pulRequestId );

/**
 * @brief Sends the current state of the Azure IoT ADU agent.
 *
 * @param[in] pxAzureIoTADUClient The #AzureIoTADUClient_t * to use for this call.
 * @param[in] pxAzureIoTHubClient The #AzureIoTHubClient_t * to use for this call.
 * @param pxDeviceInformation The device information which will be used to generate the payload.
 * @param pxAduUpdateRequest The current #AzureIoTADUUpdateRequest_t. This can be `NULL` if there isn't currently
 * an update request.
 * @param xAgentState The current #AzureIoTADUAgentState_t.
 * @param pxUpdateResults The current #AzureIoTHubClientADUInstallResult_t. This can be `NULL` if there aren't any
 * results from an update.
 * @param pucBuffer The buffer into which the generated payload will be placed.
 * @param ulBufferSize The length of \p pucBuffer.
 * @param pulRequestId An optional request id to be used for the publish. This can be `NULL`.
 * @return An #AzureIoTResult_t with the result of the operation.
 */
AzureIoTResult_t AzureIoTADUClient_SendAgentState( AzureIoTADUClient_t * pxAzureIoTADUClient,
                                                   AzureIoTHubClient_t * pxAzureIoTHubClient,
                                                   AzureIoTHubClientADUDeviceInformation_t * pxDeviceInformation,
                                                   AzureIoTADUUpdateRequest_t * pxAduUpdateRequest,
                                                   AzureIoTADUAgentState_t xAgentState,
                                                   AzureIoTHubClientADUInstallResult_t * pxUpdateResults,
                                                   uint8_t * pucBuffer,
                                                   uint32_t ulBufferSize,
                                                   uint32_t * pulRequestId );


#endif /* AZURE_IOT_ADU_CLIENT_H */
