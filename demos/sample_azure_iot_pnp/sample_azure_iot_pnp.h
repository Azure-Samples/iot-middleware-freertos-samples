/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @brief Defines an interface to be used by samples when interacting with sample_azure_iot_pnp.c module.
 */

#ifndef SAMPLE_AZURE_IOT_PNP_H
#define SAMPLE_AZURE_IOT_PNP_H

#include <stdbool.h>
#include <stdint.h>

#include "azure_iot_hub_client_properties.h"

/*
 * Header demo_config.h MUST define the Plug and Play Model ID
 * through the sampleazureiotMODEL_ID configuration.
 */
#ifndef sampleazureiotMODEL_ID
    /**
     * @brief The model id for this device
     *
     * https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/samples/Thermostat.json
     *
     * The model id is the JSON document (also called the Digital Twins Model Identifier or DTMI) which
     * defines the capability of your device. The functionality of the device should match what is
     * described in the corresponding DTMI. Should you choose to program your own PnP capable device,
     * the functionality would need to match the DTMI and you would need to update the below 'model_id'.
     * Please see the sample README for more information on this DTMI.
     *
     */
    #define sampleazureiotMODEL_ID                                "dtmi:com:example:Thermostat;1"
#endif

extern AzureIoTHubClient_t xAzureIoTHubClient;

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
 * @brief Handles a property message received from the Azure IoT Hub.
 *
 * @remark This function must be implemented by the specific sample.
 *
 * @param[in] pxMessage Pointer to a structure that holds the Properties received.
 * @param[in] pvContext Context defined by the sample.
 */
void vHandleProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                        void * pvContext );

/**
 * @brief Sends a reported properties update to Azure IoT Hub.
 * 
 * @remark This function can be used by a specific sample to update the device's reported properties.
 *
 * @param[in] pucProperties      Reported properties content to be updated.
 * @param[in] ulPropertiesLength Length of `pucProperties`.
 * 
 * @return uint32_t Zero if successful, non-zero if any failure occurs.
 */
uint32_t ulSendPropertiesUpdate( uint8_t* pucProperties, 
                                 uint32_t ulPropertiesLength );

#endif /* ifndef SAMPLE_AZURE_IOT_PNP_H */
