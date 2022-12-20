/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_ca_storage.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define CA_CERT_NAMESPACE                  "root-ca-cert"
#define AZURE_TRUST_BUNDLE_NAME            "az-tb"
#define AZURE_TRUST_BUNDLE_VERSION_NAME    "az-tb-ver"

AzureIoTResult_t AzureIoTCAStorage_ReadTrustBundle( const uint8_t * pucTrustBundle,
                                                    uint32_t ulTrustBundleLength,
                                                    uint32_t * pulOutTrustBundleLength,
                                                    uint32_t * ulTrustBundleVersion )
{
}

AzureIoTResult_t AzureIoTCAStorage_WriteTrustBundle( const uint8_t * pucTrustBundle,
                                                     uint32_t ulTrustBundleLength,
                                                     uint32_t ulTrustBundleVersion )
{
}
