/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"

#include "sample_adu_jws.h"
#include "demo_config.h"
#include "FreeRTOSConfig.h"

static mbedtls_entropy_context xEntropyContext;
static mbedtls_ctr_drbg_context xCtrDrgbContext;

static const char ucManifest[] = "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"ESPRESSIF\",\"name\":\"ESP32-Azure-IoT-Kit\",\"version\":\"1.1\"},\"compatibility\":[{\"deviceManufacturer\":\"ESPRESSIF\",\"deviceModel\":\"ESP32-Azure-IoT-Kit\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:1\",\"files\":[\"fc3558477982e3235\"],\"handlerProperties\":{\"installedCriteria\":\"1.0\"}}]},\"files\":{\"fc3558477982e3235\":{\"fileName\":\"azure_iot_freertos_esp32.bin\",\"sizeInBytes\":866128,\"hashes\":{\"sha256\":\"exKJAqfEo69Ok6C6SWy9+Hhp051JbRsXsnMjGSbbJ6o=\"}}},\"createdDateTime\":\"2022-06-03T00:20:33.8421122Z\"}";
static char ucManifestJWS[] = "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJMeTlqT1hHc1ZvQ1daM0N1dFhsWWNXQ2VYY2V3YkR4Ri9GbjVqM2srSW1ZPSJ9.Wq4UoXt4dGay_P8uy7jrxM8Iip3KCXkGZvQwnu83704CzDogfVqX4GegT68s47veOi3x2Gf5rjX7vOMzVf9Ck0ylGCfon-vit938hO9MNYM7siA5htYHzotdECD1LfI_BjlLxkwXt0OyLC1PJvMw9N870pb51NtTon0OmaQslEyf6ih6DrEvsNUnyjRcrzSWlIyRo18kqlzeetARTYE7qGQr7oZPh0RWXVP5b5XR3wbJ_IeZ6i85YmjFpbRGJaSPCuzpa7XKvvFzB5rB5lGmbkWsOMyLbVzUriW87BzbB06g-wzs1S-z07s-ZGjTbFdrXrGjkKtv3TaDirjTqHhhJyI2cVLBctr4Wv4XITPyZeJt2KcIQZup-KfCRNbM3c3_PXPgvJtOg5BhmUrUKGMqFTl84EIB44B1QqKmuiTdH3bNQxPKBecpCk-O9g03pB-fk1D_3sL1ju364STs87s77DfGK9e0oHbHgfzp4EdgrwRQBvTCWWKG3iT6ByfSH4N0";

static int initMbedtls( mbedtls_entropy_context * pxEntropyContext,
                        mbedtls_ctr_drbg_context * pxCtrDrgbContext )
{
    int32_t lMbedtlsError = 0;

    /* Set the mutex functions for mbed TLS thread safety. */
    mbedtls_threading_set_alt( mbedtls_platform_mutex_init,
                               mbedtls_platform_mutex_free,
                               mbedtls_platform_mutex_lock,
                               mbedtls_platform_mutex_unlock );

    /* Initialize contexts for random number generation. */
    mbedtls_entropy_init( pxEntropyContext );
    mbedtls_ctr_drbg_init( pxCtrDrgbContext );

    /* Add a strong entropy source. At least one is required. */
    lMbedtlsError = mbedtls_entropy_add_source( pxEntropyContext,
                                                mbedtls_platform_entropy_poll,
                                                NULL,
                                                32,
                                                MBEDTLS_ENTROPY_SOURCE_STRONG );

    if( lMbedtlsError != 0 )
    {
        printf( "mbedtls_entropy_add_source failed: %i\n", lMbedtlsError );
        return lMbedtlsError;
    }

    /* Seed the random number generator. */
    lMbedtlsError = mbedtls_ctr_drbg_seed( pxCtrDrgbContext,
                                           mbedtls_entropy_func,
                                           pxEntropyContext,
                                           "jws_ut",
                                           strlen( "jws_ut" ) );

    if( lMbedtlsError != 0 )
    {
        printf( "mbedtls_ctr_drbg_seed failed: %i\n", lMbedtlsError );
    }

    return lMbedtlsError;
}

/*
 * @brief Create the task that runs the test
 */
int vStartTestTask( void )
{
    if( initMbedtls( &xEntropyContext, &xCtrDrgbContext ) != 0 )
    {
        printf( "mbedtls init failed\n" );
        return 1;
    }

    return JWS_ManifestAuthenticate( ucManifest, strlen( ucManifest ), ucManifestJWS, strlen( ucManifestJWS ) );
}
