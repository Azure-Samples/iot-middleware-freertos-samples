/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_iot_result.h"

#define azureiotrsaverifySHA256_SIZE_BYTES               32                                                                       /**< Size of the SHA256 hash. */
#define azureiotrsaverifyRSA3072_SIZE_BYTES              384                                                                      /**< Size of the RSA 3072 key. */
#define azureiotrsaverifySHA_CALCULATION_SCRATCH_SIZE    azureiotrsaverifyRSA3072_SIZE_BYTES + azureiotrsaverifySHA256_SIZE_BYTES /**< Size of the sha calculation scratch space. */


/**
 * @brief Verify the manifest via RS256 for the JWS.
 *
 * @param pucInput The input over which the RS256 will be verified.
 * @param ulInputLength The length of \p pucInput.
 * @param pucSignature The encrypted signature which will be decrypted by \p pucN and \p pucE.
 * @param ulSignatureLength The length of \p pucSignature.
 * @param pucN The key's modulus which is used to decrypt \p signature.
 * @param ulNLength The length of \p pucN.
 * @param pucE The exponent used for the key.
 * @param ulELength The length of \p pucE.
 * @param pucBuffer The buffer used as scratch space to make the calculations. It should be at least
 * `azureiotrsaverifySHA256_SIZE_BYTES` in bytes.
 * @param ulBufferLength The length of \p pucBuffer.
 * @return AzureIoTResult_t The result of the operation.
 */
AzureIoTResult_t AzureIoTSample_RS256Verify( uint8_t * pucInput,
                                             uint32_t ulInputLength,
                                             uint8_t * pucSignature,
                                             uint32_t ulSignatureLength,
                                             uint8_t * pucN,
                                             uint32_t ulNLength,
                                             uint8_t * pucE,
                                             uint32_t ulELength,
                                             uint8_t * pucBuffer,
                                             uint32_t ulBufferLength );
