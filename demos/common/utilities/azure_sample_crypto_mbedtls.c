/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure_sample_crypto.h"

#include "threading_alt.h"

/* mbed TLS includes. */
#include "mbedtls/md.h"
#include "mbedtls/threading.h"

/*-----------------------------------------------------------*/

uint32_t Crypto_Init()
{
    /* Set the mutex functions for mbed TLS thread safety. */
    mbedtls_threading_set_alt( mbedtls_platform_mutex_init,
                               mbedtls_platform_mutex_free,
                               mbedtls_platform_mutex_lock,
                               mbedtls_platform_mutex_unlock );

    return 0;
}
/*-----------------------------------------------------------*/

uint32_t Crypto_HMAC( const uint8_t * pucKey,
                      uint32_t ulKeyLength,
                      const uint8_t * pucData,
                      uint32_t ulDataLength,
                      uint8_t * pucOutput,
                      uint32_t ulOutputLength,
                      uint32_t * pulBytesCopied )
{
    uint32_t ulRet;
    mbedtls_md_context_t xCtx;
    mbedtls_md_type_t xMDType = MBEDTLS_MD_SHA256;

    if( ulOutputLength < 32 )
    {
        return 1;
    }

    mbedtls_md_init( &xCtx );

    if( mbedtls_md_setup( &xCtx, mbedtls_md_info_from_type( xMDType ), 1 ) ||
        mbedtls_md_hmac_starts( &xCtx, pucKey, ulKeyLength ) ||
        mbedtls_md_hmac_update( &xCtx, pucData, ulDataLength ) ||
        mbedtls_md_hmac_finish( &xCtx, pucOutput ) )
    {
        ulRet = 1;
    }
    else
    {
        ulRet = 0;
        *pulBytesCopied = 32;
    }

    mbedtls_md_free( &xCtx );

    return ulRet;
}
/*-----------------------------------------------------------*/
