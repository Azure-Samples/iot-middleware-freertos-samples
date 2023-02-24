/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <string.h>

#include "azure_iot_flash_platform.h"
/* #include "mflash_drv.h" */
/* #include "mflash_file.h" */

#include "azure_iot_flash_platform_port.h"
/* Logging */
#include "azure_iot.h"

#include "azure/core/az_base64.h"

#include "mbedtls/md.h"

// #include "fsl_flexspi.h"
// #include "flexspi_flash.h"
#include "flash_info.h"
#include "sbl_ota_flag.h"

/* #include "fsl_nor_flash.h" */

/* #include "fsl_flexspi_nor_flash.h" */

/* #include "fsl_adapter_flash.h" */

/* #include "flexspi_nor_flash.h" */

/* #include "flexspi_flash.h" */

/* #include "fsl_adapter_flash.h" */

#define azureiotflashSHA_256_SIZE    32


static uint8_t ucPartitionReadBuffer[ 32 ];
static uint8_t ucDecodedManifestHash[ azureiotflashSHA_256_SIZE ];
static uint8_t ucCalculatedHash[ azureiotflashSHA_256_SIZE ];
static uint32_t dstAddr;

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
        LogError( ( "az_base64_decode failed: core error=0x%08x", xCoreResult ) );
        return eAzureIoTErrorFailed;
    }

    LogInfo( ( "Unencoded the base64 encoding\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_Init( AzureADUImage_t * const pxAduImage )
{
    AzureIoTResult_t xResult = eAzureIoTSuccess;

    pxAduImage->pucBufferToWrite = NULL;
    pxAduImage->ulBytesToWriteLength = 0;
    pxAduImage->ulCurrentOffset = 0;
    pxAduImage->ulImageFileSize = 0;


    /* TODO: Fill in initialization for NXP flash writing capability. */

    LogInfo( ( "AzureIoTPlatform_Init()\r\n" ) );
    // status_t status;
    // flexspi_transfer_t flashXfer;
    // hal_flash_status_t initStat = HAL_FlashInit();
    // LogInfo( ( "init status: %08x\r\n", initStat ) );

    /* Write enable */
    // flashXfer.deviceAddress = 0x200000;
    // flashXfer.port = kFLEXSPI_PortA1;
    // flashXfer.cmdType = kFLEXSPI_Command;
    // flashXfer.SeqNumber = 1;
    // flashXfer.seqIndex = NOR_CMD_LUT_SEQ_IDX_WRITEENABLE;

    // status = FLEXSPI_TransferBlocking(FLEXSPI, &flashXfer);
    // Nor_Flash_Init()

    // hal_flash_status_t jksdfl = HAL_FlashInit();
    // LogInfo( ( "flash init: %08x\r\n", jksdfl ) );

    status_t status;
    // LogInfo( ( "Vendor ID: 0x%x\r\n", vendorID ) );
    

    uint8_t read_data;
    // hal_flash_status_t jksddfl = HAL_FlashRead(0x60073f20, 10, &read_data); //E packet....pCon
    // LogInfo( ( "flash read: %08x\r\n", jksddfl ) );
    // LogInfo( ( "flash data: %08x\r\n", read_data ) );
    // jksddfl = HAL_FlashRead(0x60000000, 10, &read_data); // FCFB...V........
    // LogInfo( ( "flash read: %08x\r\n", jksddfl ) );
    // LogInfo( ( "flash data: %08x\r\n", read_data ) );
    // jksddfl = HAL_FlashRead(0x60200000, 10, &read_data); // E packet....pCon
    // LogInfo( ( "flash read: %08x\r\n", jksddfl ) );
    // LogInfo( ( "flash data: %08x\r\n", read_data ) );
// 0x402A8000u flexspi base addr 60000000 0x200000
// dstAddr = 0x200000 or 0x100000
// sec_num prob just number of sectors to erase, so multiples of 0x1000

// address = dstAddr, len = sec_num * flash_area_image_sector_size. If either of them are not divisible by sector size, error

// base = pointer to flexspi_base, which is 0x402A8000u, address = address
// flashXfer.deviceAddress = address;

// mine
// address should be FLEXSPI_AMBA_BASE + address
    // hal_flash_status_t sjdkfl = HAL_FlashEraseSector(0x60200000, 0x1000);
    // LogInfo( ( "flash erase: %08x\r\n", sjdkfl ) );

    // // sjdkfl = HAL_FlashEraseSector(0x60200000, 0x1000);
    // // LogInfo( ( "flash erase: %08x\r\n", sjdkfl ) );

    // jksddfl = HAL_FlashRead(0x60073f20, 10, &read_data);
    // LogInfo( ( "flash read: %08x\r\n", jksddfl ) );
    // LogInfo( ( "flash data: %08x\r\n", read_data ) );
    // jksddfl = HAL_FlashRead(0x60000000, 10, &read_data);
    // LogInfo( ( "flash read: %08x\r\n", jksddfl ) );
    // LogInfo( ( "flash data: %08x\r\n", read_data ) );
    // jksddfl = HAL_FlashRead(0x60200000, 10, &read_data);
    // LogInfo( ( "flash read: %08x\r\n", jksddfl ) );
    // LogInfo( ( "flash data: %08x\r\n", read_data ) );
    // flexspi_nor_write_enable(FLEXSPI, 0x200000);
    
    

    // Nor_Flash_Read();

    // uint8_t vendorID = 0;
    // status = flexspi_nor_get_vendor_id( EXAMPLE_FLEXSPI, &vendorID );

    // if( status != kStatus_Success )
    // {
    //     LogError( ( "Get Vendor ID Failure!" ) );
    //     return eAzureIoTErrorFailed;
    // }

    // LogInfo( ( "Flash Vendor ID: 0x%x\r\n", vendorID ) );

    /* status = flexspi_nor_enable_quad_mode(EXAMPLE_FLEXSPI); */
    /* if (status != kStatus_Success) */
    /* { */
    /*     LogError(("Enable Quad Mode Failure!")); */
    /*     return eAzureIoTErrorFailed; */
    /* } */

    /* LogInfo(("Erasing Serial NOR over FlexSPI...\r\n")); */

    /* status = flexspi_nor_flash_erase_sector(EXAMPLE_FLEXSPI, EXAMPLE_SECTOR * SECTOR_SIZE); */

    /* if (status != kStatus_Success) */
    /* { */
    /*     LogError(("Erase Sector Failure!")); */
    /*     return eAzureIoTErrorFailed; */
    /* } */

    /* uint8_t *nor_read_buffer = pvPortMalloc(256); */
    /* if (NULL == nor_read_buffer) */
    /* { */
    /*     LogError(("nor_read_buffer memory allocation failed!\r\n")); */
    /*     return eAzureIoTErrorFailed; */
    /* } */

    /* memcpy(nor_read_buffer, (void *)(EXAMPLE_FLEXSPI_AMBA_BASE + EXAMPLE_SECTOR * SECTOR_SIZE), FLASH_PAGE_SIZE); */
    /* for (uint16_t i = 0; i < FLASH_PAGE_SIZE; i++) */
    /* { */
    /*     if (0xFF != nor_read_buffer[i]) */
    /*     { */
    /*         LogError(("Erase data -  read out data value incorrect !\r\n ")); */
    /*         return eAzureIoTErrorFailed; */
    /*     } */
    /* } */

    /* Process initialize requests.  */     
    write_image_ok();
    LogInfo(("Write image ok\r\n"));
    uint8_t image_position;
    volatile uint32_t primask;
    
    sfw_flash_read(REMAP_FLAG_ADDRESS, &image_position, 1);
    if(image_position == 0x01)
    {
        dstAddr = FLASH_AREA_IMAGE_2_OFFSET;
        pxAduImage->xUpdatePartition = FLASH_AREA_IMAGE_2_OFFSET;
        LogInfo(("Write to image 2\r\n"));
    }
    else if(image_position == 0x02)
    {
        dstAddr = FLASH_AREA_IMAGE_1_OFFSET;
        pxAduImage->xUpdatePartition = FLASH_AREA_IMAGE_1_OFFSET;
        LogInfo(("Write to image 1\r\n"));
    }
    else
    {
        LogError(("Invalid image position!"));
        return eAzureIoTErrorFailed;
    }

    /* Process firmware preprocess requests before writing firmware.
        Such as: erase the flash at once to improve the speed.  */
    // uint32_t sec_num = 0;
    // if((pxAduImage->ulImageFileSize) % FLASH_AREA_IMAGE_SECTOR_SIZE)
    // {
    //     sec_num = (uint32_t)(pxAduImage->ulImageFileSize / FLASH_AREA_IMAGE_SECTOR_SIZE) + 1;
    // }
    // else
    // {
    //     sec_num = (uint32_t)(pxAduImage->ulImageFileSize / FLASH_AREA_IMAGE_SECTOR_SIZE);
    // }
    LogInfo( ( "Erasing flash bank\r\n" ) );
    primask = DisableGlobalIRQ();
    status = sfw_flash_erase(dstAddr, FLASH_AREA_IMAGE_1_SIZE); //sec_num * FLASH_AREA_IMAGE_SECTOR_SIZE);
    EnableGlobalIRQ(primask);

    if (status)
    {
        LogError(("erase failed.\r\n"));
        xResult = eAzureIoTErrorFailed;
    } 

    return xResult;
}

int64_t AzureIoTPlatform_GetSingleFlashBootBankSize()
{
    /* TODO: Fill in returning of flash bank size (to make sure we don't install an image larger than that) */

    LogInfo( ( "AzureIoTPlatform_GetSingleFlashBootBankSize()\r\n" ) );

    return FLASH_AREA_IMAGE_1_SIZE;
}

AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{


    /* TODO: Fill in to write pData of size ulBlockSize to ulOffset in memory. */

    LogInfo( ( "AzureIoTPlatform_WriteBlock(): offset %i\r\n", ulOffset ) );

    uint32_t pucNextWriteAddr = pxAduImage->xUpdatePartition + ulOffset;
    // // uint8_t * pucNextReadAddr = pData;
    AzureIoTResult_t xResult = eAzureIoTSuccess;
    // // uint32_t ulEndOfBlock = pxAduImage->xUpdatePartition + ulOffset + ulBlockSize;
    volatile uint32_t primask;
    status_t status;

    primask = DisableGlobalIRQ();
    status = sfw_flash_write(pucNextWriteAddr, pData, ulBlockSize);
    EnableGlobalIRQ(primask);

    // while( pucNextWriteAddr < ulEndOfBlock )
    // {
    //     if( HAL_FLASH_Program( FLASH_TYPEPROGRAM_DOUBLEWORD, ( uint32_t ) pucNextWriteAddr, ( uint64_t ) *( uint32_t * ) pucNextReadAddr | ( ( uint64_t ) *( uint32_t * ) ( pucNextReadAddr + 4 ) ) << 32 ) != HAL_OK )
    //     {
    //         /* Error occurred while writing data in Flash memory */
    //         xResult = eAzureIoTErrorFailed;
    //         break;
    //     }

    //     pucNextWriteAddr += azureiotflashL475_DOUBLE_WORD_SIZE;
    //     pucNextReadAddr += azureiotflashL475_DOUBLE_WORD_SIZE;
    // }

    // HAL_FLASH_Lock();
    if (status)
    {
        xResult = eAzureIoTErrorFailed;
    }

    return xResult;
}

AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                               uint8_t * pucSHA256Hash,
                                               uint32_t ulSHA256HashLength )
{
    ( void ) pxAduImage;
    ( void ) pucSHA256Hash;
    ( void ) ulSHA256HashLength;

    /* TODO: Fill in to verify bytes written to flash bank match the SHA256 given by pucSHA256Hash. */

    LogInfo( ( "AzureIoTPlatform_VerifyImage()\r\n" ) );

    // int xResult;
    // uint32_t ulOutputSize;
    // uint32_t ulReadSize;

    // LogInfo( ( "Base64 Encoded Hash from ADU: %.*s", ulSHA256HashLength, pucSHA256Hash ) );
    // xResult = prvBase64Decode( pucSHA256Hash, ulSHA256HashLength, ucDecodedManifestHash, azureiotflashSHA_256_SIZE, ( size_t * ) &ulOutputSize );

    // if( xResult != eAzureIoTSuccess )
    // {
    //     LogError( ( "Unable to decode base64 SHA256\r\n" ) );
    //     return eAzureIoTErrorFailed;
    // }

    // mbedtls_md_context_t ctx;
    // mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    // mbedtls_md_init( &ctx );
    // mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 0 );
    // mbedtls_md_starts( &ctx );

    // LogInfo( ( "Starting the mbedtls calculation: image size %d\r\n", pxAduImage->ulImageFileSize ) );

    // for( size_t ulOffset = 0; ulOffset < pxAduImage->ulImageFileSize; ulOffset += sizeof( ucPartitionReadBuffer ) )
    // {
    //     ulReadSize = pxAduImage->ulImageFileSize - ulOffset < sizeof( ucPartitionReadBuffer ) ? pxAduImage->ulImageFileSize - ulOffset : sizeof( ucPartitionReadBuffer );
    //     sfw_flash_read(( pxAduImage->xUpdatePartition + ulOffset ), ucPartitionReadBuffer, ulReadSize);
    //     // memcpy( ucPartitionReadBuffer, ( pxAduImage->xUpdatePartition + ulOffset ), ulReadSize );

    //     mbedtls_md_update( &ctx, ( const unsigned char * ) ucPartitionReadBuffer, ulReadSize );
    // }

    // LogInfo( ( "mbedtls calculation completed\r\n" ) );

    // mbedtls_md_finish( &ctx, ucCalculatedHash );
    // mbedtls_md_free( &ctx );

    // if( memcmp( ucDecodedManifestHash, ucCalculatedHash, azureiotflashSHA_256_SIZE ) == 0 )
    // {
    //     LogInfo( ( "SHAs match\r\n" ) );
    //     xResult = eAzureIoTSuccess;
    // }
    // else
    // {
    //     LogError( ( "SHAs do not match\r\n" ) );
    //     LogInfo( ( "Wanted: " ) );

    //     for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
    //     {
    //         LogInfo( ( "%x", ucDecodedManifestHash[ i ] ) );
    //     }

    //     LogInfo( ( "\r\n" ) );
    //     LogInfo( ( "Calculated: " ) );

    //     for( int i = 0; i < azureiotflashSHA_256_SIZE; ++i )
    //     {
    //         LogInfo( ( "%x", ucCalculatedHash[ i ] ) );
    //     }

    //     LogInfo( ( "\r\n" ) );

    //     xResult = eAzureIoTErrorFailed;
    // }

    // return xResult;
    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_EnableImage( AzureADUImage_t * const pxAduImage )
{
    ( void ) pxAduImage;

    /* TODO: Fill in to program board to decide which memory bank to use on reboot. */

    LogInfo( ( "AzureIoTPlatform_EnableImage()\r\n" ) );
    /* Set the new firmware for next boot.  */
    enable_image();

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_ResetDevice( AzureADUImage_t * const pxAduImage )
{
    NVIC_SystemReset();

    return eAzureIoTSuccess;
}
