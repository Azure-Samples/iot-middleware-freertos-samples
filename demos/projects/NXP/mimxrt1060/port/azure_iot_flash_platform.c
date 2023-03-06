/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <string.h>

#include "azure_iot_flash_platform.h"

#include "azure_iot_flash_platform_port.h"
/* Logging */
#include "azure_iot.h"

#include "azure/core/az_base64.h"

#include "mbedtls/md.h"

#include "flash_info.h"
#include "sbl_ota_flag.h"

#define azureiotflashSHA_256_SIZE    32

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
    pxAduImage->ulCurrentOffset = 0;
    pxAduImage->ulImageFileSize = 0;

    AzureIoTResult_t xResult = eAzureIoTSuccess;
    status_t xStatus;
    uint8_t ucImagePosition;
    volatile uint32_t ulPrimask;

    sfw_flash_read( REMAP_FLAG_ADDRESS, &ucImagePosition, 1 );

    if( ucImagePosition == 0x01 )
    {
        pxAduImage->xUpdatePartition = FLASH_AREA_IMAGE_2_OFFSET;
    }
    else if( ucImagePosition == 0x02 )
    {
        pxAduImage->xUpdatePartition = FLASH_AREA_IMAGE_1_OFFSET;
    }
    else
    {
        AZLogError( ( "Invalid image position! Will write to image 2" ) );
        pxAduImage->xUpdatePartition = FLASH_AREA_IMAGE_2_OFFSET;
    }

    ulPrimask = DisableGlobalIRQ();
    xStatus = sfw_flash_erase( pxAduImage->xUpdatePartition, FLASH_AREA_IMAGE_1_SIZE );
    EnableGlobalIRQ( ulPrimask );

    if( xStatus )
    {
        AZLogError( ( "Error erasing flash.\r\n" ) );
        xResult = eAzureIoTErrorFailed;
    }

    return xResult;
}

int64_t AzureIoTPlatform_GetSingleFlashBootBankSize()
{
    return FLASH_AREA_IMAGE_1_SIZE;
}

AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{
    uint32_t pucNextWriteAddr = pxAduImage->xUpdatePartition + ulOffset;
    AzureIoTResult_t xResult = eAzureIoTSuccess;
    volatile uint32_t ulPrimask;
    status_t xStatus;

    ulPrimask = DisableGlobalIRQ();
    xStatus = sfw_flash_write( pucNextWriteAddr, pData, ulBlockSize );
    EnableGlobalIRQ( ulPrimask );

    if( xStatus )
    {
        xResult = eAzureIoTErrorFailed;
    }

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
        sfw_flash_read( ( pxAduImage->xUpdatePartition + ulOffset ), ucPartitionReadBuffer, ulReadSize );

        mbedtls_md_update( &ctx, ( const unsigned char * ) ucPartitionReadBuffer, ulReadSize );
    }

    AZLogInfo( ( "mbedtls calculation completed\r\n" ) );

    mbedtls_md_finish( &ctx, ucCalculatedHash );
    mbedtls_md_free( &ctx );

    if( memcmp( ucDecodedManifestHash, ucCalculatedHash, azureiotflashSHA_256_SIZE ) == 0 )
    {
        AZLogInfo( ( "SHAs match\r\n" ) );
        xResult = eAzureIoTSuccess;
    }
    else
    {
        AZLogError( ( "SHAs do not match\r\n" ) );
        AZLogInfo( ( "Wanted: " ) );

        for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
        {
            AZLogInfo( ( "%x", ucDecodedManifestHash[ i ] ) );
        }

        AZLogInfo( ( "\r\n" ) );
        AZLogInfo( ( "Calculated: " ) );

        for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
        {
            AZLogInfo( ( "%x", ucCalculatedHash[ i ] ) );
        }

        AZLogInfo( ( "\r\n" ) );

        xResult = eAzureIoTErrorFailed;
    }

    return xResult;
}

AzureIoTResult_t AzureIoTPlatform_EnableImage( AzureADUImage_t * const pxAduImage )
{
    enable_image();

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_ResetDevice( AzureADUImage_t * const pxAduImage )
{
    NVIC_SystemReset();

    return eAzureIoTSuccess;
}
