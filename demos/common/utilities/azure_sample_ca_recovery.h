/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdint.h>

#include "azure_iot_result.h"
#include "azure_iot_json_reader.h"

typedef struct AzureIoTCARecovery_TrustBundle
{
    const uint8_t * pucVersion;
    uint32_t ulVersionLength;

    const uint8_t * pucExpiryTime;
    uint32_t ulExpiryTimeLength;

    const uint8_t * pucCertificates;
    uint32_t ulCertificatesLength;
} AzureIoTCARecovery_TrustBundle;

typedef struct AzureIoTCARecovery_RecoveryPayload
{
    const uint8_t * pucPayloadSignature;
    uint32_t ulPayloadSignatureLength;

    const uint8_t * pucTrustBundleJSONObjectText;
    uint32_t ulTrustBundleJSONObjectTextLength;

    AzureIoTCARecovery_TrustBundle xTrustBundle;
} AzureIoTCARecovery_RecoveryPayload;

/**
 * @brief Parse the CA recovery payload received from the Device Provisioning Service.
 *
 * @param[in,out] pxReader The JSON reader initialized with the payload.
 * @param[out] pxRecoveryPayload The struct into which the payload will be parsed.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTCARecovery_ParseRecoveryPayload( AzureIoTJSONReader_t * pxReader,
                                                          AzureIoTCARecovery_RecoveryPayload * pxRecoveryPayload );
