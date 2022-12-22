/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include <string.h>

#include "azure_iot_flash_platform.h"
// #include "mflash_drv.h"
// #include "mflash_file.h"

#include "azure_iot_flash_platform_port.h"
/* Logging */
#include "azure_iot.h"

#include "azure/core/az_base64.h"

#include "mbedtls/md.h"

#include "fsl_nor_flash.h"

#include "fsl_flexspi_nor_flash.h"

#include "fsl_adapter_flash.h"

// #include "flexspi_nor_flash.h"

// #include "flexspi_flash.h"

#include "fsl_adapter_flash.h"

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

    LogInfo( ( "Unencoded the base64 encoding\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_Init( AzureADUImage_t * const pxAduImage )
{
    (void) pxAduImage;

    // TODO: Fill in initialization for NXP flash writing capability.

    LogInfo( ( "AzureIoTPlatform_Init()\r\n" ) );
    status_t status;
    flexspi_transfer_t flashXfer;

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

    uint8_t read_data;
    hal_flash_status_t jksddfl = HAL_FlashRead(0x200000, 10, &read_data);
    LogInfo( ( "flash read: %08x\r\n", jksddfl ) );
    LogInfo( ( "flash data: %.s\r\n", 10, read_data ) );
    // flexspi_nor_write_enable(FLEXSPI, 0x200000);
    
    

    // Nor_Flash_Read();

    return 1;
}

int64_t AzureIoTPlatform_GetSingleFlashBootBankSize()
{
    // TODO: Fill in returning of flash bank size (to make sure we don't install an image larger than that)
    
    LogInfo( ( "AzureIoTPlatform_GetSingleFlashBootBankSize()\r\n" ) );

    return INT64_MAX;
}

AzureIoTResult_t AzureIoTPlatform_WriteBlock( AzureADUImage_t * const pxAduImage,
                                              uint32_t ulOffset,
                                              uint8_t * const pData,
                                              uint32_t ulBlockSize )
{
    (void)pxAduImage;
    (void)ulOffset;
    (void)pData;
    (void)ulBlockSize;

    // TODO: Fill in to write pData of size ulBlockSize to ulOffset in memory.

    LogInfo( ( "AzureIoTPlatform_WriteBlock(): offset %i\r\n", ulOffset ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_VerifyImage( AzureADUImage_t * const pxAduImage,
                                               uint8_t * pucSHA256Hash,
                                               uint32_t ulSHA256HashLength )
{
    (void)pxAduImage;
    (void)pucSHA256Hash;
    (void)ulSHA256HashLength;

    // TODO: Fill in to verify bytes written to flash bank match the SHA256 given by pucSHA256Hash.

    LogInfo( ( "AzureIoTPlatform_VerifyImage()\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_EnableImage( AzureADUImage_t * const pxAduImage )
{
    (void)pxAduImage;

    // TODO: Fill in to program board to decide which memory bank to use on reboot.

    LogInfo( ( "AzureIoTPlatform_EnableImage()\r\n" ) );

    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoTPlatform_ResetDevice( AzureADUImage_t * const pxAduImage )
{
    NVIC_SystemReset();

    return eAzureIoTSuccess;
}
