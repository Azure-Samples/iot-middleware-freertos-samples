/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#ifndef AZURE_IOT_FREERTOS_ESP32_SENSORS_H
#define AZURE_IOT_FREERTOS_ESP32_SENSORS_H

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"

/**
 * @brief Generates a json with the device key information.
 * 
 * @param[out] pucPropertiesData       Buffer to write the data on.
 * @param[in]  ulPropertiesDataSize    Size of the buffer.
 * 
 * @return int32_t Number of bytes written in the buffer.
 */
int32_t lGenerateDeviceInfo( uint8_t * pucPropertiesData,
                             uint32_t ulPropertiesDataSize );

/**
 * @brief Command message callback handler
 * 
 * @param[in]  pxMessage                            Command message.
 * @param[out] pulResponseStatus                    Command response status code
 * @param[out] pucCommandResponsePayloadBuffer      Buffer for writing the command response. 
 * @param[in] ulCommandResponsePayloadBufferSize    Size of the response buffer.
 * 
 * @return uint32_t Number of bytes written into the response payload.
 */
uint32_t ulSampleHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                                uint32_t * pulResponseStatus,
                                uint8_t * pucCommandResponsePayloadBuffer,
                                uint32_t ulCommandResponsePayloadBufferSize);

/**
 * @brief Implements the sample interface for generating Telemetry payload.
 * 
 * @param pucTelemetryData Buffer to write telemetry into.
 * @param pucTelemetryData Size of the buffer.
 * 
 * @return uint32_t Number of bytes written into the buffer.
 */
uint32_t ulSampleCreateTelemetry( uint8_t * pucTelemetryData,
                                  uint32_t ulTelemetryDataLength );

/**
 * @brief Handler for writable properties updates.
 * 
 * @param pxMessage The writable properties message. 
 */
void vSampleHandleWritablePropertiesUpdate( AzureIoTHubClientPropertiesResponse_t * pxMessage );

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
uint32_t ulSampleCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                                 uint32_t ulPropertiesDataSize );

#endif // AZURE_IOT_FREERTOS_ESP32_SENSORS_H
