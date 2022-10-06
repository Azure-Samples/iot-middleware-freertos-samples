/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <string.h>

#include "azure_iot_flash_platform.h"

#include "azure_iot_flash_platform_port.h"

#include "stm32l4xx_hal.h"
/* Logging */
#include "azure_iot.h"

#include "azure/core/az_base64.h"

#include "mbedtls/md.h"

/** advance addr by this amount to program the next 32 row double-word (64-bit)
 * for fast programming
 */
#define azureiotflashL475_FLASH_ROW_SIZE    256

#define azureiotflashSHA_256_SIZE           32

static uint8_t ucPartitionReadBuffer[ 32 ];
static uint8_t ucDecodedManifestHash[ azureiotflashSHA_256_SIZE ];
static uint8_t ucCalculatedHash[ azureiotflashSHA_256_SIZE ];

static AzureIoTResult_t prvBase64Decode( uint8_t * base64Encoded,
                                         size_t ulBase64EncodedLength,
                                         uint8_t * pucOutputBuffer,
                                         size_t bufferLen,
                                         size_t * outputSize )
{
    az_result xCoreResult;

    az_span encodedSpan = az_span_create( base64Encoded, ulBase64EncodedLength );

    az_span outputSpan = az_span_create( pucOutputBuffer, bufferLen );

    if( az_result_failed( xCoreResult = az_base64_decode( outputSpan, encodedSpan, ( int32_t * ) outputSize ) ) )
    {
        AZLogError( ( "az_base64_decode failed: core error=0x%08x", xCoreResult ) );
        return eAzureIoTErrorFailed;
    }

    AZLogInfo( ( "Unencoded the base64 encoding\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_Init( AzureADUImage_t * const pxAduImage )
{
    pxAduImage->xUpdatePartition = ( uint8_t * ) ( FLASH_BASE + FLASH_BANK_SIZE );
    pxAduImage->ulCurrentOffset = 0;
    pxAduImage->ulImageFileSize = 0;

    static FLASH_EraseInitTypeDef xEraseInitStruct;
    uint32_t ulPageError;
    FLASH_OBProgramInitTypeDef xOptionBytes;
    AzureIoTResult_t xResult = eAzureIoTSuccess;

    /* Clear OPTVERR bit set on virgin samples. */
    __HAL_FLASH_CLEAR_FLAG( FLASH_FLAG_OPTVERR );
    /* Get current optionbytes configuration */
    HAL_FLASHEx_OBGetConfig( &xOptionBytes );

    /* If BFB2 (Boot From Bank 2) is set, erase bank 1, otherwise erase bank 2 */
    xEraseInitStruct.Banks =
        ( ( xOptionBytes.USERConfig & OB_BFB2_ENABLE ) == OB_BFB2_ENABLE )
        ? FLASH_BANK_1
        : FLASH_BANK_2;
    xEraseInitStruct.TypeErase = FLASH_TYPEERASE_MASSERASE;

    HAL_FLASH_Unlock();

    /* Erase non-boot bank */
    if( HAL_FLASHEx_Erase( &xEraseInitStruct, &ulPageError ) != HAL_OK )
    {
        /* Error occurred during page erase. */
        AZLogError( ( "Error erasing flash bank\r\n" ) );
        xResult = eAzureIoTErrorFailed;
    }

    HAL_FLASH_Lock();

    return xResult;
}

AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{
    uint8_t * pucNextWriteAddr = pxAduImage->xUpdatePartition + ulOffset;
    uint8_t * pucNextReadAddr = pData;
    uint8_t * pucLastSectionAddr = pxAduImage->xUpdatePartition + ulOffset + ulBlockSize - azureiotflashL475_FLASH_ROW_SIZE;
    AzureIoTResult_t xResult = eAzureIoTSuccess;

    HAL_FLASH_Unlock();

    /* write sections 1...n-1 of all blocks */
    while( pucNextWriteAddr < pucLastSectionAddr )
    {
        if( HAL_FLASH_Program( FLASH_TYPEPROGRAM_FAST, ( uint32_t ) pucNextWriteAddr, ( uint32_t ) pucNextReadAddr ) != HAL_OK )
        {
            /* Error occurred while writing data in Flash memory */
            HAL_FLASH_Lock();
            return eAzureIoTErrorFailed;
        }

        pucNextWriteAddr += azureiotflashL475_FLASH_ROW_SIZE;
        pucNextReadAddr += azureiotflashL475_FLASH_ROW_SIZE;
    }

    /** if last block and last section of that block, use
     * FLASH_TYPEPROGRAM_FAST_AND_LAST
     */
    if( pxAduImage->ulImageFileSize - ulOffset <= ulBlockSize )
    {
        if( HAL_FLASH_Program( FLASH_TYPEPROGRAM_FAST_AND_LAST, ( uint32_t ) pucNextWriteAddr, ( uint32_t ) pucNextReadAddr ) != HAL_OK )
        {
            /* Error occurred while writing data in Flash memory */
            xResult = eAzureIoTErrorFailed;
        }
    }
    else
    {
        /* write last section of blocks 1...n-1 normally */
        if( HAL_FLASH_Program( FLASH_TYPEPROGRAM_FAST, ( uint32_t ) pucNextWriteAddr, ( uint32_t ) pucNextReadAddr ) != HAL_OK )
        {
            /* Error occurred while writing data in Flash memory */
            xResult = eAzureIoTErrorFailed;
        }
    }

    HAL_FLASH_Lock();
    return xResult;
}

AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                               uint8_t * pucSHA256Hash,
                                               uint32_t ulSHA256HashLength )
{
    int xResult;
    uint32_t ulOutputSize;
    uint32_t ulReadSize;

    AZLogInfo( ( "Base64 Encoded Hash from ADU: %.*s", ulSHA256HashLength, pucSHA256Hash ) );
    xResult = prvBase64Decode( pucSHA256Hash, ulSHA256HashLength, ucDecodedManifestHash, azureiotflashSHA_256_SIZE, ( size_t * ) &ulOutputSize );

    if( xResult != eAzureIoTSuccess )
    {
        AZLogError( ( "Unable to decode base64 SHA256\r\n" ) );
        return eAzureIoTErrorFailed;
    }

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init( &ctx );
    mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 0 );
    mbedtls_md_starts( &ctx );

    AZLogInfo( ( "Starting the mbedtls calculation: image size %d\r\n", pxAduImage->ulImageFileSize ) );

    for( size_t ulOffset = 0; ulOffset < pxAduImage->ulImageFileSize; ulOffset += sizeof( ucPartitionReadBuffer ) )
    {
        ulReadSize = pxAduImage->ulImageFileSize - ulOffset < sizeof( ucPartitionReadBuffer ) ? pxAduImage->ulImageFileSize - ulOffset : sizeof( ucPartitionReadBuffer );

        memcpy( ucPartitionReadBuffer, ( pxAduImage->xUpdatePartition + ulOffset ), ulReadSize );

        mbedtls_md_update( &ctx, ( const unsigned char * ) ucPartitionReadBuffer, ulReadSize );

        if( ( ulOffset % 65536 == 0 ) && ( ulOffset != 0 ) )
        {
            printf( "." );
        }
    }

    printf( "\r\n" );

    AZLogInfo( ( "Done\r\n" ) );

    mbedtls_md_finish( &ctx, ucCalculatedHash );
    mbedtls_md_free( &ctx );

    if( memcmp( ucDecodedManifestHash, ucCalculatedHash, azureiotflashSHA_256_SIZE ) == 0 )
    {
        AZLogInfo( ( "SHAs match\r\n" ) );
        xResult = eAzureIoTSuccess;
    }
    else
    {
        AZLogInfo( ( "SHAs do not match\r\n" ) );
        AZLogInfo( ( "Wanted: " ) );

        for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
        {
            printf( "%x", ucDecodedManifestHash[ i ] );
        }

        printf( ( "\r\n" ) );
        AZLogInfo( ( "Calculated: " ) );

        for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
        {
            printf( "%x", ucCalculatedHash[ i ] );
        }

        printf( ( "\r\n" ) );

        xResult = eAzureIoTErrorFailed;
    }

    return xResult;
}

AzureIoTResult_t AzureIoTPlatform_EnableImage( AzureADUImage_t * const pxAduImage )
{
    FLASH_OBProgramInitTypeDef xOptionBytes;

    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    HAL_FLASHEx_OBGetConfig( &xOptionBytes ); /* Get current optionbytes configuration */
    xOptionBytes.OptionType = OPTIONBYTE_USER;
    xOptionBytes.USERType = OB_USER_BFB2;
    /* If BFB2 set, we will reset it */
    xOptionBytes.USERConfig =
        ( ( xOptionBytes.USERConfig & OB_BFB2_ENABLE ) == OB_BFB2_ENABLE )
        ? OB_BFB2_DISABLE
        : OB_BFB2_ENABLE;

    HAL_FLASHEx_OBProgram( &xOptionBytes );

    /**
     *  sets options bits and restarts device.
     * Also sets the book bank address to 0x08000000, which means we always write
     * to 0x08080000
     */
    HAL_FLASH_OB_Launch();
    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_ResetDevice( AzureADUImage_t * const pxAduImage )
{
    HAL_NVIC_SystemReset();
    return eAzureIoTSuccess;
}
