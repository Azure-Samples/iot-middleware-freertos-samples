/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @file
 *
 * @brief APIs to authenticate an ADU manifest.
 *
 */

#ifndef SAMPLE_ADU_JWS_H
#define SAMPLE_ADU_JWS_H

#include <stdint.h>

#define jwsRSA3072_SIZE                    384
#define jwsSHA256_SIZE                     32
#define jwsPKCS7_PAYLOAD_OFFSET            19
#define jwsJWS_HEADER_SIZE                 1400
#define jwsJWS_PAYLOAD_SIZE                60
#define jwsJWS_SIGNATURE_SIZE              400
#define jwsJWK_HEADER_SIZE                 48
#define jwsJWK_PAYLOAD_SIZE                700
#define jwsJWK_SIGNATURE_SIZE              400
#define jwsSIGNING_KEY_E_SIZE              16
#define jwsSHA_CALCULATION_SCRATCH_SIZE    jwsRSA3072_SIZE + jwsSHA256_SIZE

#define jwsSCRATCH_BUFFER_SIZE                                           \
    ( jwsRSA3072_SIZE + jwsSHA256_SIZE +                                 \
      jwsPKCS7_PAYLOAD_OFFSET + jwsJWS_HEADER_SIZE +                     \
      jwsJWS_PAYLOAD_SIZE + jwsJWS_SIGNATURE_SIZE +                      \
      jwsJWK_HEADER_SIZE + jwsJWK_PAYLOAD_SIZE + jwsJWK_SIGNATURE_SIZE + \
      jwsSIGNING_KEY_E_SIZE + jwsSHA_CALCULATION_SCRATCH_SIZE )

/**
 * @brief Authenticate the manifest from ADU.
 *
 * @param[in] pucManifest The escaped manifest from the ADU twin property.
 * @param[in] ulManifestLength The length of \p pucManifest.
 * @param[in] pucJWS The JWS used to authenticate \p pucManifest.
 * @param[in] ulJWSLength The length of \p pucJWS.
 * @param[in] pucScratchBuffer Scratch buffer space for calculations. It should be
 * `jwsSCRATCH_BUFFER_SIZE` in length.
 * @param[in] ulScratchBufferLength The length of \p pucScratchBuffer.
 * @return uint32_t The return value of this function.
 * @retval 0 if successful.
 * @retval Otherwise if failed.
 */
uint32_t JWS_ManifestAuthenticate( const char * pucManifest,
                                   uint32_t ulManifestLength,
                                   char * pucJWS,
                                   uint32_t ulJWSLength,
                                   char * pucScratchBuffer,
                                   uint32_t ulScratchBufferLength );

#endif /* SAMPLE_ADU_JWS_H */
