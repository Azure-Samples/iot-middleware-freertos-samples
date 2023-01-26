/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <stdint.h>

#include "azure_iot_result.h"

/**
 * @brief Read the trust bundle version.
 *
 * @param[out] ulTrustBundleVersion
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTCAStorage_ReadTrustBundleVersion( uint32_t * ulTrustBundleVersion );

/**
 * @brief Read the current trust bundle from the device storage.
 *
 * @param[in] pucTrustBundle The buffer into which the trust bundle will be copied.
 * @param[in] ulTrustBundleLength The size of \p pucTrustBundle.
 * @param[out] pulOutTrustBundleLength The number of bytes copied into \p pucTrustBundle.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTCAStorage_ReadTrustBundle( const uint8_t * pucTrustBundle,
                                                    uint32_t ulTrustBundleLength,
                                                    uint32_t * pulOutTrustBundleLength );


/**
 * @brief Write a trust bundle to the device storage.
 *
 * @param[in] pucTrustBundle The buffer containing the trust bundle.
 * @param[in] ulTrustBundleLength The size of \p pucTrustBundle.
 * @param[in] ulTrustBundleVersion The buffer containing the trust bundle version.
 * @return AzureIoTResult_t
 */
AzureIoTResult_t AzureIoTCAStorage_WriteTrustBundle( const uint8_t * pucTrustBundle,
                                                     uint32_t ulTrustBundleLength,
                                                     const uint32_t ulTrustBundleVersion );
