/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @brief Defines an interface to be used by samples when interacting with sample_azure_iot_pnp.c module.
 *        This interface allows the module implementing the specific plug-and-play model to exchange data
 *        with the module responsible for communicating with Azure IoT Hub.
 */

#ifndef SAMPLE_AZURE_IOT_PNP_DATA_IF_H
#define SAMPLE_AZURE_IOT_PNP_DATA_IF_H

#include <stdbool.h>
#include <stdint.h>

#include "azure_iot_adu_client.h"
#include "azure_iot_hub_client_properties.h"
#include "demo_config.h"

extern AzureIoTHubClient_t xAzureIoTHubClient;
extern AzureIoTADUClient_t xAzureIoTADUClient;
extern AzureIoTADUUpdateRequest_t xAzureIoTAduUpdateRequest;
extern bool xProcessUpdateRequest;
extern AzureIoTADUClientDeviceProperties_t xADUDeviceProperties;

/**
 * @brief Provides the payload to be sent as telemetry to the Azure IoT Hub.
 *
 * @remark This function must be implemented by the specific sample.
 *         `ulCreateTelemetry` is called periodically by the sample core task (the task created by `vStartDemoTask`).
 *         If `pulTelemetryDataLength` returned is zero, telemetry is not send to the Azure IoT Hub.
 *
 * @param[out]  pucTelemetryData        Pointer to uint8_t* that will contain the Telemetry payload.
 * @param[in]   ulTelemetryDataSize     Size of `pucTelemetryData`
 * @param[out]  pulTelemetryDataLength  The number of bytes written in `pucTelemetryData`
 *
 * @return uint32_t Zero if successful, non-zero if any failure occurs.
 */
uint32_t ulCreateTelemetry( uint8_t * pucTelemetryData,
                            uint32_t ulTelemetryDataSize,
                            uint32_t * pulTelemetryDataLength );

/**
 * @brief Provides the payload to be sent as reported properties update to the Azure IoT Hub.
 *
 * @remark This function must be implemented by the specific sample.
 *         `ulCreateReportedPropertiesUpdate` is called periodically by the sample
 *         core task (the task created by `vStartDemoTask`).
 *         If the sample does not have any properties to update, just return zero to inform no
 *         update should be sent.
 *
 * @param[out] pucPropertiesData    Pointer to uint8_t* that will contain the reported properties payload.
 * @param[in]  ulPropertiesDataSize Size of `pucPropertiesData`
 *
 * @return uint32_t The number of bytes written in `pucPropertiesData`.
 */
uint32_t ulCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                           uint32_t ulPropertiesDataSize );

/**
 * @brief Handles a Command received from the Azure IoT Hub.
 *
 * @remark This function must be implemented by the specific sample.
 *
 * @param[in]  pxMessage                          Pointer to a structure that holds details of the Command.
 * @param[out] pulResponseStatus                  Status code to be sent as response for Command request.
 * @param[out] pucCommandResponsePayloadBuffer    Buffer in which to write a payload for the Command response.
 * @param[in]  ulCommandResponsePayloadBufferSize Total size of `ucCommandResponsePayloadBuffer`.
 *
 * @return uint32_t Number of bytes written to `ucCommandResponsePayloadBuffer`.
 */
uint32_t ulHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                          uint32_t * pulResponseStatus,
                          uint8_t * pucCommandResponsePayloadBuffer,
                          uint32_t ulCommandResponsePayloadBufferSize );

/**
 * @brief Handles a properties message received from the Azure IoT Hub (writable or get response).
 *
 * @remark This function must be implemented by the specific sample.
 *
 * @param[in]  pxMessage                               Pointer to a structure that holds the Writable Properties received.
 * @param[out] pucWritablePropertyResponseBuffer       Buffer where to write the response for the property update.
 * @param[out] ulWritablePropertyResponseBufferSize    Size of `pucWritablePropertyResponseBuffer`.
 * @param[out] pulWritablePropertyResponseBufferLength Number of bytes written into `pucWritablePropertyResponseBuffer`.
 */
void vHandleWritableProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                uint8_t * pucWritablePropertyResponseBuffer,
                                uint32_t ulWritablePropertyResponseBufferSize,
                                uint32_t * pulWritablePropertyResponseBufferLength );

#endif /* ifndef SAMPLE_AZURE_IOT_PNP_DATA_IF_H */
