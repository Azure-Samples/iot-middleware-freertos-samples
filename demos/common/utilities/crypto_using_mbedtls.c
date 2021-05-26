/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdio.h>

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

uint32_t Crypto_HMAC( const uint8_t * pucKey, uint32_t ulKeyLength,
                      const uint8_t * pucData, uint32_t ulDataLength,
                      uint8_t * pucOutput, uint32_t ulOutputLength,
                      uint32_t * pulBytesCopied )
{

    uint32_t ulRet;

    if( ulOutputLength < 32 )
    {
        return 1;
    }

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init( &ctx );

    if( mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 1 ) ||
        mbedtls_md_hmac_starts( &ctx, pucKey, ulKeyLength ) ||
        mbedtls_md_hmac_update( &ctx, pucData, ulDataLength ) ||
        mbedtls_md_hmac_finish( &ctx, pucOutput ) )
    {
        ulRet = 1;
    }
    else
    {
        ulRet = 0;
    }

    mbedtls_md_free( &ctx );
    *pulBytesCopied = 32;

    return ulRet;
}
/*-----------------------------------------------------------*/
