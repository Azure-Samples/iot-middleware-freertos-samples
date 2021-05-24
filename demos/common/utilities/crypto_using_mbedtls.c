/*
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file mbedtls_freertos_port.c
 * @brief Implements mbed TLS platform functions for FreeRTOS.
 */

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
