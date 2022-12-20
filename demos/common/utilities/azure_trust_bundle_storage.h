/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdint.h>

#include "azure_iot_result.h"

AzureIoTResult_t AzureIoTCAStorage_ReadTrustBundle( const uint8_t * pucTrustBundle,
                                                    uint32_t ulTrustBundleLength,
                                                    uint32_t * pulOutTrustBundleLength,
                                                    uint32_t * ulTrustBundleVersion );

AzureIoTResult_t AzureIoTCAStorage_WriteTrustBundle( const uint8_t * pucTrustBundle,
                                                     uint32_t ulTrustBundleLength,
                                                     uint32_t ulTrustBundleVersion );
