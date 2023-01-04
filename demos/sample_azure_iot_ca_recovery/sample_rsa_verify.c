/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "sample_rsa_verify.h"

#include "mbedtls/base64.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/cipher.h"

#define azureiotjwsSHA256_SIZE                     32                                               /**< Size of the SHA256 hash. */
#define azureiotjwsRSA3072_SIZE                    384                                              /**< Size of the RSA 3072 key. */
#define azureiotjwsSHA_CALCULATION_SCRATCH_SIZE    azureiotjwsRSA3072_SIZE + azureiotjwsSHA256_SIZE /**< Size of the sha calculation scratch space. */

/**
 * @brief Calculate the SHA256 over a buffer of bytes
 *
 * @param pucInput The input buffer over which to calculate the SHA256.
 * @param ulInputLength The length of \p pucInput.
 * @param pucOutput The output buffer into which the SHA256. It must be 32 bytes in length.
 * @return uint32_t The result of the operation.
 * @retval 0 if successful.
 * @retval Non-0 if not successful.
 */
static AzureIoTResult_t prvJWS_SHA256Calculate( const uint8_t * pucInput,
                                                uint32_t ulInputLength,
                                                uint8_t * pucOutput )
{
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init( &ctx );
    mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 0 );
    mbedtls_md_starts( &ctx );
    mbedtls_md_update( &ctx, pucInput, ulInputLength );
    mbedtls_md_finish( &ctx, pucOutput );
    mbedtls_md_free( &ctx );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTSample_RS256Verify( uint8_t * pucInput,
                                             uint32_t ulInputLength,
                                             uint8_t * pucSignature,
                                             uint32_t ulSignatureLength,
                                             uint8_t * pucN,
                                             uint32_t ulNLength,
                                             uint8_t * pucE,
                                             uint32_t ulELength,
                                             uint8_t * pucBuffer,
                                             uint32_t ulBufferLength )
{
    AzureIoTResult_t xResult;
    int32_t lMbedTLSResult;
    mbedtls_rsa_context ctx;

    if( ulBufferLength < azureiotjwsSHA_CALCULATION_SCRATCH_SIZE )
    {
        AZLogError( ( "[JWS] Buffer Not Large Enough" ) );
        return eAzureIoTErrorOutOfMemory;
    }

    /* The signature is encrypted using the input key. We need to decrypt the */
    /* signature which gives us the SHA256 inside a PKCS7 structure. We then compare */
    /* that to the SHA256 of the input. */
    #if MBEDTLS_VERSION_NUMBER >= 0x03000000
        mbedtls_rsa_init( &ctx );
    #else
        mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );
    #endif

    lMbedTLSResult = mbedtls_rsa_import_raw( &ctx,
                                             pucN, ulNLength,
                                             NULL, 0,
                                             NULL, 0,
                                             NULL, 0,
                                             pucE, ulELength );

    if( lMbedTLSResult != 0 )
    {
        AZLogError( ( "[JWS] mbedtls_rsa_import_raw res: %08x", ( uint16_t ) lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return eAzureIoTErrorFailed;
    }

    lMbedTLSResult = mbedtls_rsa_complete( &ctx );

    if( lMbedTLSResult != 0 )
    {
        AZLogError( ( "[JWS] mbedtls_rsa_complete res: %08x", ( uint16_t ) lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return eAzureIoTErrorFailed;
    }

    lMbedTLSResult = mbedtls_rsa_check_pubkey( &ctx );

    if( lMbedTLSResult != 0 )
    {
        AZLogError( ( "[JWS] mbedtls_rsa_check_pubkey res: %08x", ( uint16_t ) lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return eAzureIoTErrorFailed;
    }

    /* RSA */
    xResult = prvJWS_SHA256Calculate( pucInput, ulInputLength,
                                      pucBuffer );

    if( xResult != eAzureIoTSuccess )
    {
        AZLogError( ( "[JWS] prvJWS_SHA256Calculate failed" ) );
        return xResult;
    }

    #if MBEDTLS_VERSION_NUMBER >= 0x03000000
        lMbedTLSResult = mbedtls_rsa_pkcs1_verify( &ctx, MBEDTLS_MD_SHA256, azureiotjwsSHA256_SIZE, pucBuffer, pucSignature );
    #else
        lMbedTLSResult = mbedtls_rsa_pkcs1_verify( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA256, azureiotjwsSHA256_SIZE, pucBuffer, pucSignature );
    #endif

    if( lMbedTLSResult != 0 )
    {
        AZLogError( ( "[JWS] SHA of JWK does NOT match (%08x)", ( uint16_t ) lMbedTLSResult ) );
        xResult = eAzureIoTErrorFailed;
    }
    else
    {
        xResult = eAzureIoTSuccess;
    }

    mbedtls_rsa_free( &ctx );

    return xResult;
}
