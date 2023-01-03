/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <string.h>

#include "azure_iot_flash_platform.h"

#include "azure_iot_flash_platform_port.h"
/* Logging */
#include "azure_iot.h"

#include "azure/core/az_base64.h"

#include "esp_ota_ops.h"
#include "esp_system.h"
#include "mbedtls/md.h"

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
        AZLogError( ( "az_base64_decode failed: core error=0x%08x", ( uint16_t ) xCoreResult ) );
        return eAzureIoTErrorFailed;
    }

    AZLogInfo( ( "Unencoded the base64 encoding\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_Init( AzureADUImage_t * const pxAduImage )
{
    const esp_partition_t * pxCurrentPartition = esp_ota_get_running_partition();

    if( pxCurrentPartition == NULL )
    {
        AZLogError( ( "esp_ota_get_running_partition failed" ) );
        return eAzureIoTErrorFailed;
    }

    pxAduImage->pucBufferToWrite = NULL;
    pxAduImage->ulBytesToWriteLength = 0;
    pxAduImage->ulCurrentOffset = 0;
    pxAduImage->ulImageFileSize = 0;
    pxAduImage->xUpdatePartition = esp_ota_get_next_update_partition( pxCurrentPartition );

    if( pxAduImage->xUpdatePartition == NULL )
    {
        AZLogError( ( "esp_ota_get_next_update_partition failed" ) );
        return eAzureIoTErrorFailed;
    }

    esp_partition_erase_range( pxAduImage->xUpdatePartition, 0, pxAduImage->xUpdatePartition->size );

    return eAzureIoTSuccess;
}

int64_t AzureIoTPlatform_GetSingleFlashBootBankSize()
{
    const esp_partition_t * pxCurrentPartition = esp_ota_get_running_partition();

    if( pxCurrentPartition == NULL )
    {
        AZLogError( ( "esp_ota_get_running_partition failed" ) );
        return -1;
    }

    const esp_partition_t * pxNextPartition = esp_ota_get_next_update_partition( pxCurrentPartition );

    if( pxNextPartition == NULL )
    {
        AZLogError( ( "esp_ota_get_next_update_partition failed" ) );
        return -1;
    }

    /* size of the next ota partition to be written to */
    return pxNextPartition->size;
}

AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{
    int ret;

    ret = esp_partition_write( pxAduImage->xUpdatePartition, ulOffset, pData, ulBlockSize );

    if( ret != ESP_OK )
    {
        return ret;
    }

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                               uint8_t * pucSHA256Hash,
                                               uint32_t ulSHA256HashLength )
{
    int xResult;
    esp_err_t espErr;

    uint32_t ulOutputSize;
    uint32_t ulReadSize;

    AZLogInfo( ( "Base64 Encoded Hash from ADU: %.*s", ( int16_t ) ulSHA256HashLength, pucSHA256Hash ) );
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

    AZLogInfo( ( "Starting the mbedtls calculation: image size %u\r\n", ( uint16_t ) pxAduImage->ulImageFileSize ) );

    for( size_t ulOffset = 0; ulOffset < pxAduImage->ulImageFileSize; ulOffset += sizeof( ucPartitionReadBuffer ) )
    {
        ulReadSize = pxAduImage->ulImageFileSize - ulOffset < sizeof( ucPartitionReadBuffer ) ? pxAduImage->ulImageFileSize - ulOffset : sizeof( ucPartitionReadBuffer );

        espErr = esp_partition_read_raw( pxAduImage->xUpdatePartition,
                                         ulOffset,
                                         ucPartitionReadBuffer,
                                         ulReadSize );
        ( void ) espErr;

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
        AZLogInfo( ( "SHAs do not match\r\n" ) );
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
    esp_err_t err = esp_ota_set_boot_partition( pxAduImage->xUpdatePartition );

    ( void ) err;
    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_ResetDevice( AzureADUImage_t * const pxAduImage )
{
    esp_restart();
    return eAzureIoTSuccess;
}
