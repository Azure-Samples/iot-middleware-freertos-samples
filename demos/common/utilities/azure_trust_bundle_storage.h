/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdint.h>

#include "azure_iot_result.h"

AzureIoTResult_t AzureIoTCAStorage_ReadTrustBundle( const uint8_t * pucTrustBundle,
                                                    uint32_t ulTrustBundleLength,
                                                    uint32_t * pulOutTrustBundleLength,
                                                    const uint8_t * ulTrustBundleVersion,
                                                    uint32_t ulTrustBundleVersionLength,
                                                    uint32_t * pulOutTrustBundleVersionLength );

AzureIoTResult_t AzureIoTCAStorage_WriteTrustBundle( const uint8_t * pucTrustBundle,
                                                     uint32_t ulTrustBundleLength,
                                                     const uint8_t * pucTrustBundleVersion,
                                                     uint32_t ulTrustBundleVersionLength );
