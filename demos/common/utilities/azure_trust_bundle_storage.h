/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdint.h>

#include "azure_iot_result.h"

/**
 * @brief Read the current trust bundle from the device storage.
 * 
 * @param[in] pucTrustBundle The buffer into which the trust bundle will be copied.
 * @param[in] ulTrustBundleLength The size of \p pucTrustBundle.
 * @param[out] pulOutTrustBundleLength The number of bytes copied into \p pucTrustBundle.
 * @param[in] pucTrustBundleVersion The buffer into which the trust bundle version will be copied.
 * @param[in] ulTrustBundleVersionLength The size of \p pucTrustBundleVersion.
 * @param[out] pulOutTrustBundleVersionLength The number of bytes copied into \p pucTrustBundleVersion.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTCAStorage_ReadTrustBundle( const uint8_t * pucTrustBundle,
                                                    uint32_t ulTrustBundleLength,
                                                    uint32_t * pulOutTrustBundleLength,
                                                    const uint8_t * pucTrustBundleVersion,
                                                    uint32_t ulTrustBundleVersionLength,
                                                    uint32_t * pulOutTrustBundleVersionLength );

/**
 * @brief Write a trust bundle to the device storage.
 * 
 * @param[in] pucTrustBundle The buffer containing the trust bundle.
 * @param[in] ulTrustBundleLength The size of \p pucTrustBundle
 * @param[in] pucTrustBundleVersion The buffer containing the trust bundle version.
 * @param[in] ulTrustBundleVersionLength The size of \p pucTrustBundleVersion.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTCAStorage_WriteTrustBundle( const uint8_t * pucTrustBundle,
                                                     uint32_t ulTrustBundleLength,
                                                     const uint8_t * pucTrustBundleVersion,
                                                     uint32_t ulTrustBundleVersionLength );
