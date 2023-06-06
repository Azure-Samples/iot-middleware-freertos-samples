/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_ca_recovery_rsa_verify.h"

#include "azure_iot_config.h"
#include "azure_iot_jws.h"
#include "azure/core/az_base64.h"

#include "mbedtls/base64.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/cipher.h"

/**
 * @brief Calculate the SHA256 over a buffer of bytes
 *
 * @param pucInput The input buffer over which to calculate the SHA256.
 * @param ulInputLength The length of \p pucInput.
 * @param pucOutput The output buffer into which the SHA256. It must be 32 bytes in length.
 * @return AzureIoTResult_t The result of the operation.
 */
static AzureIoTResult_t prvSHA256Calculate( const uint8_t * pucInput,
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
    int32_t outDecodeSize;
    mbedtls_rsa_context ctx;
    uint8_t * ucSignatureBase64Decoded;

    if( ulBufferLength < azureiotrsaverifySHA_CALCULATION_SCRATCH_SIZE )
    {
        AZLogError( ( "[RSA] Buffer not large enough" ) );
        return eAzureIoTErrorOutOfMemory;
    }

    ucSignatureBase64Decoded = pucBuffer + azureiotrsaverifySHA256_SIZE_BYTES;

    az_result xCoreResult = az_base64_decode( az_span_create( ucSignatureBase64Decoded, azureiotrsaverifyRSA3072_SIZE_BYTES ),
                                              az_span_create( pucSignature, ulSignatureLength ),
                                              &outDecodeSize );

    if( az_result_failed( xCoreResult ) )
    {
        AZLogError( ( "[RSA] Base64 decode failed: 0x%08x", ( unsigned int ) xCoreResult ) );
        return eAzureIoTErrorFailed;
    }

    /* The signature is signed using the input key. We compare the signature to */
    /* the SHA256 of the input. */
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
        AZLogError( ( "[RSA] mbedtls_rsa_import_raw failed, res: %08x", ( uint16_t ) lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return eAzureIoTErrorFailed;
    }

    lMbedTLSResult = mbedtls_rsa_complete( &ctx );

    if( lMbedTLSResult != 0 )
    {
        AZLogError( ( "[RSA] mbedtls_rsa_complete failed, res: %08x", ( uint16_t ) lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return eAzureIoTErrorFailed;
    }

    lMbedTLSResult = mbedtls_rsa_check_pubkey( &ctx );

    if( lMbedTLSResult != 0 )
    {
        AZLogError( ( "[RSA] mbedtls_rsa_check_pubkey failed, res: %08x", ( uint16_t ) lMbedTLSResult ) );
        mbedtls_rsa_free( &ctx );
        return eAzureIoTErrorFailed;
    }

    /* RSA */
    xResult = prvSHA256Calculate( pucInput, ulInputLength,
                                  pucBuffer );

    if( xResult != eAzureIoTSuccess )
    {
        AZLogError( ( "[RSA] prvSHA256Calculate failed" ) );
        mbedtls_rsa_free( &ctx );
        return xResult;
    }

    #if MBEDTLS_VERSION_NUMBER >= 0x03000000
        lMbedTLSResult = mbedtls_rsa_pkcs1_verify( &ctx, MBEDTLS_MD_SHA256, azureiotjwsSHA256_SIZE, pucBuffer, ucSignatureBase64Decoded );
    #else
        lMbedTLSResult = mbedtls_rsa_pkcs1_verify( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA256, azureiotrsaverifySHA256_SIZE_BYTES, pucBuffer, ucSignatureBase64Decoded );
    #endif

    if( lMbedTLSResult != 0 )
    {
        AZLogError( ( "[RSA] SHA of JWK does NOT match (0x%08x)", ( uint16_t ) lMbedTLSResult ) );
        xResult = eAzureIoTErrorFailed;
    }
    else
    {
        xResult = eAzureIoTSuccess;
    }

    mbedtls_rsa_free( &ctx );

    return xResult;
}
